#ifndef __LIB_GNIRS_IOC__
#define __LIB_GNIRS_IOC__

#include <string>
#include <thread>
#include <mutex>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

typedef enum BiasLevel { LOW, MEDIUM, HIGH } BiasLevel;

struct ReadoutConfig {
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

namespace py = pybind11;

class controllerInterface {
public:
	controllerInterface();
	~controllerInterface();
	int init();
	int biasLow();
	int biasMed();
	int biasHigh();
	int setExposure(double fowlserSamples, double adcSamples, double exposureTime, int sequence);
	void setAladdinIII(bool isAladdinIII);
        int startExposure(double temp1, double temp2, bool raw);	
	int startExposureBlock(double temp1, double temp2, bool raw); 
	void abortExposure();


        void setClockoutAll(bool isAll) {
		clockoutAll = isAll;
	}
        void setNumClockouts(int clockouts);
	void clockoutArray();
	void continuousClockoutsStart();
	void continuousClockoutsStop() {
		clockoutsStop = true;
	}


	void resetArray();
	void resetReadArray();
	void readoutArray();

        std::mutex busyMutex;

        std::mutex clockoutMutex;

	ReadoutConfig mode{"", "/gem_base/epics/ioc/gnirsdc-dsp/gnirsdc-firmware/AladdinII_SDSU_Firmware.lod", 512, 2048, 1, 1, 0.0, 'M', 1};
	std::string version();


private:
	void exposeFunct();
	int expose();
	std::string aladdinIIFilename{"/gem_base/epics/ioc/gnirsdc-dsp/gnirsdc-firmware/laddinII_SDSU_Firmware.lod"};
	std::string aladdinIIIFilename{"/gem_base/epics/ioc/gnirsdc-dsp/gnirsdc-firmware/AladdinIII_SDSU_Firmware.lod"};
	bool reset;
	bool debug;

	void clockoutFunct();
	bool clockoutAll;
	bool clockoutsStop;

	BiasLevel currentBias = MEDIUM;

	double tempIN1;
	double tempIN2;

	bool include_raw;

	std::thread* exposureThread;
};


PYBIND11_MODULE(libgnirsioc, m) {
	py::enum_<BiasLevel>(m, "BiasLevel")
		.value("LOW", BiasLevel::LOW)
		.value("MEDIUM", BiasLevel::MEDIUM)
		.value("HIGH", BiasLevel::HIGH);

	py::class_<ReadoutConfig>(m, "ReadoutConfig")
		.def(py::init<>())
		.def_readwrite("label", &ReadoutConfig::label)
		.def_readwrite("lod_file", &ReadoutConfig::lod_file);
		// ... continue with other members ...
		

	py::class_<controllerInterface>(m, "controllerInterface")
		.def(py::init<>())
		.def("init", &controllerInterface::init)
		.def("biasLow", &controllerInterface::biasLow);
		// ... continue with other methods ...
		
}

#endif // __LIB_GNIRS_IOC__
 

