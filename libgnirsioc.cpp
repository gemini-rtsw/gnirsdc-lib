
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



Controller *gCont;

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

	double getMeasurementDelta(std::string prefix, int index) {
		return double((measurements[prefix + left_justify(to_string(index), 2, '0')] - sty_clock_ref).count()) / 1000000000;
	}

	sty_time_point getMeasurementTime(std::string prefix, int index) {
		return measurements[prefix + left_justify(to_string(index), 2, '0')];
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
	json_object_object_add(job, "DATALABE", json_object_new_string(name.c_str()));
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
	  nADCs(mode.nadcs)

{
}

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
	fs.write((const char *)buffer, buffSize * sizeof(Pixel));
	std::cerr << "saved buffer to file " << fileName << '\n';
}

struct Transfer {
	DataCollector *collector;
	size_t count;
};

void
Camera::abort() {
	isAbort = true;
        if (dev->Command( TIM_ID, AEX ) != DON) {
                throw std::runtime_error("Aborting exposure failed");
        }
}


void
Camera::expose(Controller* cont, float expTime, std::string basepath, std::string basename, CExpIFace* exp_iface, std::function<void(json_object* obj1, json_object* obj2)> processHeader)
{
	int msec = int( getExposureDelay(expTime) * 1000 );
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
	json_set_datalabel(pdu, "test-image", nFrames, nADCs);
	json_object_object_add(pdu, "LNRS", json_object_new_int(nFrames));
	json_object_object_add(pdu, "NDAVGS", json_object_new_int(nADCs));
	json_object_object_add(pdu, "RAW_COLS", json_object_new_int(dCols));
	json_object_object_add(pdu, "RAW_ROWS", json_object_new_int(dRows * nFrames * 2));
	json_set_gmdate(pdu, "DATEOBS");


	DataCollector *collectors[nFrames * 2];
	for (unsigned i = 0; i < (nFrames * 2); i++) {
		collectors[i] = new DataCollector((Pixel *)dev->CommonBufferVA(), dRows * dCols, dCols, basepath, basename, i);
		json_object_array_add(samples, json_object_new_string(collectors[i]->getFileName().c_str()));
	}

	std::queue<Transfer> transfers;

	// Create the clock object just before starting the exposure (this will set the reference)
	Clock clock;

	// Start the exposure
	if (dev->Command( TIM_ID, SEX ) != DON) {
		throw std::runtime_error("Starting exposure failed");
	}


//	auto ut_start = steady_clock::now();

	std::cout << "ARC controller readout and exposure started\n";

	isAbort = false;

	int pixelsToReadPerFrame = dRows * dCols;
	if (gIsAladdinIII) 
		pixelsToReadPerFrame = (dRows + 1) * dCols;


	int lastPixelCount = 0;
	long totalPixelCount = 0;
	unsigned int i = 0;	
	while (i < nFrames * 2 && !isAbort) {

	//	std::cout << "Waiting for next readout to start last count: " << lastPixelCount << " pixels read: " << dev->GetPixelCount() << "\n";
	//	if (i == nFrames) std::cout << "Exposing\n";

//std::cout << "[ " << dev->GetPixelCount() << "]" << i << std::endl;
		while (lastPixelCount >= dev->GetPixelCount() && !isAbort) {
			lastPixelCount = dev->GetPixelCount();
		}
//std::cout << "[ " << dev->GetPixelCount() << "]" << i << std::endl;

		if (i == 0) {
			clock.set_timing_prefix("RESET_");
			clock.set_timing_index(0);
			clock.add_measurement(steady_clock::now());
		}
		else if (i == nFrames) {
			clock.set_timing_prefix("SIGNAL_");
			clock.set_timing_index(0);
			clock.add_measurement(steady_clock::now());
		}
			
		
//		std::cout << "---------- Reading out Fowler sample: " << i + 1 << " ----------------" << std::endl;

//std::cout << "[ " << dev->GetPixelCount() << "]" << i << std::endl;

		// wait until we read all data before moving on
		// check that we read enough data and that we haven't rolled off the end and started the next frame
		int currentPixelCount = lastPixelCount = dev->GetPixelCount();
		while (currentPixelCount < pixelsToReadPerFrame && currentPixelCount >= lastPixelCount && !isAbort) {
			lastPixelCount = currentPixelCount; 
			currentPixelCount = dev->GetPixelCount();
		}
//std::cout << "[ " << dev->GetPixelCount() << "]" << i << std::endl;

		clock.add_measurement(steady_clock::now());


//		std::cout << "Readout: " << i << " complete\n";

		collectors[i]->update(pixelsToReadPerFrame);
		threads.push_back(new std::thread(&DataCollector::data_save, collectors[i]));

		lastPixelCount = pixelsToReadPerFrame;
		totalPixelCount += pixelsToReadPerFrame;

//		std::cout << "Total pixels read: "  << totalPixelCount << std::endl;
		i++;
	}


//	auto ut_end = steady_clock::now();

	std::cerr << "Total loops = " << loops << '\n';
	std::cerr << "Joining threads\n";
	for (auto t: threads) {
		t->join();
		delete t;
	}
	std::cerr << "Writing header\n";

	auto ut_start = clock.getMeasurementTime("RESET_", 0);
	auto ut_end = clock.getMeasurementTime("SIGNAL_", nFrames);
	clock.json_set_gmtime(pdu, "UTSTART", ut_start); 
	clock.json_set_gmtime(pdu, "UTEND", ut_end);

//	auto realExpTime = std::chrono::duration_cast<seconds>(ut_end - ut_start); 
//	std::chrono::duration<double> realExpTime = ut_end - ut_start;

	auto add_measurements = [timing](std::string label, double diff) { json_object_object_add(timing, label.c_str(), json_object_new_double(diff)); };

	clock.visit_measurements(add_measurements);

	double aveExposure=0;

	// sum exposure times at the time the last pixel of a readout is recieved
	for (unsigned int i = 1; i <= nFrames; i++ ) {
		aveExposure += clock.getMeasurementDelta("SIGNAL_", i) - clock.getMeasurementDelta("RESET_", i);
	}
	aveExposure /= nFrames;

	json_object_object_add(pdu, "EXPTIME", json_object_new_double(aveExposure));
	json_object_object_add(pdu, "EXPREQ", json_object_new_double(expTime));
	json_object_object_add(pdu, "MIN_INT", json_object_new_double((cExpConst1 + cExpConst2 * nADCs) * nFrames));//getExposureOverhead(1, 1))); //function doesn't match time


	processHeader(temperature, pdu);//bias_voltage, p_mode);


	std::ostringstream oss;
	oss << basepath + basename << ".header";
	std::ofstream ofs(oss.str());

	ofs << json_object_to_json_string_ext(json_output, JSON_C_TO_STRING_PRETTY) << '\n';
	json_object_put(json_output);

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


controllerInterface::controllerInterface() : exposureThread(NULL)  {


 	gCont = new Controller(512, 12288);

	reset = false;
	debug = false;
	clockoutAll = false;

	//include_raw = false;
	std::cout << "Connecting to Arc Controller \n";

	try {
		gCont->connect_device();

		std::cout << "List of devices:\n";
		for (auto st: gCont->device_list()) {
			std::cout << "  " << st << '\n';
		}

		std::cout << "TDL testing: " << gCont->tdl_testing(123) << '\n';
	}
	catch (const std::exception& e) {
		std::cout << "Exception caught: " << e.what() << std::endl;
		throw;
	}

}

controllerInterface::controllerInterface(std::string readoutPath, std::string lodPath) : controllerInterface() {
	this->readoutPath = readoutPath;
	this->lodPath = lodPath;	
}


controllerInterface::~controllerInterface() {
	delete gCont;
}

std::string controllerInterface::version() {
	return GIT_COMMIT;
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

int controllerInterface::init() {

	std::string fullPath = this->lodPath + "/" + mode.lod_file;

	std::cout << "Setting up with file: " << fullPath << '\n';


    // Check if file exists
	std::ifstream file(fullPath);
	if (!file) {
		std::cout << "Error: LOD file does not exist.\n";
		return -1; // or handle the error as needed
	}
	else {
		std::cout << "LOD file found.\n";
	}

	std::cout << "Performing reset.\n";
	gCont->getDev()->Reset();

	gCont->setup_controller(fullPath, true, reset); // Power on

	biasMed();

	return 0;
}

bool controllerInterface::testDataLink() {
	std::cout << "Testing Data Link"; 
	
	bool result = gCont->tdl_testing(123);
	
	std::cout << "TDL: " << result << "\n";

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

int controllerInterface::setExposure(double fowlerSamples, double adcSamples, double exposureTime, int sequence) {

	if (fowlerSamples < 1) fowlerSamples = 1;
	if (adcSamples < 1) adcSamples = 1;
	if (exposureTime < 0) exposureTime = 0;
	if (sequence < 1) sequence = 1;


	mode.frames = fowlerSamples;
	mode.nadcs = adcSamples;
	mode.exposure = exposureTime;
	mode.nrows = ROWS_PER_FRAME;
        mode.ncols = COLS_PER_FRAME * mode.nadcs; 
	mode.sequence = sequence;

	std::cout << "Fowler samples: " << mode.frames << " ADCs: " << mode.nadcs << " Exposure time: " << mode.exposure << std::endl;

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
	Camera camera(gCont->getDev(), mode);


	try {
		camera.abort();	
	}
	catch (std::runtime_error& error) {
		std::cout << "Error: " << error.what() << std::endl;
		std::cout << "Will abort after current readout\n";
	}

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
        std::cout << "Sequence " << mode.sequence << std::endl;

	auto processHeader = [this](json_object* temp, json_object* pdu){//json_object* bias, json_object* pmode){

                        json_object_object_add(temp, "TEMP IN1", json_object_new_double(this->tempIN1));
                        json_object_object_add(temp, "TEMP IN2", json_object_new_double(this->tempIN2));
		
			double biasVolts = 0;	
			switch (this->currentBias) {
				case LOW: biasVolts = -3.6;break;
				case MEDIUM: biasVolts = -3.4;break;
				case HIGH: biasVolts = -3.2;break;
			}
             //           json_object_object_add(pdu, "VDET", json_object_new_double(biasVolts));
              //          json_object_object_add(pdu, "VDDUC", json_object_new_double(-4.0));
                        json_object_object_add(pdu, "DETBIAS", json_object_new_double(-4.0 - biasVolts));
                        json_object_object_add(pdu, "DCVER", json_object_new_string(GIT_COMMIT));

			if (this->include_raw)
	                        json_object_object_add(pdu, "P_MODE", json_object_new_string("SEP"));
			else
	                        json_object_object_add(pdu, "P_MODE", json_object_new_string("STARE"));
                };

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

	if (debug) {
		std::cout << "Testing for mode: " << mode.label << '\n';
	}
	std::cout << "Exposing for " << mode.exposure << " seconds\n";

	if (debug)
		std::cout << "Exposing\n";

	ExpIFace callbacks(debug);

	Camera camera(gCont->getDev(), mode);



	Camera::isAbort = false; //just incase abort is pressed while not in an exposure

        std::cout << "Clock through array to reduce first frame effect" << std::endl;

	clockoutMutex.lock(); // wait for continuous clockout to stop

    for (int i=0; i < mode.sequence; i++) {

		if (Camera::isAbort) break;

                if (clockoutAll || i == 0) {
			clockoutArray();
		}

		std::string path = std::string(this->readoutPath) + "/new/";
		camera.expose(gCont, mode.exposure, path, get_uuid(), &callbacks, processHeader);
	}

	clockoutMutex.unlock();

	Camera::isAbort = false;


	std::cout << "Exposure complete\n";

//        gCont->save_to("/home/hstecher/fits/test.fits");
	
	return 0;

}
