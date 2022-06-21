// +----------------------------------------------------------------------+
//  CArcPCIe.cpp : Defines the exported functions for the DLL application.
// +----------------------------------------------------------------------+
//
// KNOWN PROBLEMS:
//
// 1. Dec 15, 2010 - Using DS9 to display image buffer data and then
//    switching devices prevents old device from being closed, thus the
//    module use count doesn't get decremented.  This will result in a
//    "device busy" error.  Only fix is to terminate the application.
//    I think DS9 doesn't release image buffer data, which prevents the
//    close() system call from being called.
//
//    Symptoms:
//    1. Open device 0, then switch to device 1, then back to device 0
//       works; UNLESS DS9 is started and images taken and displayed.
//       THEN, device used to take images will not be closed and module
//       use count ( as seen by /sbin/lsmod ) doesn't decrement.
//    2. The system function close() doesn't call the driver xxxClose
//       function when DS9 is used to display images as described in 1.
// +----------------------------------------------------------------------+

#ifdef WIN32
	#define INITGUID
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <list>
#include <cstddef>
#include <algorithm>
#include <ctime>
#include <ios>
#include "CStringList.h"
#include "CArcPCIe.h"
#include "CArcTools.h"
#include "Reg9056.h"
#include "PCIRegs.h"
#include "ArcDefs.h"
#include "ArcOSDefs.h"

#ifdef WIN32
	#include <setupapi.h>
	#include <devguid.h>
	#include <regstr.h>
	#include "AstroPCIeGUID.h"
#endif

#if defined( linux ) || defined( __linux )
	#include <sys/ioctl.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/mman.h>
	#include <dirent.h>
	#include <fcntl.h>
	#include <errno.h>
	#include <cstring>
#endif

#ifdef __APPLE__
	#include <sys/types.h>
	#include <sys/mman.h>
#endif


using namespace std;
using namespace arc;
using namespace arc::device;


//#include <fstream>
//std::ofstream dbgStream2( "CArcPCIe_Debug.txt" );


// +----------------------------------------------------------------------------
// |  PCIe Device Info
// +----------------------------------------------------------------------------
#if defined( linux ) || defined( __linux )

	#define DEVICE_DIR			"/dev/"
	#define DEVICE_NAME			"AstroPCIe"
	#define DEVICE_NAME_ALT		"Arc66PCIe"

#elif defined( __APPLE__ )

	#define AstroPCIeClassName		 com_arc_driver_AstroPCIe
	#define kAstroPCIeClassName		"com_arc_driver_AstroPCIe"

#endif


// +----------------------------------------------------------------------------
// |  Macro to validate device and vendor id's
// +----------------------------------------------------------------------------
#define VALID_DEV_VEN_ID( devVenId )	\
				( ( devVenId == 0x9056 || devVenId == 0x10B5 ) ? 1 : 0 )



// +----------------------------------------------------------------------------
// |  Macro to verify command values as 24-bits. This is necessary to prevent
// |  the 'AC' pre-amble from being OR'd with other bits and being lost.
// +----------------------------------------------------------------------------
#define VERIFY_24BITS( clazz, method, value )												\
				if ( ( value & 0xFF000000 ) != 0 )											\
				{																			\
					CArcTools::ThrowException(												\
							clazz,															\
							method,															\
							"Data value %d [ 0x%X ] too large! Must be 24-bits or less!",	\
											  value, value );								\
				}


// +----------------------------------------------------------------------------
// |  PLX Register Structure
// +----------------------------------------------------------------------------
typedef struct PLX_REG_ITEM
{
    int dAddr;
    string sText;
} PLXRegItem;



// +----------------------------------------------------------------------------
// |  PLX BAR Address To String Translation Tables
// +----------------------------------------------------------------------------
string LCRMapName       =   "Local Config (BAR0)";
PLXRegItem LCRMap[ 18 ] = { { PCI9056_SPACE0_RANGE, "Direct Slave Local Address Space 0 Range" },
                            { PCI9056_SPACE0_REMAP, "Direct Slave Local Address Space 0 ( Remap )" },
                            { PCI9056_LOCAL_DMA_ARBIT, "Mode/DMA Arbitration" },
                            { PCI9056_ENDIAN_DESC, "Local Misc Ctrl 2/EEPROM Addr Boundary/Local Misc Ctrl 1/Endian Descriptor" },
                            { PCI9056_EXP_ROM_RANGE, "Direct Slave Expansion ROM Range" },
                            { PCI9056_EXP_ROM_REMAP, "Direct Slave Exp ROM Local Base Addr (Remap) & BREQo Ctrl" },
                            { PCI9056_SPACE0_ROM_DESC, "Local Addr Space 0/Expansion ROM Bus Region Descriptor" },
                            { PCI9056_DM_RANGE, "Local Range Direct Master-to-PCIe" },
                            { PCI9056_DM_MEM_BASE, "Local Base Addr Direct Master-to-PCIe Memory" },
                            { PCI9056_DM_IO_BASE, "Local Base Addr Direct Master-to-PCIe I/O Configuration" },
                            { PCI9056_DM_PCI_MEM_REMAP, "PCIe Base Addr (Remap) Master-to-PCIe Memory" },
                            { PCI9056_DM_PCI_IO_CONFIG, "PCI Config Addr Direct Master-to-PCIe I/O Configuration" },
                            { PCI9056_SPACE1_RANGE, "Direct Slave Local Addr Space 1 Range" },
                            { PCI9056_SPACE1_REMAP, "Direct Slave Local Addr Space 1 Local Base Addr (Remap)" },
                            { PCI9056_SPACE1_DESC, "Local Addr Space 1 Bus Region Descriptor" },
                            { PCI9056_DM_DAC, "Direct Master PCIe Dual Addr Cycles Upper Addr" },
                            { PCI9056_ARBITER_CTRL, "Internal Arbiter Control" },
                            { PCI9056_ABORT_ADDRESS, "PCI Abort Address" } };

string RTRMapName       =   "Runtime Regs (BAR0)";
PLXRegItem RTRMap[ 14 ] = { { PCI9056_MAILBOX0, "Mailbox 0" },
                            { PCI9056_MAILBOX1, "Mailbox 1" },
                            { PCI9056_MAILBOX2, "Mailbox 2" },
                            { PCI9056_MAILBOX3, "Mailbox 3" },
                            { PCI9056_MAILBOX4, "Mailbox 4" },
                            { PCI9056_MAILBOX5, "Mailbox 5" },
                            { PCI9056_MAILBOX6, "Mailbox 6" },
                            { PCI9056_MAILBOX7, "Mailbox 7" },
                            { PCI9056_LOCAL_DOORBELL, "PCIe-to-Local Doorbell" },
                            { PCI9056_PCI_DOORBELL, "Local-to-PCIe Doorbell" },
                            { PCI9056_INT_CTRL_STAT, "Interrupt Control/Status" },
                            { PCI9056_EEPROM_CTRL_STAT, "Serial EEPROM Ctrl, PCI Cmd Codes, User I/O Ctrl, Init Ctrl" },
                            { PCI9056_PERM_VENDOR_ID, "Device ID / Vendor ID" },
                            { PCI9056_REVISION_ID, "Reserved / PCI Hardwired Revision ID" } };

string DMAMapName       =   "DMA Regs (BAR0)";
PLXRegItem DMAMap[ 15 ] = { { PCI9056_DMA0_MODE, "DMA Channel 0 Mode" },
                            { PCI9056_DMA0_PCI_ADDR, "DMA Channel 0 PCIe Address" },
                            { PCI9056_DMA0_LOCAL_ADDR, "DMA Channel 0 Local Address" },
                            { PCI9056_DMA0_COUNT, "DMA Channel 0 Transfer Size (Bytes)" },
                            { PCI9056_DMA0_DESC_PTR, "DMA Channel 0 Descriptor Pointer" },
                            { PCI9056_DMA1_MODE, "DMA Channel 1 Mode" },
                            { PCI9056_DMA1_PCI_ADDR, "DMA Channel 1 PCIe Address" },
                            { PCI9056_DMA1_LOCAL_ADDR, "DMA Channel 1 Local Address" },
                            { PCI9056_DMA1_COUNT, "DMA Channel 1 Transfer Size (Bytes)" },
                            { PCI9056_DMA1_DESC_PTR, "DMA Channel 1 Descriptor Pointer" },
                            { PCI9056_DMA_COMMAND_STAT, "Reserved / DMA Ch 1 Cmd-Status / DMA Ch 0 Cmd-Status" },
                            { PCI9056_DMA_ARBIT, "DMA Arbitration" },
                            { PCI9056_DMA_THRESHOLD, "DMA Threshold" },
                            { PCI9056_DMA0_PCI_DAC, "DMA Channel 0 PCIe Dual Addr Cycle Upper Addr" },
                            { PCI9056_DMA1_PCI_DAC, "DMA Channel 1 PCIe Dual Addr Cycle Upper Addr" } };

string MSQMapName       =   "Msg Q Regs (BAR0)";
PLXRegItem MSQMap[ 13 ] = { { PCI9056_OUTPOST_INT_STAT, "Outbound Post Queue Interrupt Status" },
                            { PCI9056_OUTPOST_INT_MASK, "Outbound Post Queue Interrupt Mask" },
                            { PCI9056_MU_CONFIG, "Messaging Queue Configuration" },
                            { PCI9056_FIFO_BASE_ADDR, "Queue Base Address" },
                            { PCI9056_INFREE_HEAD_PTR, "Inbound Free Head Pointer" },
                            { PCI9056_INFREE_TAIL_PTR, "Inbound Free Tail Pointer" },
                            { PCI9056_INPOST_HEAD_PTR, "Inbound Post Head Pointer" },
                            { PCI9056_INPOST_TAIL_PTR, "Inbound Post Tail Pointer" },
                            { PCI9056_OUTFREE_HEAD_PTR, "Outbound Free Head Pointer" },
                            { PCI9056_OUTFREE_TAIL_PTR, "Outbound Free Tail Pointer" },
                            { PCI9056_OUTPOST_HEAD_PTR, "Outbound Post Head Pointer" },
                            { PCI9056_OUTPOST_TAIL_PTR, "Outbound Post Tail Pointer" },
                            { PCI9056_FIFO_CTRL_STAT, "Reserved / Queue Control-Status" } };


// +----------------------------------------------------------------------------
// |  Device Name Sorting Function
// +----------------------------------------------------------------------------
bool DevListSort( ArcDev_t i, ArcDev_t j )
{
	return ( ( i.sName.compare( j.sName ) ) != 0 );
}


// +----------------------------------------------------------------------------
// |  Static Class Member Initialization
// +----------------------------------------------------------------------------
vector<ArcDev_t> CArcPCIe::m_vDevList;
shared_ptr<string> CArcPCIe::m_psDevList;


// +---------------------------------------------------------------------------+
// | ArrayDeleter                                                              |
// +---------------------------------------------------------------------------+
// | Called by std::shared_ptr to delete the temporary image buffer.      |
// | This method should NEVER be called directly by the user.                  |
// +---------------------------------------------------------------------------+
template<typename T> void CArcPCIe::ArrayDeleter( T* p )
{
	if ( p != NULL )
	{
		delete [] p;
	}
}


// +----------------------------------------------------------------------------
// |  Constructor
// +----------------------------------------------------------------------------
// |  See CArcPCIe.h for the class definition
// +----------------------------------------------------------------------------
CArcPCIe::CArcPCIe()
{
	m_hDevice = INVALID_HANDLE_VALUE;
}


// +----------------------------------------------------------------------------
// |  Destructor
// +----------------------------------------------------------------------------
CArcPCIe::~CArcPCIe()
{
	Close();
}


// +----------------------------------------------------------------------------
// |  ToString
// +----------------------------------------------------------------------------
// |  Returns a string that represents the device controlled by this library.
// +----------------------------------------------------------------------------
const string CArcPCIe::ToString()
{
	return string( "PCIe [ ARC-66 / 67 ]" );
}


// +----------------------------------------------------------------------------
// |  FindDevices
// +----------------------------------------------------------------------------
// |  Searches for available ARC, Inc PCIe devices and stores the list, which
// |  can be accessed via device number ( 0,1,2... ).
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCIe::FindDevices()
{
#ifdef __APPLE__

	if ( !m_vDevList.empty() )
	{
		//
		// Don't generate a new list on MAC, the stored service
		// object is currently in use by the open device.
		//
		return;
	}

#else

    if ( !m_vDevList.empty() )
	{
        m_vDevList.clear();

        if ( m_vDevList.size() > 0 )
		{
			CArcTools::ThrowException( "CArcPCIe",
									   "FindDevices",
									   "Failed to free existing device list!" );
		}
	}

#endif

#ifdef WIN32

	PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData = NULL;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	HDEVINFO HardwareDeviceInfo;

   	BOOL  bResult		   = FALSE;
	DWORD dwRequiredLength = 0;
	DWORD dwMemberIndex	   = 0;

	HardwareDeviceInfo = SetupDiGetClassDevs( ( LPGUID )&GUID_DEVINTERFACE_ARC_PCIE,
											   NULL,
											   NULL,
											  ( DIGCF_PRESENT | DIGCF_DEVICEINTERFACE ) );

	if ( HardwareDeviceInfo == INVALID_HANDLE_VALUE )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "FindDevices",
								   "SetupDiGetClassDevs failed!" );
	}

	DeviceInterfaceData.cbSize = sizeof( SP_DEVICE_INTERFACE_DATA );

	while ( 1 )
	{
		bResult = SetupDiEnumDeviceInterfaces( HardwareDeviceInfo,
											   0,
											   ( LPGUID )&GUID_DEVINTERFACE_ARC_PCIE,
											   dwMemberIndex,
											   &DeviceInterfaceData );

		if ( bResult == FALSE )
		{
			SetupDiDestroyDeviceInfoList( HardwareDeviceInfo );
			break;
		}

		SetupDiGetDeviceInterfaceDetail( HardwareDeviceInfo,
										 &DeviceInterfaceData,
										 NULL,
										 0,
										 &dwRequiredLength,
										 NULL );

		DeviceInterfaceDetailData = ( PSP_DEVICE_INTERFACE_DETAIL_DATA )
										LocalAlloc( LMEM_FIXED, dwRequiredLength );

		if ( DeviceInterfaceDetailData == NULL )
		{
			SetupDiDestroyDeviceInfoList( HardwareDeviceInfo );
			break;
		}

		DeviceInterfaceDetailData->cbSize = sizeof( SP_DEVICE_INTERFACE_DETAIL_DATA );

		bResult = SetupDiGetDeviceInterfaceDetail( HardwareDeviceInfo,
												   &DeviceInterfaceData,
												   DeviceInterfaceDetailData,
												   dwRequiredLength,
												   &dwRequiredLength,
												   NULL );

		if ( bResult == FALSE )
		{
			SetupDiDestroyDeviceInfoList( HardwareDeviceInfo );
			LocalFree( DeviceInterfaceDetailData );
			break;
		}

		ArcDev_t tArcDev;

		tArcDev.sName =
				CArcTools::ConvertWideToAnsi(
								( LPWSTR )DeviceInterfaceDetailData->DevicePath );

		m_vDevList.push_back( tArcDev );

		dwMemberIndex++;
	}

#elif defined( __APPLE__ )

    io_service_t  service;
    io_iterator_t iterator;

	//
	// Look up the objects we wish to open. This example uses simple class
    // matching (IOServiceMatching()) to find instances of the class defined by the kext.
	//
	// Because Mac OS X has no weak-linking support in the kernel, the only way to
	// support mutually-exclusive KPIs is to provide separate kexts. This in turn means that the
	// separate kexts must have their own unique CFBundleIdentifiers and I/O Kit class names.
	//
	// This sample shows how to do this in the SimpleUserClient and SimpleUserClient_10.4 Xcode targets.
	//
	// From the userland perspective, a process must look for any of the class names it is prepared to talk to.
	//
    // This creates an io_iterator_t of all instances of our driver that exist in the I/O Registry.
	//
    kern_return_t kernResult = IOServiceGetMatchingServices( kIOMasterPortDefault,
															 IOServiceMatching( kAstroPCIeClassName ),
														     &iterator );

    if ( kernResult != KERN_SUCCESS )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "FindDevices",
								   "IOServiceGetMatchingServices failed: 0x%X",
								    kernResult );
    }

    while ( ( service = IOIteratorNext( iterator ) ) != IO_OBJECT_NULL )
	{
		ArcDev_t tArcDev;

		tArcDev.sName			= kAstroPCIeClassName;
		tArcDev.tService		= service;

		m_vDevList.push_back( tArcDev );
	}

	//
    // Release the io_iterator_t now that we're done with it.
	//
    IOObjectRelease( iterator );


#else	// LINUX

	struct dirent* pDirEntry = NULL;
	DIR	*pDir = NULL;

	pDir = opendir( DEVICE_DIR );

	if ( pDir == NULL )
	{
		CArcTools::ThrowException(
							"CArcPCIe",
							"FindDevices",
							"Failed to open dir: %s",
							 DEVICE_DIR );
	}
	else
	{
		while ( ( pDirEntry = readdir( pDir ) ) != NULL )
		{
			std::string sDirEntry( pDirEntry->d_name );

			if ( sDirEntry.find( DEVICE_NAME ) != std::string::npos ||
				 sDirEntry.find( DEVICE_NAME_ALT ) != std::string::npos )
			{
				ArcDev_t tArcDev;
				tArcDev.sName   = DEVICE_DIR + sDirEntry;
				m_vDevList.push_back( tArcDev );
			}
		}

		closedir( pDir );
	}

#endif

    //
	// Make sure the list isn't empty
	//
	if ( m_vDevList.empty() )
	{
		CArcTools::ThrowException(
							"CArcPCIe",
							"FindDevices",
							string( "No device bindings exist! " ) +
							"Make sure an ARC, Inc PCIe card is installed!" );
	}

	//
	// Sort the device list by name
	//
//	sort( m_vDevList.begin(), m_vDevList.end(), DevListSort );
}


// +----------------------------------------------------------------------------
// |  DeviceCount
// +----------------------------------------------------------------------------
// |  Returns the number of items in the device list. Must be called after
// |  FindDevices().
// +----------------------------------------------------------------------------
int CArcPCIe::DeviceCount()
{
	return static_cast<int>( m_vDevList.size() );
}


// +----------------------------------------------------------------------------
// |  GetDeviceStringList
// +----------------------------------------------------------------------------
// |  Returns a string list representation of the device list. Must be called
// |  after GetDeviceList().
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
const string* CArcPCIe::GetDeviceStringList()
{
    if ( !m_vDevList.empty() )
    {
		m_psDevList.reset( new string[ m_vDevList.size() ],
						   &CArcPCIe::ArrayDeleter<string> );

		for ( string::size_type i=0; i<m_vDevList.size(); i++ )
        {
			ostringstream oss;

			oss << "PCIe Device "
				<< i
#ifdef WIN32
				<< ends;
#else
				<< m_vDevList.at( i ).sName
				<< ends;
#endif

			( m_psDevList.get() )[ i ] = string( oss.str() );
		}
    }
    else
    {
		m_psDevList.reset( new string[ 1 ], &CArcPCIe::ArrayDeleter<string> );

		( m_psDevList.get() )[ 0 ] = string( "No Devices Found!" );
   }

	return const_cast<const string *>( m_psDevList.get() );
}


// +----------------------------------------------------------------------------
// |  IsOpen
// +----------------------------------------------------------------------------
// |  Returns 'true' if connected to PCIe device; 'false' otherwise.
// |
// |  Throws NOTHING on error. No error handling.
// +----------------------------------------------------------------------------
bool CArcPCIe::IsOpen()
{
	return CArcDevice::IsOpen();
}

// +----------------------------------------------------------------------------
// |  Open
// +----------------------------------------------------------------------------
// |  Opens a connection to the device driver associated with the specified
// |  device.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dDeviceNumber - Device number
// +----------------------------------------------------------------------------
void CArcPCIe::Open( int dDeviceNumber )
{
	if ( dDeviceNumber < 0 || dDeviceNumber > int( m_vDevList.size() ) )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "Open",
								   "Invalid device number: %d",
								    dDeviceNumber );
	}

	if ( IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "Open",
								   "Device already open, call Close() first!" );
	}

	std::string sDeviceName = m_vDevList.at( dDeviceNumber ).sName;

#ifdef __APPLE__

	io_service_t tService = m_vDevList.at( dDeviceNumber ).tService;
	Arc_OpenHandle( m_hDevice, &tService );

#else

	Arc_OpenHandle( m_hDevice, sDeviceName.c_str() );

#endif

	if ( m_hDevice == INVALID_HANDLE_VALUE )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "Open",
								   "Failed to open device ( %s ) : %s",
								   sDeviceName.c_str(),
								   CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );
	}

	// EXTREMELY IMPORTANT
	//
	// Prevent a forking problem. Forking a new process results in the
	// device file descriptor being copied to the child process, which
	// results in problems when later trying to close the device from
	// the parent process.
	//
	// Example: The CArcDisplay::Launch() method will fork a new DS9.
	// The parent and child processes now hold copies of the device file
	// descriptor. The OS marks the device usage count as being two;
	// instead of one. When the parent then closes and tries to re-open
	// the same device it will fail, because the usage count prevents
	// the device from closing ( since it's greater than zero ). In fact,
	// calling close( hdev ) will not result in the driver close/release
	// function being called because the usage count is not zero.  The
	// fcntl() function sets the device file descriptor to close on exec(),
	// which is used in conjunction with the fork() call, and causes the
	// child's copies of the device descriptor to be closed before exec()
	// is called.
#if defined( linux ) || defined( __linux )

	fcntl( m_hDevice, F_SETFD, FD_CLOEXEC );

#endif

	//
	//  Clear the status register
	// +-------------------------------------------------------+
	ClearStatus();
}

// +----------------------------------------------------------------------------
// |  Open
// +----------------------------------------------------------------------------
// |  This version first calls Open, then MapCommonBuffer if Open
// |  succeeded. Basically, this function just combines the other two.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dDeviceNumber - PCI device number
// |  <IN>  -> dBytes - The size of the kernel image buffer in bytes
// +----------------------------------------------------------------------------
void CArcPCIe::Open( int dDeviceNumber, int dBytes )
{
	Open( dDeviceNumber );

	MapCommonBuffer( dBytes );
}

// +----------------------------------------------------------------------------
// |  Open
// +----------------------------------------------------------------------------
// |  Convenience method. This version first calls Open, then MapCommonBuffer
// |  if Open succeeded. Basically, this function just combines the other two.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dDeviceNumber - PCI device number
// |  <IN>  -> dRows         - The image buffer row size ( in pixels )
// |  <IN>  -> dCols         - The image buffer column size ( in pixels )
// +----------------------------------------------------------------------------
void CArcPCIe::Open( int dDeviceNumber, int dRows, int dCols )
{
	Open( dDeviceNumber );

	MapCommonBuffer( ( dRows * dCols * sizeof( unsigned short ) ) );
}

// +----------------------------------------------------------------------------
// |  Close
// +----------------------------------------------------------------------------
// |  Closes the currently open driver that was opened with a call to
// |  Open.
// |
// |  Throws NOTHING on error. No error handling.
// +----------------------------------------------------------------------------
void CArcPCIe::Close()
{
	//
	// Prevents access violation from code that follows
	//
	bool bOldStoreCmds = m_bStoreCmds;
	m_bStoreCmds       = false;

	UnMapCommonBuffer();

	Arc_CloseHandle( m_hDevice );

	m_dCCParam   = 0;
 	m_hDevice    = INVALID_HANDLE_VALUE;
	m_bStoreCmds = bOldStoreCmds;
}

// +----------------------------------------------------------------------------
// |  Reset
// +----------------------------------------------------------------------------
// |  Resets the PCIe board.
// |
// |  Throws NOTHING on error. No error handling.
// +----------------------------------------------------------------------------
void CArcPCIe::Reset()
{
	WriteBar( DEV_REG_BAR, REG_RESET, 1 );

	//
	// Check that the status is now idle
	//
	int dStatus = ReadBar( DEV_REG_BAR, REG_STATUS );

	if ( !PCIe_STATUS_IDLE( dStatus ) )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "Reset",
								   "Reset failed! Device status not idle: 0x%X",
								    dStatus );
	}
}


// +----------------------------------------------------------------------------
// |  MapCommonBuffer
// +----------------------------------------------------------------------------
// |  Map the device driver image buffer.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> bBytes - The number of bytes to map as an image buffer. Not
// |                    used by PCI/e.
// +----------------------------------------------------------------------------
void CArcPCIe::MapCommonBuffer( int dBytes )
{
	if ( dBytes <= 0 )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "MapCommonBuffer",
								   "Invalid buffer size: %d. Must be greater than zero!",
								    dBytes );		
	}

	//  Map the kernel buffer
	// +----------------------------------------------------+
	m_tImgBuffer.pUserAddr = Arc_MMap( m_hDevice,
									   ARC_MEM_MAP,
									   size_t( dBytes ) );

	if ( m_tImgBuffer.pUserAddr == MAP_FAILED )
	{
		Arc_ZeroMemory( &m_tImgBuffer, sizeof( ImgBuf_t ) );

		CArcTools::ThrowException( "CArcPCIe",
								   "MapCommonBuffer",
								   "Failed to map image buffer : %s",
									CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );		
	}

	bool bSuccess = GetCommonBufferProperties();

	if ( !bSuccess )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "MapCommonBuffer",
								   "Failed to read image buffer size : %s",
									CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );		
	}

	if ( m_tImgBuffer.dSize < size_t( dBytes ) )
	{
		ostringstream oss;

		oss << "Failed to allocate buffer of the correct size.\nWanted: "
			<< dBytes << " bytes [ " << ( dBytes / 1E6 ) << "MB ] - Received: "
			<< m_tImgBuffer.dSize << " bytes [ " << ( m_tImgBuffer.dSize / 1E6 )
			<< "MB ]";

		CArcTools::ThrowException( "CArcPCIe",
								   "MapCommonBuffer",
								    oss.str() );		
	}
}

// +----------------------------------------------------------------------------
// |  UnMapCommonBuffer
// +----------------------------------------------------------------------------
// |  Un-Maps the device driver image buffer.
// |
// |  Throws NOTHING
// +----------------------------------------------------------------------------
void CArcPCIe::UnMapCommonBuffer()
{
	if ( m_tImgBuffer.pUserAddr != ( void * )NULL )
	{
		Arc_MUnMap( m_hDevice,
					ARC_MEM_UNMAP,
					m_tImgBuffer.pUserAddr,
					static_cast<int>( m_tImgBuffer.dSize ) );
	}

	Arc_ZeroMemory( &m_tImgBuffer, sizeof( ImgBuf_t ) );
}

// +----------------------------------------------------------------------------
// |  GetId
// +----------------------------------------------------------------------------
// |  Returns the PCIe board id, which should be 'ARC6'
// +----------------------------------------------------------------------------
int CArcPCIe::GetId()
{
	return ReadBar( DEV_REG_BAR, REG_ID_HI );
}

// +----------------------------------------------------------------------------
// |  GetStatus
// +----------------------------------------------------------------------------
// |  Returns the PCIe status register value.
// +----------------------------------------------------------------------------
int CArcPCIe::GetStatus()
{
	return ReadBar( DEV_REG_BAR, REG_STATUS );
}

// +----------------------------------------------------------------------------
// |  ClearStatus
// +----------------------------------------------------------------------------
// |  Clears the PCIe status register.
// +----------------------------------------------------------------------------
void CArcPCIe::ClearStatus()
{
	WriteBar( DEV_REG_BAR,
			  REG_STATUS,
			  PCIe_STATUS_CLEAR_ALL );
}

// +----------------------------------------------------------------------------
// |  Set2xFOTransmitter
// +----------------------------------------------------------------------------
// |  Sets the controller to use two fiber optic transmitters.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> bOnOff - True to enable dual transmitters; false otherwise.
// +----------------------------------------------------------------------------
void CArcPCIe::Set2xFOTransmitter( bool bOnOff )
{
	int dReply = 0;

	if ( bOnOff )
	{
		if ( ( dReply = Command( TIM_ID, XMT, 1 ) ) != DON )
		{
			CArcTools::ThrowException(
						"CArcPCIe",
						"Set2xFOTransmitter",
						"Failed to SET use of 2x fiber optic transmitters on controller, reply: 0x%X",
						 dReply );
		}

		WriteBar( DEV_REG_BAR,
				  REG_FIBER_2X_CTRL,
				  FIBER_2X_ENABLE );
	}
	else
	{
		if ( ( dReply = Command( TIM_ID, XMT, 0 ) ) != DON )
		{
			CArcTools::ThrowException(
						"CArcPCIe",
						"Set2xFOTransmitter",
						"Failed to CLEAR use of 2x fiber optic transmitters on controller, reply: 0x%X",
						 dReply );
		}

		WriteBar( DEV_REG_BAR,
				  REG_FIBER_2X_CTRL,
				  FIBER_2X_DISABLE );
	}
}

// +----------------------------------------------------------------------------
// |  LoadDeviceFile
// +----------------------------------------------------------------------------
// |  Not used by PCIe.
// +----------------------------------------------------------------------------
void CArcPCIe::LoadDeviceFile( const string sFile )
{
	CArcTools::ThrowException( "CArcPCIe",
							   "LoadDeviceFile",
							   "Method not available for PCIe!" );
}


// +----------------------------------------------------------------------------
// |  Command
// +----------------------------------------------------------------------------
// |  Send a command to the controller timing or utility board. Returns the
// |  controller reply, typically DON.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dBoardId       - Command board id ( TIM or UTIL )
// |  <IN>  -> dCommand       - Board command
// |  <IN>  -> dArg1 to dArg4 - Command arguments ( optional )
// +----------------------------------------------------------------------------
int CArcPCIe::Command( int dBoardId, int dCommand, int dArg0, int dArg1, int dArg2, int dArg3 )
{
	int dNumOfArgs = 2;
	int dHeader    = 0;
	int dReply     = 0;

	//
	//  Calculate the number of arguments
	// +-------------------------------------------------+
	if ( dArg0 >= 0 ) { dNumOfArgs++; }
	if ( dArg1 >= 0 ) { dNumOfArgs++; }
	if ( dArg2 >= 0 ) { dNumOfArgs++; }
	if ( dArg3 >= 0 ) { dNumOfArgs++; }

	//
	//  Report error if device reports readout in progress
	// +------------------------------------------------------+
	if ( PCIe_STATUS_READOUT( ReadBar( DEV_REG_BAR, REG_STATUS ) ) )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "Command",
								   "Device reports readout in progress! Status: 0x%X",
								    ReadBar( DEV_REG_BAR, REG_STATUS ) );
	}

	//
	//  Clear Status Register
	// +-------------------------------------------------+
	ClearStatus();

	try
	{
		//
		//  Send Header
		// +-------------------------------------------------+
		dHeader = ( ( dBoardId << 8 ) | dNumOfArgs );

		VERIFY_24BITS( "CArcPCIe", "Command", dHeader )

		WriteBar( DEV_REG_BAR,
				  REG_CMD_HEADER,
				  ( 0xAC000000 | dHeader ) );

		//
		//  Send Command
		// +-------------------------------------------------+
		VERIFY_24BITS( "CArcPCIe", "Command", dCommand )

		WriteBar( DEV_REG_BAR,
				  REG_CMD_COMMAND,
				  ( 0xAC000000 | dCommand ) );

		//
		//  Send Arguments
		// +-------------------------------------------------+
		if ( dArg0 >= 0 )
		{
			VERIFY_24BITS( "CArcPCIe", "Command", dArg0 )

			WriteBar( DEV_REG_BAR,
					  REG_CMD_ARG0,
					  ( 0xAC000000 | dArg0 ) );
		}

		if ( dArg1 >= 0 )
		{
			VERIFY_24BITS( "CArcPCIe", "Command", dArg1 )

			WriteBar( DEV_REG_BAR,
					  REG_CMD_ARG1,
					  ( 0xAC000000 | dArg1 ) );
		}

		if ( dArg2 >= 0 )
		{
			VERIFY_24BITS( "CArcPCIe", "Command", dArg2 )

			WriteBar( DEV_REG_BAR,
					  REG_CMD_ARG2,
					  ( 0xAC000000 | dArg2 ) );
		}

		if ( dArg3 >= 0 )
		{
			VERIFY_24BITS( "CArcPCIe", "Command", dArg3 )

			WriteBar( DEV_REG_BAR,
					  REG_CMD_ARG3,
					  ( 0xAC000000 | dArg3 ) );
		}
	}
	catch ( ... )
	{
		if ( m_bStoreCmds )
		{
			m_cApiLog.Put(
					CArcTools::CmdToString( dReply,
											dBoardId,
				   							dCommand,
											dArg0,
											dArg1,
											dArg2,
											dArg3 ).c_str() );
		}

		throw;
	}

	//
	//  Return the reply
	// +-------------------------------------------------+
	try
	{
		dReply = ReadReply();
	}
	catch ( exception& e )
	{
		if ( m_bStoreCmds )
		{
			m_cApiLog.Put(
					CArcTools::CmdToString( dReply,
											dBoardId,
				   							dCommand,
											dArg0,
											dArg1,
											dArg2,
											dArg3 ).c_str() );
		}

		ostringstream oss;

		oss << e.what() << endl
			<< "Exception Details: 0x"
			<< hex << dHeader << dec << " "
			<< CArcTools::CmdToString( dCommand ) << " "
			<< "0x" << hex << dArg0 << dec << " "
			<< "0x" << hex << dArg1 << dec << " "
			<< "0x" << hex << dArg2 << dec << " "
			<< "0x" << hex << dArg3 << dec << endl;

		CArcTools::ThrowException( "CArcPCIe", "Command", oss.str() );
	}

	//
	// Set the debug message queue.
	//
	if ( m_bStoreCmds )
	{
		m_cApiLog.Put(
				CArcTools::CmdToString( dReply,
										dBoardId,
				   						dCommand,
										dArg0,
										dArg1,
										dArg2,
										dArg3 ).c_str() );
	}

	if ( dReply == CNR )
	{
		CArcTools::ThrowException(
						"CArcPCIe",
						"Command",
						"Controller not ready! Verify controller has been setup! Reply: 0x%X",
						 dReply );
	}

    return dReply;
}

// +----------------------------------------------------------------------------
// |  GetControllerId
// +----------------------------------------------------------------------------
// |  Returns the controller ID or 'ERR' if none.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcPCIe::GetControllerId()
{
	int dReply = 0;

	//
	//  Clear status register
	// +-------------------------------------------------+
	ClearStatus();

	//
	//  Get the controller ID
	// +-------------------------------------------------+
	WriteBar( DEV_REG_BAR,
			  REG_CTLR_SPECIAL_CMD,
			  CONTROLLER_GET_ID );

	//
	//  Read the reply
	//
	//  NOTE: Ignore any error, as Gen III will return
	//  a timeout, which will indicate a Gen III system.
	// +-------------------------------------------------+
	try
	{
		dReply = ReadReply( 0.5 );
	}
	catch ( ... )
	{
		dReply = 0;
	}

	return dReply;
}

// +----------------------------------------------------------------------------
// |  ResetController
// +----------------------------------------------------------------------------
// |  Resets the controller.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCIe::ResetController()
{
	//
	//  Clear status register
	// +-------------------------------------------------+
	ClearStatus();

	//
	//  Reset the controller or DSP
	// +-------------------------------------------------+
	WriteBar( DEV_REG_BAR,
			  REG_CTLR_SPECIAL_CMD,
			  CONTROLLER_RESET );

	//
	//  Read the reply
	// +-------------------------------------------------+
	int dReply = ReadReply();

	if ( dReply != SYR )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "ResetController",
						           "Failed to reset controller, reply: 0x%X",
						            dReply );
	}
}

// +----------------------------------------------------------------------------
// | IsControllerConnected
// +----------------------------------------------------------------------------
// |  Returns 'true' if a controller is connected to the PCIe board.  This is
// |  for fiber optic A only.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
bool CArcPCIe::IsControllerConnected()
{
	return IsFiberConnected();
}

// +----------------------------------------------------------------------------
// | IsFiberConnected
// +----------------------------------------------------------------------------
// |  Returns 'true' if the specified  PCIe fiber optic is connected to a
// |  powered-on controller.
// |
// |  <IN> -> dFiberId - The ID of the fiber to check: CArcPCIe::FIBER_A or
// |                     CArcPCIe::FIBER_B. Default: FIBER_A
// |
// |  Throws std::runtime_error on invalid parameter
// +----------------------------------------------------------------------------
bool CArcPCIe::IsFiberConnected( int dFiberId )
{
	bool bConnected = false;

	if ( dFiberId == FIBER_A )
	{
		bConnected = PCIe_STATUS_FIBER_A_CONNECTED( GetStatus() );
	}

	else if ( dFiberId == FIBER_B )
	{
		bConnected = PCIe_STATUS_FIBER_B_CONNECTED( GetStatus() );
	}

	else
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "IsFiberConnected",
								   "Invalid fiber id: %d",
								    dFiberId );
	}

	return bConnected;
}

// +----------------------------------------------------------------------------
// |  StopExposure
// +----------------------------------------------------------------------------
// |  Stops the current exposure.
// |
// |  NOTE: The command is sent manually and NOT using the Command() method
// |        because the Command() method checks for readout and fails.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCIe::StopExposure()
{
	//
	//  Send Header
	// +-------------------------------------------------+
	int dHeader = ( ( TIM_ID << 8 ) | 2 );

	WriteBar( DEV_REG_BAR,
			  REG_CMD_HEADER,
			  ( 0xAC000000 | dHeader ) );

	//
	//  Send Command
	// +-------------------------------------------------+
	WriteBar( DEV_REG_BAR,
			  REG_CMD_COMMAND,
			  ( 0xAC000000 | ABR ) );

	//
	//  Read the reply
	// +-------------------------------------------------+
	int dReply = ReadReply();

	if ( dReply != DON )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "StopExposure",
						           "Failed to stop exposure/readout, reply: 0x%X",
						            dReply );
	}
}

// +----------------------------------------------------------------------------
// |  IsReadout
// +----------------------------------------------------------------------------
// |  Returns 'true' if the controller is in readout.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
bool CArcPCIe::IsReadout()
{
	//int dReadOut = ( GetStatus() & 0x4 ) >> 2;

	//return static_cast<bool>( dReadOut );

	return ( ( ( GetStatus() & 0x4 ) > 0 ) ? true : false );
}

// +----------------------------------------------------------------------------
// |  GetPixelCount
// +----------------------------------------------------------------------------
// |  Returns the current pixel count.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcPCIe::GetPixelCount()
{
	int dPixCnt = ReadBar( DEV_REG_BAR, REG_PIXEL_COUNT );

	if ( m_bStoreCmds )
	{
		m_cApiLog.Put(
			CArcTools::FormatString( "[ PIXEL COUNT REG: 0x%X -> %d ]",
									  REG_PIXEL_COUNT,
									  dPixCnt ).c_str() );
	}

	return dPixCnt;
}

// +----------------------------------------------------------------------------
// |  GetCRPixelCount
// +----------------------------------------------------------------------------
// |  Returns the cumulative pixel count as when doing continuous readout.
// |  This method is used by user applications when reading super dArge images
// | ( greater than 4K x 4K ). This value increases across all frames being
// |  readout. This code needs to execute fast, so not pre-checking, such as 
// |  IsControllerConnected() is done.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcPCIe::GetCRPixelCount()
{
	CArcTools::ThrowException( "CArcPCIe",
							   "GetCRPixelCount",
							   "Method not supported by PCIe!" );
	return 0;
}

// +----------------------------------------------------------------------------
// |  GetFrameCount
// +----------------------------------------------------------------------------
// |  Returns the current frame count. The camera MUST be set for continuous
// |  readout for this to work. This code needs to execute fast, so no 
// |  pre-checking, such as IsControllerConnected() is done.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcPCIe::GetFrameCount()
{
	int dFrameCnt = ReadBar( DEV_REG_BAR, REG_FRAME_COUNT );

	if ( m_bStoreCmds )
	{
		m_cApiLog.Put(
			CArcTools::FormatString( "[ FRAME COUNT REG: 0x%X -> %d ]",
									  REG_FRAME_COUNT,
									  dFrameCnt ).c_str() );
	}

	return dFrameCnt;
}

// +----------------------------------------------------------------------------
// |  WriteBar
// +----------------------------------------------------------------------------
// |  Writes a value to the specified PCI/e BAR offset using mapped registers.
// |  Automatically calls WriteBarDirect() if no mappings exist.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> dBar    - The PCI BAR number ( 0 - 5 ).
// |  <IN> -> bOffset - The offset into the specified BAR.
// |  <IN> -> dValue  - 32-bit value to write.
// +----------------------------------------------------------------------------
void CArcPCIe::WriteBar( int dBar, int dOffset, int dValue )
{
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "WriteBar",
								   "Not connected to any device." );
	}

	int dArgs[ 3 ] = { dBar,
					   dOffset,
					   dValue };

	if ( dBar < 0 || dBar > 5 )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "WriteBar",
								   "Invalid BAR number: 0x%X",
									dBar );
	}

	int dSuccess = Arc_IOCtl( m_hDevice,
							  ARC_WRITE_BAR,
							  dArgs,
							  sizeof( dArgs ) );

	if ( !dSuccess )
	{
		//if ( m_bStoreCmds )
		//{
		//	m_cApiLog.Put( "Write Bar: %d  Offset: %s  Value: 0x%X",
		//					dBar, GetFPGARegString( dOffset ), dValue );
		//}

		CArcTools::ThrowException( "CArcPCIe",
								   "WriteBar",
								   "Writing 0x%X to 0x%X : 0x%X failed! %s",
									dValue,
									dBar,
									dOffset,
									CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );
	}

	//if ( m_bStoreCmds )
	//{
	//	m_cApiLog.Put( "Write Bar: %d  Offset: %s  Value: 0x%X",
	//					dBar, GetFPGARegString( dOffset ), dValue );
	//}
}

// +----------------------------------------------------------------------------
// |  ReadBar
// +----------------------------------------------------------------------------
// |  Read a value from the specified PCI/e BAR offset using mapped registers.
// |  Automatically calls ReadBarDirect() if no mappings exist.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> dBar    - The PCI BAR number ( 0 - 5 ).
// |  <IN> -> bOffset - The offset into the specified BAR.
// +----------------------------------------------------------------------------
int CArcPCIe::ReadBar( int dBar, int dOffset )
{
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "ReadBar",
								   "Not connected to any device." );
	}

	int dIn[ 2 ] = { dBar, dOffset };

	if ( dBar < 0 || dBar > 5 )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "ReadBar",
								   "Invalid BAR number: 0x%X",
									dBar );
	}

	int dSuccess = Arc_IOCtl( m_hDevice,
							  ARC_READ_BAR,
							  dIn,
							  sizeof( dIn ) );

	if ( !dSuccess )
	{
		//if ( m_bStoreCmds )
		//{
		//	m_cApiLog.Put( "Read Bar: %d  Offset: %s  -> 0x%X",
		//					dBar,
		//					GetFPGARegString( dOffset ),
		//					dIn[ 0 ] );
		//}

		CArcTools::ThrowException( "CArcPCIe",
								   "ReadBar",
								   "Reading 0x%X : 0x%X failed! %s",
									dBar,
									dOffset,
									CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );
	}

	//if ( m_bStoreCmds )
	//{
	//	m_cApiLog.Put( "Read Bar: %d  Offset: %s  -> 0x%X",
	//					dBar,
	//					GetFPGARegString( dOffset ),
	//					dIn[ 0 ] );
	//}

	return dIn[ 0 ];
}

// +----------------------------------------------------------------------------
// |  GetCommonBufferProperties
// +----------------------------------------------------------------------------
// |  Fills in the image buffer structure with its properties, such as
// |  physical address and size.
// |
// |  Throws NOTHING on error. No error handling.
// +----------------------------------------------------------------------------
bool CArcPCIe::GetCommonBufferProperties()
{
	unsigned long lProps[ 2 ] = { 0, 0 };

	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "GetCommonBufferProperties",
								   "Not connected to any device." );
	}

	int dSuccess = Arc_IOCtl( m_hDevice,
							  ARC_BUFFER_PROP,
							  lProps,
							  sizeof( lProps ) );

	if ( !dSuccess )
	{
		return false;
	}

	m_tImgBuffer.ulPhysicalAddr = ( ulong  )lProps[ 0 ];
	m_tImgBuffer.dSize          = ( size_t )lProps[ 1 ];

	return true;
}

// +----------------------------------------------------------------------------
// |  LoadGen23ControllerFile
// +----------------------------------------------------------------------------
// |  Loads a timing or utility file (.lod) into a GenII or GenIII controller.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> sFilename   - The TIM or UTIL lod file to load.
// |  <IN> -> bValidate   - Set to 1 if the download should be read back and
// |                        checked after every write.
// |  <IN> -> bAbort      - 'true' to stop; 'false' otherwise. Default: false
// +----------------------------------------------------------------------------
void CArcPCIe::LoadGen23ControllerFile( const string sFilename, bool bValidate, const bool& bAbort )
{
	int  dBoardId		= 0;
	int  dType			= 0;
	int  dAddr			= 0;
	int  dData			= 0;
	int  dReply			= 0;
	char typeChar		= ' ';
	bool bIsCLodFile	= false;

	std::string sLine;
	std::string sToken;
	CArcTools::CTokenizer cTokenizer;

	if ( bAbort ) { return; }

	//
	// Verify device connection
	// -------------------------------------------------------------------
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "LoadGen23ControllerFile",
								   "Not connected to any device." );
	}

	if ( bAbort ) { return; }

	//
	// Open the file for reading.
	// -------------------------------------------------------------------
	ifstream inFile( sFilename.c_str() );

	if ( !inFile.is_open() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "LoadGen23ControllerFile",
						           "Cannot open file: %s",
						            sFilename.c_str() );
	}

	if ( bAbort ) { inFile.close(); return; }

	//
	// Check for valid TIM or UTIL file
	// -------------------------------------------------------------------
	getline( inFile, sLine );

	if ( sLine.find( "TIM" ) != std::string::npos )
	{
		dBoardId = TIM_ID;
	}
	else if ( sLine.find( "CRT" ) != std::string::npos )
	{
		dBoardId = TIM_ID;
		bIsCLodFile = true;
	}
	else if ( sLine.find( "UTIL" ) != std::string::npos )
	{
		dBoardId = UTIL_ID;
	}
	else
	{
		CArcTools::ThrowException(
					"CArcPCIe",
					"LoadGen23ControllerFile",
					"Invalid file. Missing 'TIMBOOT/CRT' or 'UTILBOOT' string." );
	}

	if ( bAbort ) { inFile.close(); return; }

	//
	// First, send the stop command. Otherwise, the controller crashes
	// because it is downloading and executing code while you try to
	// overwrite it.
	// -----------------------------------------------------------------
	dReply = Command( TIM_ID, STP );

	if ( dReply != DON )
	{
		CArcTools::ThrowException(
					"CArcPCIe",
					"LoadGen23ControllerFile",
					"Stop ('STP') controller failed. Reply: 0x%X",
					 dReply );
	}

	if ( bAbort ) { inFile.close(); return; }

	//
	// Set the PCI status bit #1 (X:0 bit 1 = 1).
	// -------------------------------------------
	// Not Used by PCIe

	//
	// Read in the file one line at a time
	// --------------------------------------
	while ( !inFile.eof() )
	{
		if ( bAbort ) { inFile.close(); return; }

		getline( inFile, sLine );

		//
		// Only "_DATA" blocks are valid for download
		// ---------------------------------------------
		if ( sLine.find( '_' ) == 0 && sLine.find( "_DATA " ) != std::string::npos )
		{
			cTokenizer.Victim( sLine );
			cTokenizer.Next();	// Dump _DATA string

			//
			// Get the memory type and start address
			// ---------------------------------------------
			typeChar = CArcTools::StringToChar( cTokenizer.Next() );
			dAddr    = CArcTools::StringToHex( cTokenizer.Next() );

			//
			// The start address must be less than MAX_DSP_START_LOAD_ADDR
			// -------------------------------------------------------------
			if ( dAddr < MAX_DSP_START_LOAD_ADDR )
			{
				//
				// Set the DSP memory type
				// ----------------------------------
				if      ( typeChar == 'X' ) dType = X_MEM;
				else if ( typeChar == 'Y' ) dType = Y_MEM;
				else if ( typeChar == 'P' ) dType = P_MEM;
				else if ( typeChar == 'R' ) dType = R_MEM;

				//
				// Read the data block
				// ----------------------------------
				while ( !inFile.eof() && inFile.peek() != '_' )
				{
					if ( bAbort ) { inFile.close(); return; }

					getline( inFile, sLine );
					cTokenizer.Victim( sLine );

					while ( !( ( sToken = cTokenizer.Next() ).empty() ) )
					{
						if ( bAbort ) { inFile.close(); return; }

						dData = CArcTools::StringToHex( sToken );

						//
						// Write the data to the controller.
						// --------------------------------------------------------------
						dReply = Command( dBoardId,
										  WRM,
										  ( dType | dAddr ),
										  dData );

						if ( dReply != DON )
						{
							CArcTools::ThrowException(
											"CArcPCIe",
											"LoadGen23ControllerFile",
											"Write ('WRM') to controller %s board failed. WRM 0x%X 0x%X -> 0x%X",
											( dBoardId == TIM_ID ? "TIMING" : "UTILITY" ),
											( dType | dAddr ),
											 dData,
											 dReply );
						}

						if ( bAbort ) { inFile.close(); return; }

						//
						// Validate the data if required.
						// --------------------------------------------------------------
						if ( bValidate )
						{
							dReply = Command( dBoardId, RDM, ( dType | dAddr ) );

							if ( dReply != dData )
							{
								CArcTools::ThrowException(
										"CArcPCIe",
										"LoadGen23ControllerFile",
										"Write ('WRM') to controller %s board failed. RDM 0x%X -> 0x%X [ Expected: 0x%X ]",
										( dBoardId == TIM_ID ? "TIMING" : "UTILITY" ),
										( dType | dAddr ),
										 dReply,
										 dData );
							}
						}	// End if 'validate'

						dAddr++;

					} // while tokenizer next
				} // if not EOF or '_'
			}	// if address < 0x4000
		}	// if '_DATA'
	}	// if not EOF

	inFile.close();

	if ( bAbort ) { return; }

	//
	// Clear the PCI status bit #1 (X:0 bit 1 = 0)
	// --------------------------------------------------------------
	// Not Used with PCIe


	if ( bAbort ) { return; }

	//
	//  Tell the TIMING board to jump from boot code to
	//  the uploaded application. Don't check the reply,
	//  since it doesn't return one.
	// +------------------------------------------------+
	if ( bIsCLodFile )
	{
		dReply = Command( TIM_ID, JDL );

		if ( dReply != DON )
		{
			CArcTools::ThrowException(
							"CArcPCIe",
							"LoadGen23ControllerFile",
							"Jump from boot code failed. Reply: 0x%X",
							 dReply );
		}
	}
}


// +----------------------------------------------------------------------------
// |  GetContinuousImageSize
// +----------------------------------------------------------------------------
// |  Returns the boundary adjusted image size for continuous readout.  The PCIe
// |  card ( ARC-66/67 ) requires no boundary adjustments and writes data
// |  continuously.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dImageSize - The boundary adjusted image size ( in bytes ).
// +----------------------------------------------------------------------------
int CArcPCIe::GetContinuousImageSize( int dImageSize )
{
	return dImageSize;
}


// +----------------------------------------------------------------------------
// |  SmallCamDLoad
// +----------------------------------------------------------------------------
// |  Sends a .lod download file data stream of up to 6 values to the SmallCam
// |  controller.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dBoardId  - Must be SMALLCAM_DLOAD_ID
// |  <IN>  -> vData     - Data vector
// +----------------------------------------------------------------------------
int CArcPCIe::SmallCamDLoad( int dBoardId, vector<int>& vData )
{
	int dHeader    = 0;
	int dReply     = 0;

	//
	//  Report error if device reports readout in progress
	// +------------------------------------------------------+
	if ( PCIe_STATUS_READOUT( ReadBar( DEV_REG_BAR, REG_STATUS ) ) )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "SmallCamDLoad",
								   "Device reports readout in progress! Status: 0x%X",
								    ReadBar( DEV_REG_BAR, REG_STATUS ) );
	}

	//
	//  Verify the size of the data, cannot be greater than 6
	// +------------------------------------------------------+
	if ( vData.size() > 6 )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "SmallCamDLoad",
								   "Data vector too large: 0x%X! Must be less than 6!",
								    vData.size() );
	}

	//
	//  Verify the board id equals smallcam download id
	// +------------------------------------------------------+
	if ( dBoardId != SMALLCAM_DLOAD_ID )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "SmallCamDLoad",
								   "Invalid board id: %d! Must be: %d",
								    dBoardId,
									SMALLCAM_DLOAD_ID );
	}

	//
	//  Clear Status Register
	// +-------------------------------------------------+
	ClearStatus();

	try
	{
		//
		//  Send Header
		// +-------------------------------------------------+
		dHeader = ( ( dBoardId << 8 ) | static_cast<int>( vData.size() + 1 ) );

		VERIFY_24BITS( "CArcPCIe",
					   "SmallCamDLoad",
					    dHeader )

		WriteBar( DEV_REG_BAR,
				  REG_CMD_HEADER,
				  ( 0xAC000000 | dHeader ) );

		//
		//  Send the data
		// +-------------------------------------------------+
		for ( vector<int>::size_type i=0; i<vData.size(); i++ )
		{
			VERIFY_24BITS( "CArcPCIe",
						   "SmallCamDLoad",
						    vData.at( i ) )

			WriteBar( DEV_REG_BAR,
					  REG_CMD_COMMAND + ( static_cast<int>( i ) * 4 ),
					  ( 0xAC000000 | vData.at( i ) ) );
		}
	}
	catch ( ... )
	{
		if ( m_bStoreCmds )
		{
			m_cApiLog.Put( FormatDLoadString( dReply,
											  dBoardId,
											  vData ).c_str() );
		}

		throw;
	}

	//
	//  Return the reply
	// +-------------------------------------------------+
	try
	{
		dReply = ReadReply();
	}
	catch ( exception& e )
	{
		if ( m_bStoreCmds )
		{
			m_cApiLog.Put( FormatDLoadString( dReply,
											  dBoardId,
											  vData ).c_str() );
		}

		ostringstream oss;

		oss << e.what() << endl
			<< "Exception Details: 0x"
			<< hex << dHeader << dec;

		for ( vector<int>::size_type i=0; i<vData.size(); i++ )
		{
			oss << " 0x" << hex << vData.at( i ) << dec;
		}

		oss	<< ends;

		CArcTools::ThrowException( "CArcPCIe", "SmallCamDLoad", oss.str() );
	}

	//
	// Set the debug message queue.
	//
	if ( m_bStoreCmds )
	{
		m_cApiLog.Put( FormatDLoadString( dReply,
										  dBoardId,
										  vData ).c_str() );
	}

    return dReply;
}

// +----------------------------------------------------------------------------
// |  SetByteSwapping
// +----------------------------------------------------------------------------
// |  Sets the hardware byte-swapping if system architecture is solaris.
// |  Otherwise, does nothing; compiles to empty function.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCIe::SetByteSwapping()
{
	// Not Used !!!!
}

// +----------------------------------------------------------------------------
// |  ReadReply
// +----------------------------------------------------------------------------
// |  Reads the reply register value. This method will time-out if the
// |  specified number of seconds passes before the reply received register
// |  bit or an error bit ( PCIe time-out, header error, controller reset ) is
// |  set.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> gTimeOutSecs - Seconds to wait before time-out occurs.
// +----------------------------------------------------------------------------
int CArcPCIe::ReadReply( double gTimeOutSecs )
{
	time_t tStart    = time( NULL );
	double gDiffTime = 0.0;
	int    dStatus   = 0;
	int    dReply    = 0;

	do
    {
		dStatus = GetStatus();

		if ( PCIe_STATUS_HDR_ERROR( dStatus ) )
		{
			dReply = HERR;
			break;
		}

		else if( PCIe_STATUS_CONTROLLER_RESET( dStatus ) )
		{
			dReply = SYR;
			break;
		}

		if ( ( gDiffTime = difftime( time( NULL ), tStart ) ) > gTimeOutSecs )
		{
			CArcTools::ThrowException(
							"CArcPCIe",
							"ReadReply",
							"Time Out [ %f sec ] while waiting for status [ 0x%X ]!",
							 gDiffTime,
							 dStatus );
		}

    } while ( !PCIe_STATUS_REPLY_RECVD( dStatus ) );


	if ( dReply != HERR && dReply != SYR )
	{
		dReply = ReadBar( DEV_REG_BAR,
						  REG_CMD_REPLY );
	}

	return dReply;
}

// +----------------------------------------------------------------------------
// |  GetCfgSpByte
// +----------------------------------------------------------------------------
// |  Returns the specified BYTE from the specified PCI configuration space
// |  register.
// |
// |  <IN> -> dOffset - The byte offset from the start of PCI config space
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcPCIe::GetCfgSpByte( int dOffset )
{
	int dRegValue = dOffset;

	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "GetCfgSpByte",
								   "Not connected to any device." );
	}

	int dSuccess = Arc_IOCtl( m_hDevice,
							  ARC_READ_CFG_8,
							  &dRegValue,
							  sizeof( dRegValue ) );

	if ( !dSuccess )
	{
		CArcTools::ThrowException(
						"CArcPCIe",
						"GetCfgSpByte",
						"Reading configuration BYTE offset 0x%X failed : %s",
						 dOffset,
						 CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );
	}

	return dRegValue;
}

// +----------------------------------------------------------------------------
// |  GetCfgSpWord
// +----------------------------------------------------------------------------
// |  Returns the specified WORD from the specified PCI configuration space
// |  register.
// |
// |  <IN> -> dOffset - The byte offset from the start of PCI config space
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcPCIe::GetCfgSpWord( int dOffset )
{
	int dRegValue = dOffset;

	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "GetCfgSpWord",
								   "Not connected to any device." );
	}

	int dSuccess = Arc_IOCtl( m_hDevice,
							  ARC_READ_CFG_16,
							  &dRegValue,
							  sizeof( dRegValue ) );

	if ( !dSuccess )
	{
		CArcTools::ThrowException(
						"CArcPCIe",
						"GetCfgSpWord",
						"Reading configuration WORD offset 0x%X failed : %s",
						 dOffset,
						 CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );
	}

	return dRegValue;
}

// +----------------------------------------------------------------------------
// |  GetCfgSpDWord
// +----------------------------------------------------------------------------
// |  Returns the specified DWORD from the specified PCI configuration space
// |  register.
// |
// |  <IN> -> dOffset - The byte offset from the start of PCI config space
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcPCIe::GetCfgSpDWord( int dOffset )
{
	int dRegValue = dOffset;

	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "GetCfgSpDWord",
								   "Not connected to any device." );
	}

	int dSuccess = Arc_IOCtl( m_hDevice,
							  ARC_READ_CFG_32,
							  &dRegValue,
							  sizeof( dRegValue ) );

	if ( !dSuccess )
	{
		CArcTools::ThrowException(
						"CArcPCIe",
						"GetCfgSpDWord",
						"Reading configuration DWORD offset 0x%X failed : %s",
						 dOffset,
						 CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );
	}

	return dRegValue;
}

// +----------------------------------------------------------------------------
// |  SetCfgSpByte
// +----------------------------------------------------------------------------
// |  Writes the specified BYTE to the specified PCI configuration space
// |  register.
// |
// |  <IN> -> dOffset - The byte offset from the start of PCI config space
// |  <IN> -> dValue  - The BYTE value to write
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCIe::SetCfgSpByte( int dOffset, int dValue )
{
	int dArgs[ 2 ] = { dOffset, dValue };

	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "SetCfgSpByte",
								   "Not connected to any device." );
	}

	int dSuccess = Arc_IOCtl( m_hDevice,
							  ARC_WRITE_CFG_8,
							  dArgs,
							  sizeof( dArgs ) );

	if ( !dSuccess )
	{
		CArcTools::ThrowException(
						"CArcPCIe",
						"SetCfgSpByte",
						"Writing configuration BYTE 0x%X to offset 0x%X failed : %s",
						 dValue,
						 dOffset,
						 CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );
	}
}

// +----------------------------------------------------------------------------
// |  SetCfgSpWord
// +----------------------------------------------------------------------------
// |  Writes the specified WORD to the specified PCI configuration space
// |  register.
// |
// |  <IN> -> dOffset - The byte offset from the start of PCI config space
// |  <IN> -> dValue  - The WORD value to write
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCIe::SetCfgSpWord( int dOffset, int dValue )
{
	int dArgs[ 2 ] = { dOffset, dValue };

	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "SetCfgSpWord",
								   "Not connected to any device." );
	}

	int dSuccess = Arc_IOCtl( m_hDevice,
							  ARC_WRITE_CFG_16,
							  dArgs,
							  sizeof( dArgs ) );

	if ( !dSuccess )
	{
		CArcTools::ThrowException(
						"CArcPCIe",
						"SetCfgSpWord",
						"Writing configuration WORD 0x%X to offset 0x%X failed : %s",
						 dValue,
						 dOffset,
						 CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );
	}
}

// +----------------------------------------------------------------------------
// |  SetCfgSpDWord
// +----------------------------------------------------------------------------
// |  Writes the specified DWORD to the specified PCI configuration space
// |  register.
// |
// |  <IN> -> dOffset - The byte offset from the start of PCI config space
// |  <IN> -> dValue  - The DWORD value to write
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCIe::SetCfgSpDWord( int dOffset, int dValue )
{
	int dArgs[ 2 ] = { dOffset, dValue };

	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "SetCfgSpDWord",
								   "Not connected to any device." );
	}

	int dSuccess = Arc_IOCtl( m_hDevice,
							  ARC_WRITE_CFG_32,
							  dArgs,
							  sizeof( dArgs ) );

	if ( !dSuccess )
	{
		CArcTools::ThrowException(
						"CArcPCIe",
						"SetCfgSpDWord",
						"Writing configuration DWORD 0x%X to offset 0x%X failed : %s",
						 dValue,
						 dOffset,
						 CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );
	}
}

// +----------------------------------------------------------------------------
// |  GetCfgSp
// +----------------------------------------------------------------------------
// |  Reads and parses the entire PCIe configuration space header into readable
// |  text and bit definitions that are stored in a member list variable. The
// |  public methods of this class allow access to this list. This method will
// |  create the member list if it doesn't already exist and clears it if it
// |  does.
// +----------------------------------------------------------------------------
void CArcPCIe::GetCfgSp()
{
	int	dRegValue = 0;
	int	dRegAddr  = 0;

	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "GetCfgSp",
								   "Not connected to any device." );
	}

	CArcPCIBase::GetCfgSp();

    dRegAddr  = PCI9056_PM_CAP_ID;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				dRegAddr,
                "Power Management Capability / Next Item Ptr / Capability ID",
                dRegValue );

    dRegAddr  = PCI9056_PM_CSR;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				dRegAddr,
                "PM Cap: PM Data / Bridge Ext / PM Control & Status",
                dRegValue );

    dRegAddr  = PCI9056_HS_CAP_ID;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				dRegAddr,
                "Hot Swap Capability / Next Item Pointer / Capability ID",
                dRegValue );

    dRegAddr  = PCI9056_VPD_CAP_ID;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				dRegAddr,
                "VPD Capability / VPD Address / Next Item Ptr / Capability ID",
                dRegValue );

    dRegAddr  = PCI9056_VPD_DATA;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				dRegAddr,
                "VPD Data",
                dRegValue );
}

// +----------------------------------------------------------------------------
// |  GetBarSp
// +----------------------------------------------------------------------------
// |  Reads and parses the entire PCIe Base Address Registers (BAR) into
// |  readable text and bit definitions that are stored in a member list
// |  variable. The public methods of this class allow access to this list.
// |  This method will create the member list if it doesn't already exist and
// |  clears it if it does. NOTE: Not all BARS or PCI boards have data.
// +----------------------------------------------------------------------------
void CArcPCIe::GetBarSp()
{
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "GetBarSp",
								   "Not connected to any device." );
	}

	CArcPCIBase::GetBarSp();

    //
    //  Access the register data
    // +-------------------------------------------------------+
    GetLocalConfiguration();
}

// +----------------------------------------------------------------------------
// |  GetLocalConfiguration
// +----------------------------------------------------------------------------
// |  Reads and parses the entire PLX PCIe local registers located within BAR0.
// +----------------------------------------------------------------------------
void CArcPCIe::GetLocalConfiguration()
{
	int	dRegValue;

    if ( m_pBarList == NULL )
    {
		CArcTools::ThrowException(
						"CArcPCIe",
						"GetLocalConfiguration",
						"Unable to read PCI/e base address register!" );
    }

    //
    // Get and Add PLX Local Configuration Registers
    //
    PCIRegList* pList = new PCIRegList();

    for ( int i=0; i<int( sizeof( LCRMap ) / sizeof( PLXRegItem ) ); i++ )
    {
        dRegValue = ReadBar( 0, LCRMap[ i ].dAddr );

		AddRegItem( pList,
					LCRMap[ i ].dAddr,
                    LCRMap[ i ].sText.c_str(),
					dRegValue );
    }

    AddBarItem( LCRMapName, pList );

    //
    // Get and Add PLX Runtime Registers
    //
    pList = new PCIRegList();

    for ( int i=0; i<int( sizeof( RTRMap ) / sizeof( PLXRegItem ) ); i++ )
    {
        dRegValue = ReadBar( 0, RTRMap[ i ].dAddr );

        if ( RTRMap[ i ].dAddr == PCI9056_PERM_VENDOR_ID )
        {
			AddRegItem( pList,
						RTRMap[ i ].dAddr,
                        RTRMap[ i ].sText.c_str(),
						dRegValue,
                        GetDevVenBitList( dRegValue ) );
        }
        else
        {
 			AddRegItem( pList,
						RTRMap[ i ].dAddr,
                        RTRMap[ i ].sText.c_str(),
						dRegValue );
        }
    }

    AddBarItem( RTRMapName, pList );

    //
    // Get and Add PLX DMA Registers
    //
    pList = new PCIRegList();

    for ( int i=0; i<int( sizeof( DMAMap ) / sizeof( PLXRegItem ) ); i++ )
    {
        dRegValue = ReadBar( 0, DMAMap[ i ].dAddr );

		AddRegItem( pList,
					DMAMap[ i ].dAddr,
                    DMAMap[ i ].sText.c_str(),
					dRegValue );
    }

    AddBarItem( DMAMapName, pList );

    //
    // Get and Add PLX Messaging Queue Registers
    //
    pList = new PCIRegList();

    for ( int i=0; i<int( sizeof( MSQMap ) / sizeof( PLXRegItem ) ); i++ )
    {
        dRegValue = ReadBar( 0, MSQMap[ i ].dAddr );

		AddRegItem( pList,
					MSQMap[ i ].dAddr,
                    MSQMap[ i ].sText.c_str(),
					dRegValue );
    }

    AddBarItem( MSQMapName, pList );

    return;
}
