#ifndef __LIB_GNIRS_IOC__
#define __LIB_GNIRS_IOC__

#include <string>
#include <thread>
#include <mutex>
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>


using namespace std; 

#define ALADDINII_LOD_NAME 	"AladdinII_SDSU_Firmware.lod"
#define ALADDINIII_LOD_NAME "AladdinIII_SDSU_Firmware.lod"

typedef enum BiasLevel { LOW, MEDIUM, HIGH } BiasLevel;

extern bool gIsDebug;
void setGlobalDebug(bool debug);

struct ReadoutConfig {
	std::string label;
	std::string lod_file;
	unsigned nrows;
	unsigned ncols;
	unsigned nadcs;
	unsigned frames;
	unsigned drop_frames;
	float exposure;
    char wellDepth;
	int coadds;
	bool read_up_the_ramp;
};

namespace py = pybind11;

class controllerInterface {
public:
	controllerInterface();
	controllerInterface(std::string readoutPath, std::string lodPath);
	~controllerInterface();

	void allocController();
	void connectDevice();
	void listDevices();
	void resetDevice();
	void loadFirmware(std::string lodPath);


	int init();
	bool testDataLink();
	int biasLow();
	int biasMed();
	int biasHigh();
	int setExposure(double fowlserSamples, double adcSamples, double exposureTime, int coadds, int drop_frames, std::string datalabel);
	void setAladdinIII(bool isAladdinIII);
    int startExposure(double temp1, double temp2, bool raw);	
	int startExposureBlock(double temp1, double temp2, bool raw); 
	void abortExposure();
	double getExposureDelay(double requestedExpTime, int num_fowlers, int num_adc);

	void setReadingOut(bool readingState) { readingOut = readingState; };
	bool getReadingOut() { return readingOut; };

	void setReadUpTheRamp(bool mode) {
		readUpTheRamp = mode;
	}


    void setClockoutAll(bool isAll) {
		clockoutAll = isAll;
	}
    void setNumClockouts(int clockouts);
	void clockoutArray();
	void continuousClockoutsStart();
	void continuousClockoutsStop() { clockoutsStop = true; }
	void enableClockouts(bool status) { clockoutsEnabled = status; }


	void resetArray();
	void resetReadArray();
	void readoutArray();

    std::mutex busyMutex;

    std::mutex clockoutMutex;

	ReadoutConfig mode{"defaul-label", ALADDINII_LOD_NAME, 512, 2048, 1, 1, 0, 0.0, 'M', 1};

	std::string version();



private:
	void exposeFunct();
	int expose();
	std::string aladdinIIFilename{ALADDINII_LOD_NAME};
	std::string aladdinIIIFilename{ALADDINIII_LOD_NAME};
	std::string readoutPath{"/readout_data/"};
	std::string lodPath{"./firmware/"};
	bool reset;
//	bool debug;

	bool readUpTheRamp;

	void clockoutFunct();
	bool clockoutAll;
	bool clockoutsStop;
	bool clockoutsEnabled;

	BiasLevel currentBias = MEDIUM;

	double tempIN1;
	double tempIN2;

	bool include_raw;

	bool readingOut;

	std::thread* exposureThread;
};


/**
 * @class controllerInterfaceDebug
 * @brief This class provides an interface for controlling and debugging various functionalities.
 *
 * It includes methods for initialization, bias configuration, exposure settings, 
 * clocking out, resetting, reading, and handling specific instruments such as Aladdin III.
 * Logging is done through standard output for tracking the execution of different operations.
 */

class controllerInterfaceDebug {
public:
	controllerInterfaceDebug()						{cout << "controllerInterface constructor" << endl;}
	controllerInterfaceDebug(std::string readoutPath, std::string lodPath) {cout << "controllerInterface constructor" << endl;}
	~controllerInterfaceDebug()						{cout << "controllerInterface destructor" << endl;}
	int init()  									{cout << "init" << endl;return 0;}
	bool testDataLink() 							{cout << "TDL" << endl;return 0;}
	int biasLow() 									{cout << "biasLow" << endl;return 0;}
	int biasMed() 									{cout << "biasMed" << endl;return 0;}
	int biasHigh() 									{cout << "biasHigh" << endl;return 0;}
	int setExposure(double fowlserSamples, double adcSamples, double exposureTime, int coadds, int drop_frames, std::string datalabel){cout << "setExposure lnr: " << fowlserSamples << " adc: " << adcSamples << " exposureTime: " << exposureTime << " coadds: " << coadds << " drop frames: " << drop_frames << endl;return 0;}
	void setAladdinIII(bool isAladdinIII)			{cout << "setAladdinIII" << endl;}
    int startExposure(double temp1, double temp2, bool raw) 	{cout << "startExposure" << endl;return 0;}
	int startExposureBlock(double temp1, double temp2, bool raw) {cout << "startExposureBlock" << endl;return 0;}
	void abortExposure() 							{cout << "abortExposure" << endl; }
	bool getReadingOut()							{cout << "get reading out" << endl; return true; }
	void setClockoutAll(bool isAll) 				{cout << "setClockoutAll" << endl;}
    void setNumClockouts(int clockouts) 			{cout << "setNumClockouts" << endl;}
	void enableClockouts(bool status) 			    {cout << "enableClockouts" << endl;}
	void clockoutArray() 							{cout << "clockoutArray" << endl;}
	void continuousClockoutsStart() 				{cout << "continuousClockoutsStart" << endl;}
	void continuousClockoutsStop() 					{cout << "continuousClockoutsStop" << endl;}
	void resetArray() 								{cout << "resetArray" << endl; }
	void resetReadArray() 							{cout << "resetReadArray" << endl; }
	void readoutArray() 							{cout << "readoutArray" << endl; }
};


PYBIND11_MODULE(libgnirsioc, m) {
	py::enum_<BiasLevel>(m, "BiasLevel")
		.value("LOW", BiasLevel::LOW)
		.value("MEDIUM", BiasLevel::MEDIUM)
		.value("HIGH", BiasLevel::HIGH);

    // Expose the setGlobalDebug function
    m.def("setGlobalDebug", &setGlobalDebug, "Set the global debug flag");

	py::class_<ReadoutConfig>(m, "ReadoutConfig")
		.def(py::init<>())
		.def_readwrite("label", &ReadoutConfig::label)
		.def_readwrite("lod_file", &ReadoutConfig::lod_file)
		.def_readwrite("nrows", &ReadoutConfig::nrows)
		.def_readwrite("ncols", &ReadoutConfig::ncols)
		.def_readwrite("nadcs", &ReadoutConfig::nadcs)
		.def_readwrite("frames", &ReadoutConfig::frames)
		.def_readwrite("drop_frames", &ReadoutConfig::drop_frames)
		.def_readwrite("exposure", &ReadoutConfig::exposure)
		.def_readwrite("wellDepth", &ReadoutConfig::wellDepth)
		.def_readwrite("coadds", &ReadoutConfig::coadds);
		

	py::class_<controllerInterface>(m, "controllerInterface")
        .def(py::init<>()) 							// Default constructor
        .def(py::init<std::string, std::string>()) 	// Overloaded constructor
		.def("version", &controllerInterface::version)
		.def("allocController", &controllerInterface::allocController)
		.def("connectDevice", &controllerInterface::connectDevice)
		.def("listDevices", &controllerInterface::listDevices)
		.def("resetDevice", &controllerInterface::resetDevice)
		.def("loadFirmware", &controllerInterface::loadFirmware)
		.def("init", &controllerInterface::init)
		.def("testDataLink", &controllerInterface::testDataLink)
		.def("biasLow", &controllerInterface::biasLow)
		.def("biasMed", &controllerInterface::biasMed)
		.def("biasHigh", &controllerInterface::biasHigh)
		.def("setExposure", &controllerInterface::setExposure)
		.def("setAladdinIII", &controllerInterface::setAladdinIII)
		.def("startExposure", &controllerInterface::startExposure)
		.def("startExposureBlock", &controllerInterface::startExposureBlock)
		.def("getReadingOut", &controllerInterface::getReadingOut)
		.def("abortExposure", &controllerInterface::abortExposure)
		.def("getExposureDelay", &controllerInterface::getExposureDelay)
		.def("setClockoutAll", &controllerInterface::setClockoutAll)
		.def("setNumClockouts", &controllerInterface::setNumClockouts)
		.def("enableClockouts", &controllerInterface::enableClockouts)
		.def("clockoutArray", &controllerInterface::clockoutArray)
		.def("continuousClockoutsStart", &controllerInterface::continuousClockoutsStart)
		.def("continuousClockoutsStop", &controllerInterface::continuousClockoutsStop)
		.def("resetArray", &controllerInterface::resetArray)
		.def("resetReadArray", &controllerInterface::resetReadArray)
		.def("readoutArray", &controllerInterface::readoutArray)
		.def("setReadUpTheRamp", &controllerInterface::setReadUpTheRamp);



	py::class_<controllerInterfaceDebug>(m, "controllerInterfaceDebug")
		.def(py::init<>())							// Default constructor
	    .def(py::init<std::string, std::string>()) 	// Overloaded constructor
		.def("init", &controllerInterfaceDebug::init)
		.def("testDataLink", &controllerInterfaceDebug::testDataLink)
		.def("biasLow", &controllerInterfaceDebug::biasLow)
		.def("biasMed", &controllerInterfaceDebug::biasMed)
		.def("biasHigh", &controllerInterfaceDebug::biasHigh)
		.def("setExposure", &controllerInterfaceDebug::setExposure)
		.def("setAladdinIII", &controllerInterfaceDebug::setAladdinIII)
		.def("startExposure", &controllerInterfaceDebug::startExposure)
		.def("startExposureBlock", &controllerInterfaceDebug::startExposureBlock)
		.def("getReadingOut", &controllerInterfaceDebug::getReadingOut)
		.def("abortExposure", &controllerInterfaceDebug::abortExposure)
		.def("setClockoutAll", &controllerInterfaceDebug::setClockoutAll)
		.def("setNumClockouts", &controllerInterfaceDebug::setNumClockouts)
		.def("enableClockouts", &controllerInterfaceDebug::enableClockouts)
		.def("clockoutArray", &controllerInterfaceDebug::clockoutArray)
		.def("continuousClockoutsStart", &controllerInterfaceDebug::continuousClockoutsStart)
		.def("continuousClockoutsStop", &controllerInterfaceDebug::continuousClockoutsStop)
		.def("resetArray", &controllerInterfaceDebug::resetArray)
		.def("resetReadArray", &controllerInterfaceDebug::resetReadArray)
		.def("readoutArray", &controllerInterfaceDebug::readoutArray);
		
}

#endif // __LIB_GNIRS_IOC__
 

