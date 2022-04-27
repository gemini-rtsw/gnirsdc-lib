#include "libgnirs.h"
#include <cstdlib>
#include <iostream>

using namespace arc::device;
using namespace arc::fits;
using namespace arc::deinterlace;

Controller::Controller(int rows, int cols)
	: pArcDev(nullptr),
	  maxRows(rows),
	  maxCols(cols),
	  confRows(rows),
	  confCols(cols),
	  dataSize(2)
{
}

Controller::~Controller()
{
	if (pArcDev != nullptr) {
		pArcDev->Close();
		delete pArcDev;
	}
}


bool
Controller::connect_device(int which)
{
	if (is_connected()) {
		std::cerr << "Already connected.\n";
		return false;
	}

	CArcPCIe::FindDevices();

	if (CArcPCIe::DeviceCount() < 1) {
		std::cerr << "Couldn't find any device.\n";
		return false;
	}

	pArcDev = new CArcPCIe();

	pArcDev->Open(which);

		return pArcDev->IsOpen();
}

bool
Controller::is_fiber_connected() const
{
//	if (!is_connected())
//		throw std::runtime_error("Device not initialized");

	return pArcDev->IsControllerConnected();
}

std::vector<std::string>
Controller::device_list() const
{
	int count;
	std::vector<std::string> res;

	if ((count = CArcPCIe::DeviceCount()) < 1) {
		std::cerr << "No devices to list.\n";
	}
	else {
		const std::string* device_strings = CArcPCIe::GetDeviceStringList();
		for(int i=0; i < count; i++) {
			res.push_back(device_strings[i]);
		}
	}

	return res;
}

bool
Controller::tdl_testing(int times)
{
	if (!is_connected()) {
		std::cerr << "Device not initialized\n";

		return false;
	}

	int bad_tdl = 0;
	for (int i = 0; i < times; i++)
	{
		int r = std::rand() % 65535;
		int dRetVal = pArcDev->Command(TIM_ID, TDL, r);
		if (dRetVal != r)
			bad_tdl++;
	}

	if (bad_tdl != 0) {
		std::cerr << bad_tdl << " bad TDL results out of " << times << '\n';
		return false;
	}

	return true;
}

bool
Controller::load_lod_file(std::string path)
{
	if (!is_connected()) {
		std::cerr << "Device not initialized\n";

		return false;
	}

	// TODO: Test file path
	try {
		pArcDev->LoadControllerFile(path);
	}
	catch (std::runtime_error& e)
	{
		std::cerr << "Error when loading the file\n";
		return false;
	}

	return true;
}

bool
Controller::setup_controller(std::string path, bool power_on, bool reset)
{
	// TODO: Test the file path
	try {
		pArcDev->SetupController( reset,      // Reset the controller
					  true,      // Test the data link
					  power_on,  // Power on the controller or not
					  maxRows,   // Start with a full size detector
					  maxCols,
					  path       // Path to the LOD file
					);
	}
	catch (std::runtime_error& e)
	{
		std::cerr << "Error when setting up the controller\n";
		return false;
	}

	return true;
}


std::string
Controller::device_name() const
{
	if (!is_connected())
		throw std::runtime_error("Device not initialized");

	return pArcDev->ToString();
}

int
Controller::status_string() const
{
	if (!is_connected())
		throw std::runtime_error("Device not initialized");

	return pArcDev->GetStatus();
}

bool
Controller::is_connected() const
{
	if (pArcDev == nullptr)
		return false;

	return pArcDev->IsOpen() && pArcDev->IsControllerConnected();
}

bool
Controller::is_configured() const
{
	if (!is_connected())
		return false;

	return true;
}

bool
Controller::set_size(int rows, int cols)
{
	if (!is_connected()) {
		std::cerr << "Device not initialized\n";

		return false;
	}
/*
	if ((rows > maxRows) || (cols > maxCols)) {
		std::cerr << "Provided size larger than the detector\n";
		return false;
	}
*/
	pArcDev->UnMapCommonBuffer();
	pArcDev->SetImageSize(rows, cols);

	int bufSize = rows * cols * dataSize;
	pArcDev->MapCommonBuffer(bufSize);
	if (pArcDev->CommonBufferSize() != bufSize)
		throw std::runtime_error("The common buffer hasn't mapped properly");
	confRows = rows;
	confCols = cols;

	return true;
}

bool
Controller::set_synthetic(bool setting)
{
	if (!is_connected()) {
		std::cerr << "Device not initialized\n";

		return false;
	}

	pArcDev->SetSyntheticImageMode(setting);

	if (pArcDev->IsSyntheticImageMode() != setting) {
		std::cerr << "Error: couldn't set the desired synthetic image mode\n";

		return false;
	}

	return true;
}

void
Controller::start_logging()
{
	if (!is_connected())
		throw std::runtime_error("Device not initialized");

	pArcDev->SetLogCmds(true);
}

void
Controller::stop_logging(std::ostream& stream)
{
	if (!is_connected())
		throw std::runtime_error("Device not initialized");

	pArcDev->SetLogCmds(false);

	while ( pArcDev->GetLoggedCmdCount() > 0 )
		stream << pArcDev->GetNextLoggedCmd() << '\n';
}

bool
Controller::reset_controller()
{
	if (!is_connected())
		return false;

	pArcDev->ResetController();
	return true;
}

bool
Controller::deinterlace(int mode)
{
	if (!is_connected()) {
		std::cerr << "No controller detected\n";
		return false;
	}

	unsigned short *buf = (unsigned short *)pArcDev->CommonBufferVA();
	CArcDeinterlace cDlacer;
	cDlacer.RunAlg(buf, confRows, confCols, mode);

	return true;
}

bool
Controller::expose(float time, bool open_shutter, CExpIFace* iface)
{
	bool res = false;

	if (!is_connected()) {
		std::cerr << "No controller detected\n";
		return false;
	}

	pArcDev->Expose(time,
			confRows,
			confCols,
			false,    // bAbort
			iface,     // pExpIFace
			open_shutter
			);

	return res;
}

bool
Controller::save_to(std::string path)
{
	if (!is_connected()) {
		std::cerr << "No controller detected\n";
		return false;
	}

	unsigned short *buf = (unsigned short *)pArcDev->CommonBufferVA();
	CArcFitsFile cFits(path.c_str(), confRows, confCols);
        cFits.Write(buf);

	return true;
}

int
Controller::command(int dCommand)
{
	if (!is_connected()) {
		std::cerr << "No controller detected\n";
		return -1;
	}

	return pArcDev->Command(TIM_ID, dCommand);
}

arc::device::CArcDevice*
Controller::getDev()
{
	return pArcDev;
}
