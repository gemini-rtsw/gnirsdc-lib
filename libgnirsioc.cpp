
#include <iostream>
#include <filesystem>
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
#include <uuid/uuid.h>
#include <json-c/json.h>
#include "libgnirs.h"
#include <CExpIFace.h>
#include <execinfo.h>
#include <signal.h>
#include <functional>

#include "version.h"
#include "libgnirsioc.h"

#define SFS			0x00534653	// Send number of Fowler Samples
#define SDS			0x00534453	// Send number of Digital Samples
#define AEX			0x00414558	// Abort

#define SBL			0x0053424C      // Set low bias voltage
#define SBV			0x00534256      // set normal bias voltage
#define SBH			0x00534248      // set high bias voltage

#define SNC 			0x00534E43      // Set number of clockouts
#define COA 			0x00434F41      // Clock out array 
#define RAR 			0x00524152      // Reset array 
#define RRO			0x0052524f      // Reset then readout array 
#define ROR			0x00524f52      // Reset then readout array 


static constexpr int ROWS_BUFFER = 512;
static constexpr int COLS_BUFFER = 12288; 

using Pixel = unsigned short;

bool gIsDebug = false;

void* debugBuffer;

// Function to set the global variable
void setGlobalDebug(bool debug) {
    gIsDebug = debug;

	if (gIsDebug) {
		debugBuffer = malloc(ROWS_BUFFER * COLS_BUFFER * sizeof(Pixel));
	}
}


Controller *gCont;

using std::chrono::system_clock;
using std::chrono::steady_clock;
using std::chrono::microseconds;
using std::chrono::seconds;
using sys_time_point = std::chrono::time_point<system_clock>;
using sty_time_point = std::chrono::time_point<steady_clock>;

using std::to_string;

static constexpr auto READ_TIMEOUT = 200;
static constexpr int SDSU3_PON_BIT = 0x400000;
static constexpr int MAX_FS = 256; 
static constexpr int MAX_ADCS = 256; 
static constexpr int ROWS_PER_FRAME = 512;
static constexpr int COLS_PER_FRAME = 2048; // Twice the usual max

static constexpr double singleReadoutTime = 0.238241778;
static constexpr double singleADCTime = 0.1;

static constexpr unsigned rowsPerTransfer = 64;
static constexpr unsigned lagBy = 1;

bool gIsAladdinIII = false;


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
		  << "  gnirsdc [-h] [-d] [-r] [-a #adc] [-f #samples] [-e seconds] [-w <S|M|D>] [-s #coadds]\n\n"
		  << " -h		shows this help page\n"
		  << " -d		increased debugging output\n"
		  << " -r		resets the controller as part of the setup\n"
		  << " -a <#>		number of times to sample the ADCs [Default: 1]\n"
		  << " -f <#>		number of Fowler samples (1 Fowler = reset and signal) [Default: 1]\n"
		  << " -e <s>		expose for <s> seconds [Default: 0.0]\n"
		  << " -w <S|M|D>	set well depth to shallow (-3.2) medium (-3.4) deep (-3.6) [Default: medium]\n"
		  << " -s <#>		number of exposures in coadds [Default: 1]\n";
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
		measurements[t_prefix + left_justify(to_string(t_index), 3, '0')] = measurement;
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

	double getMeasurementDelta(std::string prefix, int index) {
		return double((measurements[prefix + left_justify(to_string(index), 3, '0')] - sty_clock_ref).count()) / 1000000000;
	}

	sty_time_point getMeasurementTime(std::string prefix, int index) {
		return measurements[prefix + left_justify(to_string(index), 3, '0')];
	}

private:
	std::map<std::string, sty_time_point> measurements;

	sys_time_point sys_clock_ref;
	sty_time_point sty_clock_ref;


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
	json_object_object_add(job, "RAWLABEL", json_object_new_string(name.c_str()));
}

class DataCollector {
public:
	DataCollector(Pixel *origin, size_t totalPixels, int cols, const std::string &basePath, const std::string &baseFileName, int buffNo);
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

//		std::cout << "Copied DMA block\n";

		currentOffset += count;

		if (full() && gIsAladdinIII) {
			copyRow513(mCols);
		}
	}

        inline void copyRow513(size_t cols)
        {
//		std::cout << "Aladdin III copied DMA extra row 513\n";


		Pixel *buff512 = &buffer[currentOffset - cols]; //row 512 (last row) in new buffer
		Pixel *buff511 = &buffer[currentOffset - 2 * cols]; //row 511 (2nd to last row) in new buffer

		for (unsigned int i=0; i < cols; i+=32) {

			std::memcpy(&buff512[i+16], &origBuffer[currentOffset + i], 16 * sizeof(Pixel)); //row 513 in PCI device buffer
													 // copy from row 513 quad 1 & 2 to row 512 3 & 4
													 //
			std::memcpy(&buff511[i+16], &origBuffer[currentOffset + i+16], 16 * sizeof(Pixel)); //row 513 in PCI device buffer
													 // copy from row 513 quad 3 & 4 to row 511 of 3 & 4
		}

        }

	virtual ~DataCollector();

private:
	size_t buffSize;
	int mCols;
	size_t currentOffset;

	Pixel *origBuffer;
	Pixel *buffer;
	Pixel *buffLimit;
	std::string path;
	std::string fileName;
};


DataCollector::DataCollector(Pixel *origin, size_t totalPixels, int cols, const std::string &basePath, const std::string &baseFileName, int buffNo)
	: buffSize(totalPixels),
	  mCols(cols),
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

	if (!fs) {
		std::cerr << "Failed to open file: " << path << std::endl;
		return;
	}

	fs.write((const char *)buffer, buffSize * sizeof(Pixel));
	std::cerr << "saved buffer to file " << fileName << '\n';

}


#define cExpConst1 0.023850195
#define cExpConst2 0.214065865

class Camera {
public:
	Camera(CArcDevice *pDevice, const ReadoutConfig &mode);
	void expose(Controller* cont, float expTime, std::string basepath, std::string basename, CExpIFace* exp_iface, std::function<void (json_object*, json_object*)>);
	void abort();

	static bool isAbort;

private:
	CArcDevice *dev;
	unsigned dRows;
	unsigned dCols;
	unsigned nFrames;
	unsigned nADCs;
	unsigned nDropFrames;
	bool readUpTheRamp;
	std::string dataLabel;

	double getExposureOverhead(double fowlers, double  ADCs) {
		return (cExpConst1 + cExpConst2 * ADCs) * fowlers;
	}		

	double getExposureDelay(double requestedExpTime) {
		std::cout << "exp delay: " << requestedExpTime << " -  " <<  (cExpConst2 * nFrames * nADCs + cExpConst1  * nFrames) << " = " << requestedExpTime - (cExpConst2 * nFrames * nADCs + cExpConst1 * nFrames) << std::endl;

		return std::max((double)0, requestedExpTime - (cExpConst1 + cExpConst2 * nADCs) * nFrames);
	}
};


bool Camera::isAbort = false; 

Camera::Camera(CArcDevice *pDevice, const ReadoutConfig &mode)
	: dev(pDevice),
	  dRows(mode.nrows),
	  dCols(mode.ncols),
	  nFrames(mode.frames),
	  nADCs(mode.nadcs),
	  nDropFrames(mode.drop_frames),
	  readUpTheRamp(mode.read_up_the_ramp),
	  dataLabel(mode.label)
{
}

struct Transfer {
	DataCollector *collector;
	size_t count;
};

void
Camera::abort() {
	isAbort = true;
    if (!gIsDebug && dev->Command( TIM_ID, AEX ) != DON) {
	    throw std::runtime_error("Aborting exposure failed");
    }
}


void
Camera::expose(Controller* cont, float expTime, std::string basepath, std::string basename, CExpIFace* exp_iface, std::function<void(json_object* obj1, json_object* obj2)> processHeader)
{
	int msec = int( getExposureDelay(expTime) * 1000 );

	// Setting the exposure time
	if (!gIsDebug && dev->Command( TIM_ID, SET, msec ) != DON) {
		throw std::runtime_error("Set exposure time failed");
	}

	if (!gIsDebug && dev->Command( TIM_ID, SFS, nFrames * (1 + nDropFrames)) != DON) {
		throw std::runtime_error("Set number of fowlers failed");
	}

	if (!gIsDebug && dev->Command( TIM_ID, SDS, nADCs) != DON) {
		throw std::runtime_error("Set analog digital samples failed");
	}
	
	unsigned long long loops = 0;
	std::vector<std::thread *>threads;

	json_object *json_output = json_object_new_object();
	json_object *pdu = json_object_new_object();
	json_object *samples = json_object_new_array();
	json_object *timing = json_object_new_object();
	json_object *temperature = json_object_new_object();

	json_object_object_add(json_output, "PDU", pdu);
	json_object_object_add(json_output, "FRAMES", samples);
	json_object_object_add(json_output, "TIME_SAMPLES", timing);
	json_object_object_add(json_output, "TEMPERATURE", temperature);

	json_object_object_add(pdu, "CAMERA", json_object_new_string("GNIRS"));
	json_set_datalabel(pdu, "", nFrames, nADCs);
	json_object_object_add(pdu, "LNRS", json_object_new_int(nFrames));
	json_object_object_add(pdu, "NDAVGS", json_object_new_int(nADCs));
	json_object_object_add(pdu, "RAW_COLS", json_object_new_int(dCols));
	json_object_object_add(pdu, "RAW_ROWS", json_object_new_int(dRows * nFrames * 2));
	json_set_gmdate(pdu, "DATEOBS");

	Pixel* buffer = (gIsDebug ? (Pixel *) debugBuffer : (Pixel *) dev->CommonBufferVA());

	DataCollector *collectors[nFrames * 2];

	for (unsigned i = 0; i < (nFrames * 2); i++) {
		collectors[i] = new DataCollector(buffer, dRows * dCols, dCols, basepath, basename, i);

		json_object_array_add(samples, json_object_new_string(collectors[i]->getFileName().c_str()));
	}

	std::queue<Transfer> transfers;

	// Create the clock object just before starting the exposure (this will set the reference)
	Clock clock;

	// Start the exposure
	if (!gIsDebug && dev->Command( TIM_ID, SEX ) != DON) {
		throw std::runtime_error("Starting exposure failed");
	}

	std::cout << "ARC controller readout and exposure started\n";

	isAbort = false;

	int pixelsToReadPerFrame = dRows * dCols;
	if (gIsAladdinIII) 
		pixelsToReadPerFrame = (dRows + 1) * dCols;


	int lastPixelCount = 0;
	long totalPixelCount = 0;
	unsigned int i = 0;	
	unsigned int collector = 0;

	int totalFrames = nFrames * 2 * (1 + nDropFrames);

	if (gIsDebug) std::cout << "Total Frames: " << nFrames * (1 + nDropFrames) << " Total Fowlers/Save Frames: " << nFrames << " Drop: " << nDropFrames << "\n";

	while (i < totalFrames && !isAbort) {
		if (gIsDebug) std::cout << "Loop: " << i << " end: " << totalFrames << "\n";


		if (!gIsDebug) {
			while (lastPixelCount >= dev->GetPixelCount() && !isAbort) {
				lastPixelCount = dev->GetPixelCount();
			}
		}

		if (readUpTheRamp == true) {
			if (i == 0) {
				clock.set_timing_prefix("INTG_");
				clock.set_timing_index(0);
				clock.add_measurement(steady_clock::now());
			}
		}
		else {
			if (i == 0) {
				clock.set_timing_prefix("RSET_");
				clock.set_timing_index(0);
				clock.add_measurement(steady_clock::now());
			}
			else if (i == totalFrames / 2) {
				clock.set_timing_prefix("SGNL_");
				clock.set_timing_index(0);
				clock.add_measurement(steady_clock::now());
			}
		}
		
				
		if (!gIsDebug) {
			// wait until we read all data before moving on
			// check that we read enough data and that we haven't rolled off the end and started the next frame

			int currentPixelCount = lastPixelCount = dev->GetPixelCount();
			while (currentPixelCount < pixelsToReadPerFrame && currentPixelCount >= lastPixelCount && !isAbort) {
				lastPixelCount = currentPixelCount; 
				currentPixelCount = dev->GetPixelCount();
			}
		}
		else if (i == totalFrames / 2) {
			std::cout << "Simulate integration time. Busy-waiting for " << msec << " ms\n";

			auto start = std::chrono::high_resolution_clock::now();
			auto end = start;
			while (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() < msec && !isAbort) {
				end = std::chrono::high_resolution_clock::now();
			}

			std::cout << "Simulate integration time done.\n";
		}
		
		// save the data to file 
		if (i % (1 + nDropFrames) == 0) {
			if (gIsDebug) std::cout << "Save frame: " << i << "\n";

			clock.add_measurement(steady_clock::now());


			if (gIsDebug) std::cout << "Total collectors: " << nFrames * 2 << " Current collector: " << collector << "\n";

			collectors[collector]->update(pixelsToReadPerFrame);
			
			threads.push_back(new std::thread(&DataCollector::data_save, collectors[collector]));
			collector++;
		}

		lastPixelCount = pixelsToReadPerFrame;
		totalPixelCount += pixelsToReadPerFrame;

		i++;
	}



	std::cerr << "Total loops = " << loops << '\n';
	std::cerr << "Joining threads\n";
	for (auto t: threads) {
		t->join();
		delete t;
	}
	std::cerr << "Writing header\n";

	sty_time_point ut_start;// = clock.getMeasurementTime("RSET_", 0);
	sty_time_point ut_end;// = clock.getMeasurementTime("SGNL_", nFrames);

	if (readUpTheRamp) {
		ut_start = clock.getMeasurementTime("INTG_", 0);
		ut_end = clock.getMeasurementTime("INTG_", nFrames);
	}
	else {
		ut_start = clock.getMeasurementTime("RSET_", 0);
		ut_end = clock.getMeasurementTime("SGNL_", nFrames);
	}
	clock.json_set_gmtime(pdu, "UTSTART", ut_start); 
	clock.json_set_gmtime(pdu, "UTEND", ut_end);


	auto add_measurements = [timing](std::string label, double diff) { json_object_object_add(timing, label.c_str(), json_object_new_double(diff)); };

	clock.visit_measurements(add_measurements);

	double aveExposure=0;

	// sum exposure times at the time the last pixel of a readout is recieved
	for (unsigned int i = 1; i <= nFrames; i++ ) {
		if (readUpTheRamp == true) {
			aveExposure += clock.getMeasurementDelta("INTG_", i) - clock.getMeasurementDelta("INTG_", i-1);
		}
		else {
			aveExposure += clock.getMeasurementDelta("SGNL_", i) - clock.getMeasurementDelta("RSET_", i);
		}
	}

	aveExposure /= nFrames;

	json_object_object_add(pdu, "EXPTIME", json_object_new_double(aveExposure));
	json_object_object_add(pdu, "EXPREQ", json_object_new_double(expTime));
	json_object_object_add(pdu, "MIN_INT", json_object_new_double((cExpConst1 + cExpConst2 * nADCs) * nFrames));//getExposureOverhead(1, 1))); //function doesn't match time


	if (processHeader) {
		if (gIsDebug) std::cout << "Processing header\n";
		processHeader(temperature, pdu);


		std::size_t hyphen_pos = basename.rfind('-');
		if (hyphen_pos != std::string::npos) {
			basename = basename.substr(0, hyphen_pos);
		}

		std::ostringstream oss;
		oss << basepath + basename << ".header";


		if (gIsDebug) std::cout << "Header: " << basepath + basename << ".header\n";

		std::ofstream ofs(oss.str());

		if (!ofs) {
			std::cerr << "Failed to open file: " << oss.str() << std::endl;
		}

		ofs << json_object_to_json_string_ext(json_output, JSON_C_TO_STRING_PRETTY) << '\n';
		json_object_put(json_output);
	}


	for (unsigned i = 0; i < (nFrames * 2); i++) {
		delete collectors[i];
	}

}

void parse_cmd(int argc, char **argv, ReadoutConfig& mode, bool& reset, bool& debug)
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

			mode.coadds = std::stoi(argv[argi]);
			if (mode.coadds < 1) {
				std::cerr << "Number in coadds must be positive\n";
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


controllerInterface::controllerInterface() : exposureThread(NULL)  {


	reset = false;
	clockoutAll = false;
	clockoutsEnabled = false;

	readUpTheRamp = false;

    cout << "Allocating controller device \n";

	if (!gIsDebug) {
		this->allocController();

		this->connectDevice();

		this->listDevices();
	}	
}



controllerInterface::controllerInterface(std::string readoutPath, std::string lodPath) :  controllerInterface() {
    this->readoutPath = readoutPath;
    this->lodPath = lodPath;
}


controllerInterface::~controllerInterface() {
	delete gCont;
}

std::string controllerInterface::version() {
	return GIT_COMMIT;
}



void controllerInterface::allocController() {
	try {
		std::cout << "Allocating Controller Memory Buffer.\n";

		gCont = new Controller(ROWS_BUFFER, COLS_BUFFER);

	}
	catch (const std::exception& e) {
		std::cout << "Exception caught: " << e.what() << std::endl;
		throw;
	}
}

void controllerInterface::connectDevice(){
	try {
		std::cout << "Connecting to PCI Device.\n";

		gCont->connect_device();
	}
	catch (const std::exception& e) {
		std::cout << "Exception caught: " << e.what() << std::endl;
		throw;
	}
}

void controllerInterface::listDevices() {
	try {
		std::cout << "List of devices:\n";
		for (auto st: gCont->device_list()) {
			std::cout << "  " << st << '\n';
		}
	}
	catch (const std::exception& e) {
		std::cout << "Exception caught: " << e.what() << std::endl;
		throw;
	}
}

void controllerInterface::resetDevice() {
	try {
		std::cout << "Performing reset.\n";
		gCont->getDev()->Reset();

	}
	catch (std::runtime_error& e)
	{
		std::cerr << "Error setting up the controller: " << e.what() << "\n";
		throw;
	}
}

void controllerInterface::loadFirmware(std::string path) {
	try {
		// Check if file exists
		std::ifstream file(path);
		if (!file) {
			throw std::runtime_error("Error: LOD file does not exist.");
		}
		else {
			std::cout << "LOD file found.\n";
		}

		std::cout << "Loading LOD file.\n";
		gCont->setup_controller(path, true, reset);
	}
	catch (std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << "\n";
		throw; // Re-throwing to be handled by the caller
	}
}


int controllerInterface::init() {

	if (gIsDebug)
		return true;
		
	try {
		this->resetDevice();

		this->loadFirmware(this->lodPath);
	}
	catch (std::runtime_error& e)
	{
		std::cerr << "Error setting up the controller: " << e.what() << "\n";
		return false;
	}

	this->biasMed();

	return true;
}


void controllerInterface::setAladdinIII(bool isAladdinIII) {

	if (isAladdinIII) {
		printf("Firmware set to Aladdin III\n");
		mode.lod_file = aladdinIIIFilename;
		gIsAladdinIII = true;
	}
	else {
		mode.lod_file = aladdinIIFilename;
		printf("Firmware set to Aladdin II\n");
		gIsAladdinIII = false;
	}
}	


bool controllerInterface::testDataLink() {
	std::cout << "Testing Data Link \n"; 

	bool result = 1;

	cout << " debug: " << (gIsDebug? "TRUE" : "FALSE") << "\n";

	if (!gIsDebug)
		result = gCont->tdl_testing(123);
	
	std::cout << "TDL:  " << result << "\n";

	return result;
}


int controllerInterface::biasLow() {

	std::cout << "Well Depth set to -3.6 \n";

	currentBias = LOW;
	if (gCont->getDev()->Command( TIM_ID, SBH ) != DON) { ///NOTE: bias labels are backwards SBH is shallow well
		throw std::runtime_error("Set bias voltage");
	}

	return 0;
}

int controllerInterface::biasMed() {

	std::cout << "Well Depth set to -3.4 \n";

	currentBias = MEDIUM;
	if (gCont->getDev()->Command( TIM_ID, SBV ) != DON) {
		throw std::runtime_error("Set bias voltage");
	}

	return 0;
}

int controllerInterface::biasHigh() {

	std::cout << "Well Depth set to -3.2 \n";
   
	currentBias = HIGH;
	if (gCont->getDev()->Command( TIM_ID, SBL ) != DON) {   ///NOTE: bias labels are backwards SBL is deep well
		throw std::runtime_error("Set bias voltage");
	}

	return 0;
}

int controllerInterface::setExposure(double fowlerSamples, double adcSamples, double exposureTime, int coadds, int drop_frames, std::string datalabel) {

	if (fowlerSamples < 1) fowlerSamples = 1;
	if (drop_frames < 0) drop_frames = 0;
	if (adcSamples < 1) adcSamples = 1;
	if (exposureTime < 0) exposureTime = 0;
	if (coadds < 1) coadds = 1;


	mode.frames = fowlerSamples;
	mode.drop_frames = drop_frames;
	mode.nadcs = adcSamples;
	mode.exposure = exposureTime;
	mode.nrows = ROWS_PER_FRAME;
    mode.ncols = COLS_PER_FRAME * mode.nadcs; 
	mode.coadds = coadds;
	mode.read_up_the_ramp = this->readUpTheRamp;
	mode.label = datalabel;

	std::cout << "Fowler samples: " << mode.frames << " ADCs: " << mode.nadcs << " Exposure time: " << mode.exposure << " Drop frames: " << mode.drop_frames << std::endl;

	return 0;
}

int controllerInterface::startExposure(double temp1, double temp2, bool raw) {
	include_raw=raw;
	tempIN1 = temp1;
	tempIN2 = temp2;

	new std::thread(&controllerInterface::exposeFunct, this);

	return 0;
}

int controllerInterface::startExposureBlock(double temp1, double temp2, bool raw) {
	include_raw=raw;
	tempIN1 = temp1;
	tempIN2 = temp2;

	exposeFunct();

	return 0;
}



void controllerInterface::abortExposure() {
	std::cout << "Trying to abort\n";

	arc::device::CArcDevice* dev = nullptr;
	if (!gIsDebug) dev = gCont->getDev();

	Camera camera(dev, mode);

	try {
		camera.abort();	
	}
	catch (std::runtime_error& error) {
		std::cout << "Error: " << error.what() << std::endl;
		std::cout << "Will abort after current readout\n";
	}

}

double controllerInterface::getExposureDelay(double requestedExpTime, int num_fowlers, int num_adc) {
		return std::max((double)0, requestedExpTime + (cExpConst1 + cExpConst2 * num_adc) * num_fowlers);
}


void controllerInterface::setNumClockouts(int nClockouts) {
        if (nClockouts >= 0) {
        
        	if (gCont->getDev()->Command( TIM_ID, SNC, nClockouts) != DON) {
        		throw std::runtime_error("Set number of clockouts failed");
        	}
	}
	else {
		std::cout << "Number of clockouts must be greater than or equal to 0\n" << std::endl;
        }
}

void controllerInterface::clockoutArray() {
        if (gCont->getDev()->Command( TIM_ID, COA ) != DON) {
                throw std::runtime_error("Clockout array failed");
        }
	std::cout << "Clockout array command sent\n";

}

void controllerInterface::continuousClockoutsStart() {
	clockoutsStop = false;

	new std::thread(&controllerInterface::clockoutFunct, this);
}


void controllerInterface::clockoutFunct() {

	while (!clockoutsStop) {
		clockoutMutex.lock();
			
		std::cout << "Continuous clockout\n";
	
		clockoutArray();
	
		clockoutMutex.unlock();
	}

}

void controllerInterface::resetArray() {
        if (gCont->getDev()->Command( TIM_ID, RAR ) != DON) {
                throw std::runtime_error("Reset array failed");
        }
	std::cout << "Reset array command sent\n";

}

void controllerInterface::resetReadArray() {
        if (gCont->getDev()->Command( TIM_ID, RRO ) != DON) {
                throw std::runtime_error("Reset read array failed");
        }
	std::cout << "Reset read array command sent\n";

}

void controllerInterface::readoutArray() {
        if (gCont->getDev()->Command( TIM_ID, ROR ) != DON) {
                throw std::runtime_error("Readout array failed");
        }
	std::cout << "Readout array command sent\n";

}



void controllerInterface::exposeFunct() {
	if (busyMutex.try_lock()) {
		
		std::cout << "Locking mutex\n";

		expose();


		std::cout << "-------- Exposure complete --------  Unlocking mutex\n";
		busyMutex.unlock();
	}
	else {
	 	std::cout << "Exposure currently in progress...aborting this exposure request\n";
	}
}

int controllerInterface::expose() {

	this->setReadingOut(true);

	std::cout << "coadds " << mode.coadds << std::endl;

	std::function<void(json_object*, json_object*)> processHeader = [this](json_object* temp, json_object* pdu){

		json_object_object_add(temp, "TEMP IN1", json_object_new_double(this->tempIN1));
		json_object_object_add(temp, "TEMP IN2", json_object_new_double(this->tempIN2));
			
		double biasVolts = 0;	
		switch (this->currentBias) {
			case LOW: biasVolts = -3.6;break;
			case MEDIUM: biasVolts = -3.4;break;
			case HIGH: biasVolts = -3.2;break;
		}

		json_object_object_add(pdu, "LABEL", json_object_new_string(mode.label.c_str()));
		json_object_object_add(pdu, "DETBIAS", json_object_new_double(-4.0 - biasVolts));
		json_object_object_add(pdu, "DCVER", json_object_new_string(GIT_COMMIT));
		json_object_object_add(pdu, "COADDS", json_object_new_int(mode.coadds));

		if (this->include_raw)
			json_object_object_add(pdu, "P_MODE", json_object_new_string("SEP"));
		else
			json_object_object_add(pdu, "P_MODE", json_object_new_string("STARE"));
	};


	if (!gIsDebug) {
		try {	
			if (gIsAladdinIII) {
				gCont->set_size(mode.nrows + 1, mode.ncols);
			}
			else {
				gCont->set_size(mode.nrows, mode.ncols);
			};
		}
		catch (std::runtime_error& error) {
			std::cout << "Error: " << error.what() << std::endl;
			std::cout << "ARC currently reading out. Exiting this exposure. \n";
			return -1;	
		}	

		std::cout << "Data Label: " << mode.label << '\n';
	}

	std::cout << "Exposing for " << mode.exposure << " seconds\n";

	if (gIsDebug)
		std::cout << "Exposing\n";

	ExpIFace callbacks(gIsDebug);
	if (gIsDebug) std::cout << "Debug 1 \n";

	arc::device::CArcDevice* dev = nullptr;
	if (!gIsDebug) dev = gCont->getDev();

	Camera camera(dev, mode);

	if (gIsDebug) std::cout << "Debug 2 \n";

	Camera::isAbort = false; //just incase abort is pressed while not in an exposure

	clockoutMutex.lock(); // wait for continuous clockout to stop


	std::string uuid = get_uuid();

    for (int i=0; i < mode.coadds; i++) {

		if (Camera::isAbort) break;

		
		if (clockoutsEnabled && (clockoutAll || i == 0)) {
			std::cout << "Clock through array to reduce first frame effect" << std::endl;
			clockoutArray();
		}

		std::string path = std::string(this->readoutPath) + "/new/";

		camera.expose(gCont, mode.exposure, path, uuid + "-" + std::to_string(i), &callbacks, (i == mode.coadds -1 ? processHeader : nullptr));
	}

	clockoutMutex.unlock();

	Camera::isAbort = false;


	this->setReadingOut(false);

	std::cout << "Exposure complete\n";
	
	return 0;

}
