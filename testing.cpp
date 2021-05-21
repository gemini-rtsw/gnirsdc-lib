#include <iostream>
#include <unordered_map>
#include <string>
#include <unistd.h>
#include "libgnirs.h"
#include <CExpIFace.h>

struct Config {
	std::string label;
	std::string lod_file;
	int nrows;
	int ncols;
	float exposure;
};

static const auto READ_TIMEOUT = 200;
const int SDSU3_PON_BIT = 0x400000;

std::unordered_map<std::string, Config> setups{
	{"ffvf", {"Very Faint Object - Full frame", "./DSP/VeryFaintFullFrame.lod", 32768, 3072 * 4}},
	{"ffvb", {"Very Bright Object - Full frame", "./DSP/VeryBrightObjectFullFrame.lod", 1024, 2048}}
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
		  << "   testing [-h] [-d] [-r] [-e seconds] <mode>\n\n"
		  << " -h     shows this help page\n"
		  << " -d     increased debugging output\n"
		  << " -r     resets the controller as part of the setup\n"
		  << " -e <s> expose por <s> seconds\n"
		  << " <mode> needs to be one of the following:\n\n"
		  << "     ffvb   Very Bright Object (Full Frame)\n"
		  << "     ffvf   Very Faint Object (Full Frame)\n";
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

void custom_expose(CArcDevice* dev, float expTime, int dRows, int dCols, CExpIFace* exp_iface=nullptr)
{
	int msec = int( expTime * 1000 );
	ExposurePhase status = ExposurePhase::FIRST_READOUT;

	// Setting the exposure time
	if (dev->Command( TIM_ID, SET, msec ) != DON) {
		throw std::runtime_error("Set exposure time failed");
	}

	// Start the exposure
	if (dev->Command( TIM_ID, SEX ) != DON) {
		throw std::runtime_error("Starting exposure failed");
	}

	const int totalCount = dRows * dCols;
	const int exposeTimeout = (int(expTime * 1000) / 25) + 20; // Exposure time + 0.5s, in 25 millisecond ticks
	int pixelCount = 0;
	int timeoutCounter = 0;
	float remainingTime = expTime;
	int exposingCount = 0;
	while ( pixelCount < totalCount ) {
		bool readingOut = dev->IsReadout();
		bool exposing = (status == ExposurePhase::FIRST_READOUT) && (pixelCount == (totalCount / 2));
		int lastPixelCount = pixelCount;

		switch (status) {
			case ExposurePhase::FIRST_READOUT:
			case ExposurePhase::SECOND_READOUT:
				pixelCount = dev->GetPixelCount();
				if (exposing) {
					if (!readingOut) {
						timeoutCounter = 0;
						status = ExposurePhase::EXPOSING;
						continue;
					}

					if (exp_iface != nullptr)
						exp_iface->ExposeCallback(float(exposingCount * 25) / 1000);

					exposingCount++;
				}

				if (!exposing && (exp_iface != nullptr))
					exp_iface->ReadCallback(pixelCount);

				if (dev->ContainsError(pixelCount)) {
					dev->StopExposure();
					throw std::runtime_error("Failed to read pixel count");
				}


				if (!exposing) {
					timeoutCounter = (pixelCount == lastPixelCount) ? (timeoutCounter + 1) : 0;

					if (timeoutCounter >= 800) { // 20s * 40 slices
						dev->StopExposure();
						throw std::runtime_error("Read timeout");
					}
				}
				break;
			case ExposurePhase::EXPOSING:
				if (readingOut) {
					if (exp_iface != nullptr)
						exp_iface->ExposeCallback(expTime);
					timeoutCounter = 0;
					status = ExposurePhase::SECOND_READOUT;
					continue;
				}
				if (remainingTime > 0.0) {
					int ret = dev->Command( TIM_ID, RET );

					if (ret != ROUT) {
						if (dev->ContainsError(ret) || dev->ContainsError(ret, 0, msec)) {
							dev->StopExposure();
							throw std::runtime_error("Failed to read elapsed time");
						}

						float elapsedTime = (float(ret) / 1000.0);
						remainingTime = expTime - elapsedTime;
						if (exp_iface != nullptr)
							exp_iface->ExposeCallback(elapsedTime);

					}
				}
				if ((++timeoutCounter) > exposeTimeout) {
					dev->StopExposure();
					throw std::runtime_error("Timeout while exposing");
				}
				break;

		}



		usleep( 25000 ); // Sleep for 25ms
	}

	dev->StopExposure();
}

void parse_cmd(int argc, char **argv, Config& mode, bool& reset, bool& debug)
{
	bool found_mode = false;
	bool exp_set = false;
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

			exp_set = true;
			exp = std::stof(argv[argi]);
		}
		else {
			if (setups.count(current) == 0) {
				std::cerr << "Unknown mode " << current << ". Pass -h if you need a list\n";
				exit(1);
			}

			mode = setups.at(current);
			if (exp_set) {
				mode.exposure = exp;
			}
			return;
		}
	}

	if (!found_mode) {
		std::cerr << "Need an operating mode. Pass -h if you need a list\n";
		exit(1);
	}
}

int main(int argc, char **argv) {
	bool reset = false;
	bool debug = false;
	Config mode{"", "", 0, 0, 0.0};

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
//	cont.expose(mode.exposure, true, &callbacks);
	custom_expose(cont.getDev(), mode.exposure, mode.nrows, mode.ncols, &callbacks);
//	cont.stop_logging(std::cout);

	cont.save_to("test_file.fits");

//	custom_expose(cont.getDev(), mode.exposure, mode.nrows, mode.ncols, &callbacks);

//	cont.save_to("test_file2.fits");

	return 0;
}
