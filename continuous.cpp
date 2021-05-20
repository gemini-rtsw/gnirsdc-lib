#include <iostream>
#include <map>
#include <string>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <cstring>
#include "uuid/uuid.h"
#include "json.h"
#include "libgnirs.h"
#include <CExpIFace.h>

struct Config {
	std::string label;
	std::string lod_file;
	unsigned nrows;
	unsigned ncols;
	unsigned frames;
	float exposure;
};

static constexpr auto READ_TIMEOUT = 200;
static constexpr int SDSU3_PON_BIT = 0x400000;
static constexpr int MAX_FS = 64; // Twice the usual max

enum class AdcType {
	ADC_1 = 1,
	ADC_6 = 6
};

std::map<AdcType, Config> setups{
	{AdcType::ADC_1, {"1 ADC per Fowler Sample", "./DSP/Aladdin_2048_1024XnFS_1DS_3V4.lod", 512, 2048}},
	{AdcType::ADC_6, {"6 ADC per Fowler Sample", "./DSP/Aladdin_12288_1024XnFS_6DS_3V4.lod", 512, 6 * 2048}},
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
		  << " -e <s> expose por <s> seconds [Default: 0.0]\n"
		  << " <mode> needs to be one of the following:\n\n";
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

class DataCollector {
public:
	DataCollector(const Config &mode);
	void expose(CArcDevice* dev, float expTime, std::string basepath, CExpIFace* exp_iface=nullptr);
	void data_save(std::string prefix, unsigned buffNo, json_object *sample_array);

	virtual ~DataCollector();

private:
	unsigned dRows;
	unsigned dCols;
	unsigned nFrames;
	unsigned buffSize;
	unsigned short **buffers;
};

DataCollector::DataCollector(const Config &mode)
	: dRows(mode.nrows),
	  dCols(mode.ncols),
	  nFrames(mode.frames)
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
DataCollector::expose(CArcDevice* dev, float expTime, std::string basepath, CExpIFace* exp_iface)
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

	json_object *json_output = json_object_new_object();
	json_object *pdu = json_object_new_object();
	json_object *samples = json_object_new_array();

	json_object_object_add(json_output, "PDU", pdu);
	json_object_object_add(json_output, "FRAMES", samples);

	json_object_object_add(pdu, "CAMERA", json_object_new_string("GNIRS"));
	json_object_object_add(pdu, "DATALABE", json_object_new_string("foobar-vb"));
	json_object_object_add(pdu, "DATEOBS", json_object_new_string("2021-01-01"));
	json_object_object_add(pdu, "UTSTART", json_object_new_string("00:00:00.000000"));
	json_object_object_add(pdu, "UTEND", json_object_new_string("00:00:00.000001"));
	json_object_object_add(pdu, "LRNS", json_object_new_int(nFrames));
	json_object_object_add(pdu, "NDAVGS", json_object_new_int(1));
	json_object_object_add(pdu, "RAW_COLS", json_object_new_int(dCols));
	json_object_object_add(pdu, "RAW_ROWS", json_object_new_int(dRows * nFrames * 2));
	json_object_object_add(pdu, "EXPTIME", json_object_new_double(expTime));

	// Start the exposure
	if (dev->Command( TIM_ID, SEX ) != DON) {
		throw std::runtime_error("Starting exposure failed");
	}

	while ( pixelsCopied < totalCount ) {
		if (pixelCount < totalCount) {
			int pixelRead = dev->GetPixelCount();
			int diff = pixelRead - latestPixelCount;

			if (diff != 0) {
				std::cerr << '[' << frameCount << "] ";
				if (diff < 0) {
					frameCount ++;
					diff = (pixelsPerFrame - latestPixelCount) + pixelRead;
				}
				pixelCount += diff;
				std::cerr << latestPixelCount << " -> " << pixelRead << " => "
					<< pixelCount << " (" << pixelsCopied << ")\n";
				latestPixelCount = pixelRead;
			}
		}

		if ((pixelCount - pixelsCopied) >= pixelsPerTransfer) {
			copy_raw(dev, currentBuffer, pixelsCopiedThisFrame, pixelsPerTransfer);

			rowsCopiedThisFrame += rowsPerTransfer;
			pixelsCopiedThisFrame += pixelsPerTransfer;
			pixelsCopied += pixelsPerTransfer;

			if (rowsCopiedThisFrame >= dRows) {
				threads[bufferIndex] = new std::thread(&DataCollector::data_save, this, basepath, bufferIndex, samples);
				bufferIndex++;
				currentBuffer = buffers[bufferIndex];
				rowsCopiedThisFrame = 0;
				pixelsCopiedThisFrame = 0;
			}
		}

		loops++;
	}

	dev->StopExposure();
	std::cerr << "Total loops = " << loops << '\n';
	std::cerr << "Joining threads\n";
	for (unsigned i = 0; i < (nFrames * 2); i++)
		if (threads[i] != nullptr) {
			threads[i]->join();
			delete threads[i];
		}
	std::cerr << "Writing header\n";

	std::ostringstream oss;
	oss << basepath << ".header";
	std::ofstream ofs(oss.str());

	ofs << json_object_to_json_string_ext(json_output, JSON_C_TO_STRING_PRETTY) << '\n';
	json_object_put(json_output);
}

void DataCollector::data_save(std::string prefix, unsigned buffNo, json_object *sample_array)
{
	std::ostringstream oss;
	oss << prefix << ".frame." << buffNo;

	std::cerr << "Saving buffer " << buffNo << '\n';
	std::string filename = oss.str();
	std::ofstream fs(filename);
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
	Config mode{"", "", 0, 0, 0, 0.0};

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
	collector.expose(cont.getDev(), mode.exposure, get_uuid(), &callbacks);
//	cont.stop_logging(std::cout);
//	custom_expose(cont.getDev(), mode.exposure, mode.nrows, mode.ncols, &callbacks);

//	cont.save_to("test_file2.fits");

	return 0;
}
