#ifndef __LIB_GNIRS__
#define __LIB_GNIRS__

#include <string>
#include <vector>

#include <CArcDevice.h>
#include <CArcPCIe.h>
#include <ArcDefs.h>
#include <CArcFitsFile.h>
#include <CArcDeinterlace.h>
#include <CExpIFace.h>

class Controller {
public:
	Controller(int rows, int cols);
	virtual ~Controller();

	bool connect_device(int which=0);
	bool is_fiber_connected() const;
	std::vector<std::string> device_list() const;
	bool tdl_testing(int times);
	bool load_lod_file(std::string path);
	bool setup_controller(std::string path, bool power_on=true, bool reset=false);
	std::string device_name() const;
	int status_string() const;
	bool is_connected() const;
	bool is_configured() const;
	bool set_size(int rows, int cols);
	bool set_synthetic(bool setting);
	void start_logging();
	void stop_logging(std::ostream& stream);

	bool reset_controller();
	bool deinterlace(int mode);
	bool expose(float time, bool open_shutter = false, arc::device::CExpIFace* iface = NULL);
	bool save_to(std::string path);
	int command(int dCommand);

	arc::device::CArcDevice* getDev();
private:
	arc::device::CArcDevice* pArcDev;
	int maxRows;
	int maxCols;
	int confRows;
	int confCols;
	int dataSize;

	std::string current_lod;
};

#endif // __LIB_GNIRS__
