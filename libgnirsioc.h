#ifndef __LIB_GNIRS_IOC__
#define __LIB_GNIRS_IOC__

#include <string>


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

class controllerInterface {
public:
	controllerInterface();
	~controllerInterface();
	int init();
	int biasLow();
	int biasMed();
	int biasHigh();
	int setExposure(double fowlserSamples, double adcSamples, double exposureTime);
	int expose();


	Config mode{"", "/gem_test/gnirsdc/lib/DSP/Aladdin_SDSU_Firmware.lod", 512, 2048, 1, 1, 0.0, 'M', 1};

private:
	bool reset;
	bool debug;
};




#endif // __LIB_GNIRS_IOC__
 

