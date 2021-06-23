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
#include <queue>
#include <vector>
#include "uuid/uuid.h"
#include "json.h"
#include "libgnirs.h"
#include <CExpIFace.h>

#include "libgnirsioc.h"

#define SFS			0x00534653	// Send number of Fowler Samples
#define SDS			0x00534453	// Send number of Digital Samples

#define SBL			0x0053424C      // Set low bias voltage
#define SBV			0x00534256      // set normal bias voltage
#define SBH			0x00534248      // set high bias voltage


struct Config {
	std::string label;
	std::string lod_file;
	unsigned nrows;
	unsigned ncols;
	unsigned nadcs;
	unsigned frames;
	float exposure;
        char wellDepth;
	int sequence;
};

using std::chrono::system_clock;
using std::chrono::steady_clock;
using std::chrono::microseconds;
using std::chrono::seconds;
using sys_time_point = std::chrono::time_point<system_clock>;
using sty_time_point = std::chrono::time_point<steady_clock>;

using std::to_string;
using Pixel = unsigned short;

static constexpr auto READ_TIMEOUT = 200;
static constexpr int SDSU3_PON_BIT = 0x400000;
static constexpr int MAX_FS = 64; // Twice the usual max
static constexpr int MAX_ADCS = 12; // Twice the usual max
static constexpr int ROWS_PER_FRAME = 512;
static constexpr int COLS_PER_FRAME = 2048; // Twice the usual max

static constexpr unsigned rowsPerTransfer = 64;
static constexpr unsigned lagBy = 1;


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
		  << "  gnirsdc [-h] [-d] [-r] [-a #adc] [-f #samples] [-e seconds] [-w <S|M|D>] [-s #sequence]\n\n"
		  << " -h		shows this help page\n"
		  << " -d		increased debugging output\n"
		  << " -r		resets the controller as part of the setup\n"
		  << " -a <#>		number of times to sample the ADCs [Default: 1]\n"
		  << " -f <#>		number of Fowler samples (1 Fowler = reset and signal) [Default: 1]\n"
		  << " -e <s>		expose for <s> seconds [Default: 0.0]\n"
		  << " -w <S|M|D>	set well depth to shallow (-3.2) medium (-3.4) deep (-3.6) [Default: medium]\n"
		  << " -s <#>		number of exposures in a sequence [Default: 1]\n";
}


enum class ExposurePhase {
	FIRST_READOUT,
	EXPOSING,
	SECOND_READOUT
};

struct CopyInfo {
	unsigned buffNo;
};

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
	       	      + "-" + to_string(nADCs) + "ds" +
		      + "-" + get_date() + "-" + get_time();
	json_object_object_add(job, "DATALABE", json_object_new_string(name.c_str()));
}

class DataCollector {
public:
	DataCollector(Pixel *origin, size_t totalPixels, const std::string &basePath, const std::string &baseFileName, int buffNo);
	std::string getFileName() const { return fileName; };
	bool full() const { return currentOffset >= buffSize; }
	void data_save() const;

	inline void update(size_t count)
	{
		Pixel *buffNow = &buffer[currentOffset];

		if (full()) {
			return;
		}
		else if ((buffNow + count) > buffLimit) {
			count = buffLimit - buffNow;
		}

		std::memcpy(buffNow, &origBuffer[currentOffset], count * sizeof(Pixel));

		currentOffset += count;
	}

	virtual ~DataCollector();

private:
	size_t buffSize;
	size_t currentOffset;

	Pixel *origBuffer;
	Pixel *buffer;
	Pixel *buffLimit;
	std::string path;
	std::string fileName;
};

class Camera {
public:
	Camera(CArcDevice *pDevice, const Config &mode);
	void expose(Controller* cont, float expTime, std::string basepath, std::string basename, CExpIFace* exp_iface=nullptr);
private:
	CArcDevice *dev;
	unsigned dRows;
	unsigned dCols;
	unsigned nFrames;
	unsigned nADCs;
};

Camera::Camera(CArcDevice *pDevice, const Config &mode)
	: dev(pDevice),
	  dRows(mode.nrows),
	  dCols(mode.ncols),
	  nFrames(mode.frames),
	  nADCs(mode.nadcs)
{
}

DataCollector::DataCollector(Pixel *origin, size_t totalPixels, const std::string &basePath, const std::string &baseFileName, int buffNo)
	: buffSize(totalPixels),
       	  currentOffset(0),
	  origBuffer(origin)
{
       	buffer = new Pixel[buffSize];
	buffLimit = &buffer[buffSize];

	fileName = baseFileName + std::string(".frame.") + to_string(buffNo);
	path = basePath + fileName;
}

DataCollector::~DataCollector() {
	delete buffer;
}

void DataCollector::data_save() const
{
	std::ofstream fs(path);
	fs.write((const char *)buffer, buffSize * sizeof(Pixel));
	std::cerr << "Saved buffer to file " << fileName << '\n';
}

struct Transfer {
	DataCollector *collector;
	size_t count;
};

void
Camera::expose(Controller* cont, float expTime, std::string basepath, std::string basename, CExpIFace* exp_iface)
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

	if (dev->Command( TIM_ID, SDS, nADCs) != DON) {
		throw std::runtime_error("Set analog digital samples failed");
	}

	const unsigned pixelsPerFrame = dRows * dCols;
	/*const*/ unsigned totalCount = pixelsPerFrame * (nFrames * 2);
	const unsigned pixelsPerTransfer = dCols * rowsPerTransfer; // Copy 4 rows at a time
	int frameIndex = 0;
	unsigned pixelCount = 0;
	int latestPixelCount = 0;
	unsigned pixelsCopied = 0;
	unsigned rowsCopiedThisFrame = 0;
	unsigned pixelsCopiedThisFrame = 0;
	unsigned long long loops = 0;
	std::vector<std::thread *>threads;

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


	DataCollector *collectors[nFrames * 2];
	for (unsigned i = 0; i < (nFrames * 2); i++) {
		collectors[i] = new DataCollector((Pixel *)dev->CommonBufferVA(), dRows * dCols, basepath, basename, i);
		json_object_array_add(samples, json_object_new_string(collectors[i]->getFileName().c_str()));
	}

	DataCollector *currentCollector = collectors[frameIndex];
	std::queue<Transfer> transfers;

	// Create the clock object just before starting the exposure (this will set the reference)
	Clock clock;
	// Start the exposure
	if (dev->Command( TIM_ID, SEX ) != DON) {
		throw std::runtime_error("Starting exposure failed");
	}
	auto ut_start = steady_clock::now();
	bool waiting_for_signal = false;
	bool reading_reset = true;
	clock.set_timing_prefix("RESET_");
	clock.set_timing_index(1);


	while ( pixelsCopied < totalCount) {
		if (pixelCount < totalCount) {
			int pixelRead = dev->GetPixelCount();
			int diff = pixelRead - latestPixelCount;


			if (diff != 0) {
				if (waiting_for_signal && (pixelRead > 0)) {
					clock.add_measurement(steady_clock::now());
					waiting_for_signal = false;
				}
				if (diff < 0) {
					diff = (pixelsPerFrame - latestPixelCount) + pixelRead;
				}
				pixelCount += diff;
				latestPixelCount = pixelRead;
			}
		}

		if ((pixelCount - pixelsCopied) >= pixelsPerTransfer) {
			transfers.push({currentCollector, pixelsPerTransfer});
			if (transfers.size() > lagBy) {
				Transfer t(transfers.front());
				transfers.pop();
				t.collector->update(t.count);
				if (t.collector->full()) {
					threads.push_back(new std::thread(&DataCollector::data_save, t.collector));
				}
			}

			rowsCopiedThisFrame += rowsPerTransfer;
			pixelsCopiedThisFrame += pixelsPerTransfer;
			pixelsCopied += pixelsPerTransfer;

//	 		cont->save_to(basename + std::to_string(frameIndex) + ".fits");

			if (rowsCopiedThisFrame >= dRows) {
				clock.add_measurement(steady_clock::now());

				if (reading_reset && (clock.timing_index() > nFrames)) {
					reading_reset = false;
					waiting_for_signal = true;
					clock.set_timing_prefix("SIGNAL_");
					clock.set_timing_index(0);
				}

				frameIndex++;
				currentCollector = collectors[frameIndex];
				rowsCopiedThisFrame = 0;
				pixelsCopiedThisFrame = 0;
			}
		}

		loops++;
	}
	auto ut_end = steady_clock::now();
	while (transfers.size() > 0) {
		Transfer t(transfers.front());
		transfers.pop();
		t.collector->update(t.count);
		if (t.collector->full()) {
			threads.push_back(new std::thread(&DataCollector::data_save, t.collector));
		}
	}

	dev->StopExposure();
	std::cerr << "Total loops = " << loops << '\n';
	std::cerr << "Joining threads\n";
	for (auto t: threads) {
		t->join();
		delete t;
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

void parse_cmd(int argc, char **argv, Config& mode, bool& reset, bool& debug)
{

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

			float exp = std::stof(argv[argi]);
			if (exp < 0.0) {
				std::cerr << "Negative exposure is not allowed\n";
				exit(1);
			}
			mode.exposure = exp;
		}
		else if (current == "-f") {
			++argi;
			if (argi >= argc) {
				std::cerr << "Missing argument for -f\n";
				exit(1);
			}

			int samp = std::stoi(argv[argi]);
			if ((samp < 1) || (samp > MAX_FS)) {
				std::cerr << "Fowler samples out of range (1 .. " << MAX_FS << ")\n";
				exit(1);
			}
			mode.frames = unsigned(samp);
		}
		else if (current == "-a") {
			++argi;
			if (argi >= argc) {
				std::cerr << "Missing argument for -a\n";
				exit(1);
			}

			int na = std::stoi(argv[argi]);
			if ((na < 1) || (na > MAX_ADCS)) {
				std::cerr << "Analog Digital Conversions out of range (1.. " << MAX_ADCS << ")\n";
				exit(1);
			}
			mode.nadcs = unsigned(na);

		}
		else if (current == "-w") {
                        ++argi;

			if (argi >= argc) {
				std::cerr << "Missing argument for -w\n";
				exit(1);
			}

			mode.wellDepth = argv[argi][0];


			std::cout << "WellDepth: " << mode.wellDepth << " value: " << 'S' << '\n';
			
			if (mode.wellDepth != 'S' &&
			    mode.wellDepth != 'M' && 
			    mode.wellDepth != 'D') {

  			   std::cerr << "Bad well depth argument " << argv[argi] << '\n';
			   exit(1);
    			}
		}
		else if (current == "-s") {
			++argi;
			if (argi >= argc) {
				std::cerr << "Missing argument for -s\n";
				exit(1);
			}

			mode.sequence = std::stoi(argv[argi]);
			if (mode.sequence < 1) {
				std::cerr << "Number in sequence must be positive\n";
				exit(1);
			}
		}
		else {
			std::cerr << "Unknown argument " << argv[argi] << '\n';
			exit(1);
		}
	}

	mode.nrows = ROWS_PER_FRAME;
        mode.ncols = COLS_PER_FRAME * mode.nadcs; 
}

std::string get_uuid() {
	uuid_t uuid;
	char uuid_string[40];

	uuid_generate(uuid);
	uuid_unparse(uuid, uuid_string);

	return std::string(uuid_string);
}


int dcInit(int argc, char **argv) {

	bool reset = false;
	bool debug = false;
	
	Config mode{"", "/usr/local/bin/DSP/Aladdin_SDSU_Firmware.lod", 512, 2048, 1, 1, 0.0, 'M', 1};

	parse_cmd(argc-1, &argv[1], mode, reset, debug);

	Controller cont(mode.nrows, mode.ncols);

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

	// Setting the bias voltage 
        switch (mode.wellDepth) {
		case 'S':
      	      		std::cout << "Well Depth set to -3.2 \n";
			if (cont.getDev()->Command( TIM_ID, SBL ) != DON) {
				throw std::runtime_error("Set bias voltage");
			}
		break;
		case 'M':
      	      		std::cout << "Well Depth set to -3.4 \n";
			if (cont.getDev()->Command( TIM_ID, SBV ) != DON) {
				throw std::runtime_error("Set bias voltage");
			}
		break;
		case 'D':
      	      		std::cout << "Well Depth set to -3.6 \n";
			if (cont.getDev()->Command( TIM_ID, SBH ) != DON) {
				throw std::runtime_error("Set bias voltage");
			}
		break;
	}

	return 0;
}



int dcExpose(int argc, char **argv) {
	bool reset = false;
	bool debug = false;
	
	Config mode{"", "/usr/local/bin/DSP/Aladdin_SDSU_Firmware.lod", 512, 2048, 1, 1, 0.0, 'M', 1};

	parse_cmd(argc-1, &argv[1], mode, reset, debug);

	Controller cont(mode.nrows, mode.ncols);

	if (debug) {
		std::cout << "Testing for mode: " << mode.label << '\n';
	}
	std::cout << "Exposing for " << mode.exposure << " seconds\n";

	if (debug)
		std::cout << "Exposing\n";

	ExpIFace callbacks(debug);

//	cont.start_logging();
//
	Camera camera(cont.getDev(), mode);

        std::cout << "Sequence " << mode.sequence << std::endl;

        for (int i=0; i < mode.sequence; i++) {
		camera.expose(&cont, mode.exposure, "/home/readout_data/new/", get_uuid(), &callbacks);
	}

//	cont.stop_logging(std::cout);
//	custom_expose(cont.getDev(), mode.exposure, mode.nrows, mode.ncols, &callbacks);

//	cont.save_to("test_file2.fits");

	return 0;
}
