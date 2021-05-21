#include <iostream>
#include <map>
#include <string>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <cstring>
#include <ctime>
#include <chrono>
#include "uuid/uuid.h"
#include "json.h"
#include "libgnirs.h"
#include <CExpIFace.h>

struct Config {
	std::string label;
	std::string lod_file;
	unsigned nrows;
	unsigned ncols;
	unsigned nadcs;
	unsigned frames;
	float exposure;
};

using std::chrono::system_clock;
using std::chrono::steady_clock;
using std::chrono::microseconds;
using std::chrono::seconds;
using sys_time_point = std::chrono::time_point<system_clock>;
using sty_time_point = std::chrono::time_point<steady_clock>;
using std::to_string;

static constexpr auto READ_TIMEOUT = 200;
static constexpr int SDSU3_PON_BIT = 0x400000;
static constexpr int MAX_FS = 64; // Twice the usual max

enum class AdcType {
	ADC_1 = 1,
	ADC_6 = 6
};

std::map<AdcType, Config> setups{
	{AdcType::ADC_1, {"1 ADC per Fowler Sample", "./DSP/Aladdin_2048_1024XnFS_1DS_3V4.lod", 512, 2048, 1}},
	{AdcType::ADC_6, {"6 ADC per Fowler Sample", "./DSP/Aladdin_12288_1024XnFS_6DS_3V4.lod", 512, 6 * 2048, 6}},
};

using namespace arc::device;
using namespace arc::deinterlace;

class ExpIFace : public CExpIFace
{
public:
	ExpIFace(bool debugging) : dbg(debugging) {}

	void GeneralMessage( std::string& msg ) {
		if (dbg)
			std::cout << msg << '\n';
	}

	virtual void ExposeCallback( float fElapsedTime )
	{
		if (dbg) {
			std::cout << "Exposing. Elapsed time: " << ((fElapsedTime > 0.000001) ? fElapsedTime : 0.0) << '\n';
		}
	}

	virtual void ReadCallback( int dPixelCount )
	{
		if (dbg)
			std::cout << "Pixel Count: " << dPixelCount << '\n';
	}
private:
	bool dbg;
};

void print_help()
{
	std::cerr << "ARC Detector Testing Program\n\n"
		  << "   testing [-h] [-d] [-r] [-a #adc] [-s #samples] [-e seconds]\n\n"
		  << " -h     shows this help page\n"
		  << " -d     increased debugging output\n"
		  << " -r     resets the controller as part of the setup\n"
		  << " -a <#> take <#> ADC samples per Fowler [Default: 1; Valid: 1, 6]\n"
		  << " -s <#> acquire <#> Fowler samples (both for reset and signal) [Default: 1; Max: 64]\n"
		  << " -e <s> expose por <s> seconds [Default: 0.0]\n";
}

/*
 * custom_expose is a high-level exposure function tailored specifically for the timing files
 * written for GNIRS/ARC
 */

enum class ExposurePhase {
	FIRST_READOUT,
	EXPOSING,
	SECOND_READOUT
};

inline void copy_raw(CArcDevice* dev, unsigned short *buf, size_t offset, size_t count)
{
	std::memcpy(buf + offset, ((unsigned short *)dev->CommonBufferVA()) + offset, count * sizeof(unsigned short));
}

std::string left_justify(std::string s, int size, char fillchar) {
	int nfill = size - s.length();
	return nfill > 0 ? std::string(nfill, fillchar) + s : s;
}

class Clock {
public:
	Clock() {
		sys_clock_ref = system_clock::now();
		sty_clock_ref = steady_clock::now();
	}

	void json_set_gmtime(json_object *job, const char *key, sty_time_point &t)
	{
		auto point = sys_clock_ref + (t - sty_clock_ref);
		auto point_us = std::chrono::time_point_cast<microseconds>(point);
		auto point_s = std::chrono::time_point_cast<seconds>(point);
		auto us = point_us.time_since_epoch() - (point_s.time_since_epoch());
		const std::time_t t_c = system_clock::to_time_t(point);
		std::string time_string;

		char target[9];
		strftime(target, 9, "%H:%M:%S", std::gmtime(&t_c));

		time_string = std::string(target) + "." + left_justify(to_string(long(us.count())), 6, '0');

		json_object_object_add(job, key, json_object_new_string(time_string.c_str()));
	}

	void json_set_gmtime(json_object *job, const char *key) {
		json_set_gmtime(job, key, sty_clock_ref);
	}

	void set_timing_prefix(std::string new_t_prefix) { t_prefix = new_t_prefix; }
	void set_timing_index(unsigned new_t_index) { t_index = new_t_index; }
	unsigned timing_index() const { return t_index; }
	void add_measurement(sty_time_point measurement, bool increment_index=true) {
		measurements[t_prefix + left_justify(to_string(t_index), 2, '0')] = measurement;
		if (increment_index)
			t_index++;
	}

	void visit_measurements(std::function<void(std::string, double)> fn) const {
		for (auto it=measurements.begin(); it != measurements.end(); it++) {
			auto diff = double(((*it).second - sty_clock_ref).count()) / 1000000000;
			fn((*it).first, diff);
		}
	}

	void print_measurements() const {
		for (auto it=measurements.begin(); it != measurements.end(); it++) {
			auto diff = double(((*it).second - sty_clock_ref).count()) / 1000000000;
			std::cerr << "     " << (*it).first << "   " << diff << '\n';
		}
	}

private:
	sys_time_point sys_clock_ref;
	sty_time_point sty_clock_ref;

	std::map<std::string, sty_time_point> measurements;

	std::string t_prefix;
	unsigned t_index;
};

void print_measurement(std::string label, double diff) {
	std::cerr << "     " << label << "   " << diff << '\n';
}

std::string get_date() {
	auto seconds_now = std::time(nullptr);
	std::tm *date_now = gmtime(&seconds_now);
	char the_date[11];
	std::strftime(the_date, 11, "%Y-%m-%d", date_now);

	return std::string(the_date);
}

std::string get_time() {
	const std::time_t t_c = system_clock::to_time_t(system_clock::now());
	std::string time_string;

	char target[9];
	strftime(target, 9, "%H:%M:%S", std::gmtime(&t_c));

	return std::string(target);
}

void json_set_gmdate(json_object *job, const char *key)
{
	json_object_object_add(job, key, json_object_new_string(get_date().c_str()));
}

void json_set_datalabel(json_object *job, std::string prefix, unsigned nFrames, unsigned nADCs) {
	std::string name = prefix;

	name = prefix + "-" + to_string(nFrames) + "x" + to_string(nFrames)
	       	      + "-" + to_string(nADCs) + "adc" +
		      + "-" + get_date() + "-" + get_time();
	json_object_object_add(job, "DATALABE", json_object_new_string(name.c_str()));
}

class DataCollector {
public:
	DataCollector(const Config &mode);
	void expose(Controller* cont, CArcDevice* dev, float expTime, std::string basepath, std::string basename, CExpIFace* exp_iface=nullptr);
	void data_save(std::string path, std::string prefix, unsigned buffNo, json_object *sample_array);

	virtual ~DataCollector();

private:
	unsigned dRows;
	unsigned dCols;
	unsigned nFrames;
	unsigned nADCs;
	unsigned buffSize;
	unsigned short **buffers;
};

DataCollector::DataCollector(const Config &mode)
	: dRows(mode.nrows),
	  dCols(mode.ncols),
	  nFrames(mode.frames),
	  nADCs(mode.nadcs)
{
	buffSize = dRows * dCols;
	buffers = new unsigned short *[nFrames * 2];
	for (unsigned i = 0; i < (nFrames * 2); i++)
		buffers[i] = new unsigned short[buffSize];
}

DataCollector::~DataCollector() {
	for (unsigned i = 0; i < (nFrames * 2); i++)
		delete buffers[i];
	delete buffers;
}

void
DataCollector::expose(Controller* cont, CArcDevice* dev, float expTime, std::string basepath, std::string basename, CExpIFace* exp_iface)
{
	int msec = int( expTime * 1000 );
//	ExposurePhase status = ExposurePhase::FIRST_READOUT;

	// Setting the exposure time
	if (dev->Command( TIM_ID, SET, msec ) != DON) {
		throw std::runtime_error("Set exposure time failed");
	}

	if (dev->Command( TIM_ID, SFS, nFrames) != DON) {
		throw std::runtime_error("Set number of frames failed");
	}

	const unsigned pixelsPerFrame = dRows * dCols;
	const unsigned totalCount = pixelsPerFrame * (nFrames * 2);
	const unsigned rowsPerTransfer = 4;
	const unsigned pixelsPerTransfer = dCols * rowsPerTransfer; // Copy 4 rows at a time
	int bufferIndex = 0;
	unsigned short *currentBuffer = buffers[bufferIndex];
	int frameCount = 0;
	unsigned pixelCount = 0;
	int latestPixelCount = 0;
	unsigned pixelsCopied = 0;
	unsigned rowsCopiedThisFrame = 0;
	unsigned pixelsCopiedThisFrame = 0;
	unsigned long long loops = 0;
	std::thread *threads[nFrames * 2];
	Clock clock;

	json_object *json_output = json_object_new_object();
	json_object *pdu = json_object_new_object();
	json_object *samples = json_object_new_array();
	json_object *timing = json_object_new_object();

	json_object_object_add(json_output, "PDU", pdu);
	json_object_object_add(json_output, "FRAMES", samples);
	json_object_object_add(json_output, "TIME_SAMPLES", timing);

	json_object_object_add(pdu, "CAMERA", json_object_new_string("GNIRS"));
	json_set_datalabel(pdu, "test-image", nFrames, nADCs);
	json_object_object_add(pdu, "LRNS", json_object_new_int(nFrames));
	json_object_object_add(pdu, "NDAVGS", json_object_new_int(nADCs));
	json_object_object_add(pdu, "RAW_COLS", json_object_new_int(dCols));
	json_object_object_add(pdu, "RAW_ROWS", json_object_new_int(dRows * nFrames * 2));
	json_object_object_add(pdu, "EXPTIME", json_object_new_double(expTime));
	json_set_gmdate(pdu, "DATEOBS");

	// Start the exposure
	if (dev->Command( TIM_ID, SEX ) != DON) {
		throw std::runtime_error("Starting exposure failed");
	}
	auto ut_start = steady_clock::now();
	bool waiting_for_signal = false;
	bool reading_reset = true;
	clock.set_timing_prefix("RESET_");
	clock.set_timing_index(1);

	while ( pixelsCopied < totalCount ) {
		if (pixelCount < totalCount) {
			int pixelRead = dev->GetPixelCount();
			int diff = pixelRead - latestPixelCount;

			if (diff != 0) {
				if (waiting_for_signal && (pixelRead > 0)) {
					clock.add_measurement(steady_clock::now());
					waiting_for_signal = false;
				}
				if (diff < 0) {
					frameCount ++;
					diff = (pixelsPerFrame - latestPixelCount) + pixelRead;
				}
				pixelCount += diff;
				latestPixelCount = pixelRead;
			}
		}

		if ((pixelCount - pixelsCopied) >= pixelsPerTransfer) {

			copy_raw(dev, currentBuffer, pixelsCopiedThisFrame, pixelsPerTransfer);

			rowsCopiedThisFrame += rowsPerTransfer;
			pixelsCopiedThisFrame += pixelsPerTransfer;
			pixelsCopied += pixelsPerTransfer;

			if (rowsCopiedThisFrame >= dRows) {
				clock.add_measurement(steady_clock::now());
				threads[bufferIndex] = new std::thread(&DataCollector::data_save, this, basepath, basename, bufferIndex, samples);
				
				cont->save_to(basename + std::to_string(bufferIndex) + ".fits");

				if (reading_reset && (clock.timing_index() > nFrames)) {
					reading_reset = false;
					waiting_for_signal = true;
					clock.set_timing_prefix("SIGNAL_");
					clock.set_timing_index(0);
				}

				bufferIndex++;
				currentBuffer = buffers[bufferIndex];
				rowsCopiedThisFrame = 0;
				pixelsCopiedThisFrame = 0;
			}
		}

		loops++;
	}
	auto ut_end = steady_clock::now();

	dev->StopExposure();
	std::cerr << "Total loops = " << loops << '\n';
	std::cerr << "Joining threads\n";
	for (unsigned i = 0; i < (nFrames * 2); i++)
		if (threads[i] != nullptr) {
			threads[i]->join();
			delete threads[i];
		}
	std::cerr << "Writing header\n";
	clock.json_set_gmtime(pdu, "UTSTART", ut_start);
	clock.json_set_gmtime(pdu, "UTEND", ut_end);

	auto add_measurements = [timing](std::string label, double diff) { json_object_object_add(timing, label.c_str(), json_object_new_double(diff)); };

	clock.visit_measurements(add_measurements);

	std::ostringstream oss;
	oss << basepath + basename << ".header";
	std::ofstream ofs(oss.str());

	ofs << json_object_to_json_string_ext(json_output, JSON_C_TO_STRING_PRETTY) << '\n';
	json_object_put(json_output);
}

void DataCollector::data_save(std::string path, std::string prefix, unsigned buffNo, json_object *sample_array)
{
	std::ostringstream oss;
	oss << prefix << ".frame." << buffNo;

	std::cerr << "Saving buffer " << buffNo << '\n';
	std::string filename = oss.str();
	std::ofstream fs(path + filename);
	fs.write((const char *)buffers[buffNo], buffSize * sizeof(unsigned short));
	std::cerr << "Saved buffer " << buffNo << " to file " << oss.str() << '\n';
	json_object_array_add(sample_array, json_object_new_string(filename.c_str()));
}

void parse_cmd(int argc, char **argv, Config& mode, bool& reset, bool& debug)
{
	unsigned samples = 1;
	AdcType adcs = AdcType::ADC_1;
	float exp = 0.0;

	for (int argi = 0; argi < argc; ++argi) {
		std::string current(argv[argi]);

		if (current == "-h") {
			print_help();
			exit(0);
		}
		else if (current == "-r") {
			reset = true;
		}
		else if (current == "-d") {
			debug = true;
		}
		else if (current == "-e") {
			++argi;
			if (argi >= argc) {
				std::cerr << "Missing argument for -e\n";
				exit(1);
			}

			exp = std::stof(argv[argi]);
			if (exp < 0.0) {
				std::cerr << "Negative exposure is not allowed\n";
				exit(1);
			}
		}
		else if (current == "-s") {
			++argi;
			if (argi >= argc) {
				std::cerr << "Missing argument for -s\n";
				exit(1);
			}

			int samp = std::stoi(argv[argi]);
			if ((samp < 1) || (samp > MAX_FS)) {
				std::cerr << "Fowler samples out of range (1 .. " << MAX_FS << ")\n";
				exit(1);
			}
			samples = unsigned(samp);
		}
		else if (current == "-a") {
			++argi;
			if (argi >= argc) {
				std::cerr << "Missing argument for -a\n";
				exit(1);
			}

			int raw_adcs = std::stoi(argv[argi]);
			switch (raw_adcs) {
				case 1:
					adcs = AdcType::ADC_1;
					break;
				case 6:
					adcs = AdcType::ADC_6;
					break;
				default:
					std::cerr << "Illegal ADC number: " << argv[argi] << '\n';
					exit(1);
			}
		}
		else {
			std::cerr << "Unknown argument " << argv[argi] << '\n';
			exit(1);
		}
	}

	if (setups.count(adcs) == 0) {
		std::cerr << "Something went wrong: no mode found for the configured ADCs\n";
		exit(1);
	}

	mode = setups.at(adcs);
	mode.exposure = exp;
	mode.frames = samples;
}

std::string get_uuid() {
	uuid_t uuid;
	char uuid_string[40];

	uuid_generate(uuid);
	uuid_unparse(uuid, uuid_string);

	return std::string(uuid_string);
}

int main(int argc, char **argv) {
	bool reset = false;
	bool debug = false;
	Config mode{"", "", 0, 0, 0, 0, 0.0};

	parse_cmd(argc-1, &argv[1], mode, reset, debug);

	Controller cont(mode.nrows, mode.ncols);

	if (debug) {
		std::cout << "Testing for mode: " << mode.label << '\n';
	}
	std::cout << "Exposing for " << mode.exposure << " seconds\n";

	cont.connect_device();

	std::cout << "List of devices:\n";
	for (auto st: cont.device_list()) {
		std::cout << "  " << st << '\n';
	}

	std::cout << "TDL testing: " << cont.tdl_testing(123) << '\n';
	std::cout << "Performing reset\n";
	cont.getDev()->Reset();

	if (debug)
		std::cout << "Setting up with file" << mode.lod_file << '\n';
	cont.setup_controller(mode.lod_file, true, reset); // Power on
	if (!reset)
		cont.set_size(mode.nrows, mode.ncols);
	// cont.set_synthetic(false);
	if (debug)
		std::cout << "Exposing\n";

	ExpIFace callbacks(debug);
//	cont.start_logging();
	DataCollector collector(mode);
	collector.expose(&cont, cont.getDev(), mode.exposure, "raw/", get_uuid(), &callbacks);
//	cont.stop_logging(std::cout);
//	custom_expose(cont.getDev(), mode.exposure, mode.nrows, mode.ncols, &callbacks);

	cont.save_to("test_file2.fits");

	return 0;
}
