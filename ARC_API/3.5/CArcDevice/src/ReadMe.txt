========================================================================
    DYNAMIC LINK LIBRARY : CArcDevice Project Overview
========================================================================

This file contains a summary of what you will find in each of the files
that make up thd CArcDevice DLL.

CArcDevice.cpp/h
    This is the main class for ALL devices. All applications should use
    this class to access all devices.  This class is abstract, so a
    pointer should be used to connect it to an instance of one of a
    specific ARC device ( CArcPCI, CArcPCIe, etc ).

CArcPCIBase.cpp/h
    The base class for all ARC PCI(e) devices.  Provides access to the
    PCI configuration space registers; including BARs.

CArcPCI.cpp/h
    The class for the ARC-63/64 PCI device.
    
CArcPCIe.cpp/h
    The class for the ARC-66 PCIe device.

CArcEdtCL.cpp/h
    The class for the ARC ( EDT ) PCIe8 Camera Link device.

ArcDefs.h
    Command and reply definitions.  Contains other relevent camera and
    device constants.
    
CExpIFace.h
    The interface for exposure callback.  This interface contains methods
    that must be implemented to receive elapsed exposure time and pixel
    count information during exposures.

CConIFace.h
    The interface for continuous readout callback.  This interface contains
    methods that must be implemented to receive frame callback information
    during continuous readout.


/////////////////////////////////////////////////////////////////////////////
Other standard files:

CArcTools.cpp/h
    Useful tools, mostly for text processing and exception handling.
    
CStringList.cpp/h
    A string list class.

ArcOSDefs.h
    System specific function and constant definitions.

CLog.cpp/h
    Debug logging system for API.

TempCtrl.cpp/h
    Temperature control constants.

DllMain.cpp/h
    Windows DLL interface.


/////////////////////////////////////////////////////////////////////////////
Other notes:

						+--------------+
						|  CArcDevice  |
						+--------------+
								|
								|
						+--------------+
						|  CArcPCIBase |
						+--------------+
						|		|	   |
		      +---------+		|      +---------+
			  |					|                |
		+-----------+     +-----------+     +-----------+
		|  CArcPCI  |     |  CArcPCIe |     | CArcEdtCL |
		+-----------+     +-----------+     +-----------+


/////////////////////////////////////////////////////////////////////////////
Each of the device classes ( CArcPCI, CArcPCIe, CArcEdtCL, etc ) contains a
static method, FindDevices(), that MUST be called before a device can be
opened.

Example(s):
					CArcPCIe::FindDevices();
					CArcPCI::FindDevices();
					CArcEdtCL::FindDevices();


/////////////////////////////////////////////////////////////////////////////
Examples on how to create CArcDevice:

1. ARC-63/64 PCI:
					std::auto_ptr<CArcDevice> pArcDev( new CArcPCI() );

					OR

					CArcDevice* pArcDev = new CArcPCI();


2. ARC-66 PCIe:
					std::auto_ptr<CArcDevice> pArcDev( new CArcPCIe() );

					OR

					CArcDevice* pArcDev = new CArcPCIe();


3. ARC Camera Link:
					std::auto_ptr<CArcDevice> pArcDev( new CArcEdtCL() );

					OR

					CArcDevice* pArcDev = new CArcEdtCL();


/////////////////////////////////////////////////////////////////////////////
Example on how to find, open, and close an ARC-66 PCIe device:

					std::auto_ptr<CArcDevice> pArcDev( new CArcPCIe() );

					CArcPCIe::FindDevices();

					pArcDev->Open( 0, ( 4200 * 4200 * 2 ) );

					pArcDev->Close();


/////////////////////////////////////////////////////////////////////////////
