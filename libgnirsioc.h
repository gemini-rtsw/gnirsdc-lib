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
	int expose(double temp1, double temp2);
	void exposeFunct() const;
	void setAladdinIII(bool isAladdinIII);
	

	Config mode{"", "/gem_test/gnirsdc/lib/DSP/AladdinII_SDSU_Firmware.lod", 512, 2048, 1, 1, 0.0, 'M', 1};

private:
	std::string aladdinIIFilename{"/gem_test/gnirsdc/lib/DSP/AladdinII_SDSU_Firmware.lod"};
	std::string aladdinIIIFilename{"/gem_test/gnirsdc/lib/DSP/AladdinIII_SDSU_Firmware.lod"};
	bool reset;
	bool debug;

	double tempIN1;
	double tempIN2;
};




#endif // __LIB_GNIRS_IOC__
 

