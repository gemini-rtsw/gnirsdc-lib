#ifdef WIN32
	#define INITGUID
#endif


#ifdef WIN32
	#include <windows.h>
	#include <setupapi.h>
	#include <devguid.h>
	#include <regstr.h>
	#include "astropciGUID.h"

#elif defined( linux ) || defined( __linux ) || defined( __APPLE__ )
	#include <sys/types.h>
	#include <sys/mman.h>
	#include <sys/ioctl.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <errno.h>
	#include <dirent.h>
	#include <cstring>

#else						// Unix Systems Only
	#include <sys/types.h>
	#include <sys/mman.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <stropts.h>
	#include <errno.h>
	#include <dirent.h>
#endif

#include <iostream>

#include <sstream>
#include <fstream>
#include <cmath>
#include <ios>
#include "CArcPCI.h"
#include "CArcTools.h"
#include "ArcOSDefs.h"
#include "ArcDefs.h"
#include "PCIRegs.h"


#if defined( linux ) || defined( __linux )

	#define DEVICE_DIR			"/dev/"
	#define DEVICE_NAME			"AstroPCI"
	#define DEVICE_NAME_ALT		"Arc64PCI"

#elif defined( __APPLE__ )

// +----------------------------------------------------------------------------
// | Define driver names
// +----------------------------------------------------------------------------
#define AstroPCIClassName		 com_arc_driver_AstroPCI
#define kAstroPCIClassName		"com_arc_driver_AstroPCI"

// +----------------------------------------------------------------------------
// |  Macro to validate device and vendor id's
// +----------------------------------------------------------------------------
#define VALID_DEV_VEN_ID( devVenId )	\
			( ( devVenId == 0x1057 || devVenId == 0x1801 ) ? 1 : 0 )

#endif


// +----------------------------------------------------------------------------
// |  PCI File Download Constants
// +----------------------------------------------------------------------------
#define HTF_MASK					0x200
#define HTF_CLEAR_MASK				0xFFFFFCFF	// Bits 8 and 9
#define MAX_PCI_COMM_TEST			3
#define PCI_COM_TEST_VALUE			0xABC123


using namespace std;
using namespace arc;
using namespace arc::device;


//#include <fstream>
//std::ofstream dbgStream1( "CArcPCI_Debug.txt" );


// +----------------------------------------------------------------------------
// |  Initialize Static Class Members
// +----------------------------------------------------------------------------
vector<ArcDev_t> CArcPCI::m_vDevList;
shared_ptr<string> CArcPCI::m_psDevList;


// +---------------------------------------------------------------------------+
// | ArrayDeleter                                                              |
// +---------------------------------------------------------------------------+
// | Called by std::shared_ptr to delete the temporary image buffer.      |
// | This method should NEVER be called directly by the user.                  |
// +---------------------------------------------------------------------------+
template<typename T> void CArcPCI::ArrayDeleter( T* p )
{
	if ( p != NULL )
	{
		delete [] p;
	}
}


// +----------------------------------------------------------------------------
// |  Constructor
// +----------------------------------------------------------------------------
// |  See CArcPCI.h for the class definition
// +----------------------------------------------------------------------------
CArcPCI::CArcPCI( void )
{
	m_hDevice = INVALID_HANDLE_VALUE;
}

// +----------------------------------------------------------------------------
// |  Destructor
// +----------------------------------------------------------------------------
CArcPCI::~CArcPCI( void )
{
	Close();
}

// +----------------------------------------------------------------------------
// |  ToString
// +----------------------------------------------------------------------------
// |  Returns a string that represents the device controlled by this library.
// +----------------------------------------------------------------------------
const string CArcPCI::ToString()
{
	return string( "PCI [ ARC-63 / 64 ]" );
}

// +----------------------------------------------------------------------------
// |  FindDevices
// +----------------------------------------------------------------------------
// |  Searches for available ARC, Inc PCI devices and stores the list, which
// |  can be accessed via device number ( 0,1,2... ).
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCI::FindDevices()
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
	
	//
	// Dump any existing bindings
	//
    if ( !m_vDevList.empty() )
	{
        m_vDevList.clear();
		
        if ( m_vDevList.size() > 0 )
		{
			CArcTools::ThrowException( "CArcPCI",
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

	HardwareDeviceInfo = SetupDiGetClassDevs( ( LPGUID )&GUID_DEVINTERFACE_ARC_PCI,
											   NULL,
											   NULL,
											  ( DIGCF_PRESENT | DIGCF_DEVICEINTERFACE ) );

	if ( HardwareDeviceInfo == INVALID_HANDLE_VALUE )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "FindDevices",
								   "SetupDiGetClassDevs failed!" );
	}

	DeviceInterfaceData.cbSize = sizeof( SP_DEVICE_INTERFACE_DATA );

	while ( 1 )
	{
		bResult = SetupDiEnumDeviceInterfaces( HardwareDeviceInfo,
											   0,
											   ( LPGUID )&GUID_DEVINTERFACE_ARC_PCI,
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
															IOServiceMatching( kAstroPCIClassName ),
															&iterator );
	
    if ( kernResult != KERN_SUCCESS )
	{
		CArcTools::ThrowException( "CArcPCI",
								  "FindDevices",
								  "IOServiceGetMatchingServices failed: 0x%X",
								  kernResult );
    }
	
    while ( ( service = IOIteratorNext( iterator ) ) != IO_OBJECT_NULL )
	{
		ArcDev_t tArcDev;
		
		tArcDev.sName			= kAstroPCIClassName;
		tArcDev.tService		= service;
		
		m_vDevList.push_back( tArcDev );
	}
	
	//
    // Release the io_iterator_t now that we're done with it.
	//
    IOObjectRelease( iterator );
	
#else	// LINUX
	
	struct dirent *pDirEntry = NULL;
	DIR *pDir = NULL;

	pDir = opendir( DEVICE_DIR );

	if ( pDir == NULL )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "FindDevices",
								   "Failed to open dir: %s",
									DEVICE_DIR );
	}
	else
	{
		while ( ( pDirEntry = readdir( pDir ) ) != NULL )
		{
			std::string sDirEntry( pDirEntry->d_name );

			if ( ( sDirEntry.find( DEVICE_NAME ) != std::string::npos ||
				   sDirEntry.find( DEVICE_NAME_ALT ) != std::string::npos ) &&
				   sDirEntry.find( "PCIe" ) == std::string::npos )
			{
				ArcDev_t tArcDev;
				tArcDev.sName = DEVICE_DIR + sDirEntry;
				m_vDevList.push_back( tArcDev );
			}
		}

		closedir( pDir );
	}

#endif

	//
	// Make sure the bindings exist
	//
	if ( m_vDevList.empty() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "FindDevices",
								   string( "No device bindings exist!" ) +
								   string( "Make sure an ARC, Inc PCI card is installed!" ) );
	}
}


// +----------------------------------------------------------------------------
// |  DeviceCount
// +----------------------------------------------------------------------------
// |  Returns the number of items in the device list. Must be called after
// |  FindDevices().
// +----------------------------------------------------------------------------
int CArcPCI::DeviceCount()
{
	return static_cast<int>( m_vDevList.size() );
}

// +----------------------------------------------------------------------------
// |  GetDeviceStringList
// +----------------------------------------------------------------------------
// |  Returns a string list representation of the device list. Must be called
// |  after FindDevices().
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
const string* CArcPCI::GetDeviceStringList()
{
    if ( !m_vDevList.empty() )
    {
		m_psDevList.reset( new string[ m_vDevList.size() ], &CArcPCI::ArrayDeleter<string> );

		for ( string::size_type i=0; i<m_vDevList.size(); i++ )
        {
			ostringstream oss;

#ifdef __APPLE__
			oss << "PCI Device " << i
				<< m_vDevList.at( i ).sName
				<< ends;
#else
			oss << "PCI Device " << i << ends;
#endif
			( m_psDevList.get() )[ i ] = string( oss.str() );
        }
    }
    else
    {
		m_psDevList.reset( new string[ 1 ], &CArcPCI::ArrayDeleter<string> );

		( m_psDevList.get() )[ 0 ] = string( "No Devices Found!" );
    }

	return const_cast<const string *>( m_psDevList.get() );
}


// +----------------------------------------------------------------------------
// |  IsOpen
// +----------------------------------------------------------------------------
// |  Returns 'true' if connected to PCI device; 'false' otherwise.
// |
// |  Throws NOTHING on error. No error handling.
// +----------------------------------------------------------------------------
bool CArcPCI::IsOpen()
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
void CArcPCI::Open( int dDeviceNumber )
{
	if ( IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCIe",
								   "Open",
								   "Device already open, call Close() first!" );
	}

	//
	// Make sure the bindings exist
	//
	if ( m_vDevList.empty() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "Open",
								   "No device bindings exist!" );
	}

	// Verify device number
	//
	if ( dDeviceNumber < 0 || dDeviceNumber > int( m_vDevList.size() ) )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "Open",
								   "Invalid device number: %d",
								    dDeviceNumber );
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
		CArcTools::ThrowException( "CArcPCI",
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
void CArcPCI::Open( int dDeviceNumber, int dBytes )
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
void CArcPCI::Open( int dDeviceNumber, int dRows, int dCols )
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
void CArcPCI::Close()
{
	//
	// Prevents access violation from code that follows
	//
	bool bOldStoreCmds = m_bStoreCmds;
	m_bStoreCmds       = false;

	UnMapCommonBuffer();

	Arc_CloseHandle( m_hDevice );

 	m_hDevice    = INVALID_HANDLE_VALUE;
	m_bStoreCmds = bOldStoreCmds;
}

// +----------------------------------------------------------------------------
// |  Reset
// +----------------------------------------------------------------------------
// |  Resets the PCI board.
// |
// |  Throws NOTHING on error. No error handling.
// +----------------------------------------------------------------------------
void CArcPCI::Reset()
{
	int dReply = PCICommand( PCI_RESET );

	if ( dReply != DON )
	{
		CArcTools::ThrowException(
				"CArcPCI",
				"Reset",
				"PCI reset failed! Expected: 'DON' [ 0x444F4E ], Received: 0x%X",
				 dReply );
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
// |                    used by PCI.
// +----------------------------------------------------------------------------
void CArcPCI::MapCommonBuffer( int dBytes )
{
	if ( dBytes <= 0 )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "MapCommonBuffer",
								   "Invalid buffer size: %d. Must be greater than zero!",
								    dBytes );		
	}

	m_tImgBuffer.pUserAddr = Arc_MMap( m_hDevice,
									   ASTROPCI_MEM_MAP,
									   size_t( dBytes ) );

	if ( m_tImgBuffer.pUserAddr == MAP_FAILED )
	{
		Arc_ZeroMemory( &m_tImgBuffer, sizeof( ImgBuf_t ) );

		CArcTools::ThrowException( "CArcPCI",
								   "MapCommonBuffer",
								   "Failed to map image buffer : [ %d ] %s",
								    Arc_ErrorCode(),
									CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );		
	}

	bool bSuccess = GetCommonBufferProperties();

	if ( !bSuccess )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "MapCommonBuffer",
								   "Failed to read image buffer size : [ %d ] %s",
								    Arc_ErrorCode(),
									CArcTools::GetSystemMessage( Arc_ErrorCode() ).c_str() );		
	}

	if ( m_tImgBuffer.dSize < size_t( dBytes ) )
	{
		ostringstream oss;

		oss << "Failed to allocate buffer of the correct size.\nWanted: "
			<< dBytes << " bytes [ " << ( dBytes / 1E6 ) << "MB ] - Received: "
			<< m_tImgBuffer.dSize << " bytes [ " << ( m_tImgBuffer.dSize / 1E6 )
			<< "MB ]";

		CArcTools::ThrowException( "CArcPCI",
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
void CArcPCI::UnMapCommonBuffer()
{
	if ( m_tImgBuffer.pUserAddr != ( void * )NULL )
	{
		Arc_MUnMap( m_hDevice,
					ASTROPCI_MEM_UNMAP,
					m_tImgBuffer.pUserAddr,
					static_cast<int>( m_tImgBuffer.dSize ) );
	}
	
	Arc_ZeroMemory( &m_tImgBuffer, sizeof( ImgBuf_t ) );
}

// +----------------------------------------------------------------------------
// |  GetId
// +----------------------------------------------------------------------------
// |  Returns 0, since the PCI board has no ID!
// +----------------------------------------------------------------------------
int CArcPCI::GetId()
{
	return 0;
}

// +----------------------------------------------------------------------------
// |  GetStatus
// +----------------------------------------------------------------------------
// |  Returns the current value of the PCI boards HTF ( Host Transfer Flags )
// |  bits from the HSTR register.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcPCI::GetStatus()
{
	int dRetVal = GetHstr();

	dRetVal = ( dRetVal & HTF_BIT_MASK ) >> 3;

	return dRetVal;
}

// +----------------------------------------------------------------------------
// |  ClearStatus
// +----------------------------------------------------------------------------
// |  Clears the PCI status register.
// +----------------------------------------------------------------------------
void CArcPCI::ClearStatus()
{
	// Not Used ????
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
void CArcPCI::Set2xFOTransmitter( bool bOnOff )
{
	int dReply = 0;

	if ( bOnOff )
	{
		if ( ( dReply = Command( TIM_ID, XMT, 1 ) ) != DON )
		{
			CArcTools::ThrowException(
						"CArcPCI",
						"Set2xFOTransmitter",
						"Failed to SET use of 2x fiber optic transmitters on controller, reply: 0x%X",
						 dReply );
		}
	}
	else
	{
		if ( ( dReply = Command( TIM_ID, XMT, 0 ) ) != DON )
		{
			CArcTools::ThrowException(
						"CArcPCI",
						"Set2xFOTransmitter",
						"Failed to CLEAR use of 2x fiber optic transmitters on controller, reply: 0x%X",
						 dReply );
		}
	}
}

// +----------------------------------------------------------------------------
// |  LoadDeviceFile
// +----------------------------------------------------------------------------
// |  Loads a PCI '.lod' file into the PCI boards DSP for execution, which
// |  begins immediately following upload.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> sFile - The PCI '.lod' file to load.
// +----------------------------------------------------------------------------
void CArcPCI::LoadDeviceFile( const string sFilename )
{
	int dHctrValue	      	 = 0;
	int dWordTotal	      	 = 0;
	int dWordCount			 = 0;
	int dStartAddr			 = 0;
	int dData				 = 0;
	int dFailedCount		 = 0;
	bool bPCITransferModeSet = false;
	std::string sToken	  	 = "";

	CArcTools::CTokenizer cTokenizer;
	std::string sLine;

	//
	// Open the file for reading
	// -------------------------------------------------------------------
	ifstream inFile( sFilename.c_str() );

	if ( !inFile.is_open() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "LoadDeviceFile",
								   "Cannot open file: %s",
								    sFilename.c_str() );
	}

	//
	// Check for valid PCI file
	// -------------------------------------------------------------------
	getline( inFile, sLine );

	if ( sLine.find( "PCI" ) == std::string::npos )
	{
		CArcTools::ThrowException(
						"CArcPCI",
						"LoadDeviceFile",
						"Invalid PCI file, no PCIBOOT string found." );
	}

	//
	// Verify device connection
	// -------------------------------------------------------------------
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "LoadDeviceFile",
								   "Not connected to any device." );
	}

	//
	// Set the PCI board HCTR bits. Bits 8 & 9 (HTF bits) are cleared to
	// allow 32-bit values to be written to the PCI board without loss
	// of bytes. The 32-bit values are broken up into two 16-bit values.
	// -------------------------------------------------------------------
	dHctrValue = GetHctr();

	//
	// Clear the HTF bits (8 and 9) and bit3
	//
	dHctrValue = dHctrValue & HTF_CLEAR_MASK;

	//
	// Set the HTF bits
	//
	dHctrValue = dHctrValue | HTF_MASK;
	SetHctr( dHctrValue );
	bPCITransferModeSet = true;

	//
	// Inform the DSP that new pci boot code will be downloaded
	//
	IoctlDevice( ASTROPCI_PCI_DOWNLOAD, 0 );

	//
	// Set the magic number that indicates a pci download
	//
	IoctlDevice( ASTROPCI_HCVR_DATA, 0x00555AAA );

	//
	// Write the data to the PCI
	//
	while ( !inFile.eof() )
	{
		getline( inFile, sLine );

		if ( sLine.find( "_DATA P" ) != std::string::npos )
		{
			getline( inFile, sLine );
			cTokenizer.Victim( sLine );

			//
			// Set the number of words and start address
			//
			dWordTotal = CArcTools::StringToHex( cTokenizer.Next() );
			IoctlDevice( ASTROPCI_HCVR_DATA, dWordTotal );

			dStartAddr = CArcTools::StringToHex( cTokenizer.Next() );
			IoctlDevice( ASTROPCI_HCVR_DATA, dStartAddr );

			//
			// Throw away the next line (example: _DATA P 000002)
			//
			getline( inFile, sLine );

			//
			// Load the data
			//
			while ( dWordCount < dWordTotal )
			{
				//
				// Get the next line, this is the data start
				//
				getline( inFile, sLine );

				//
				// Check for intermixed "_DATA" strings
				//
				if ( sLine.find( "_DATA P" ) == std::string::npos )
				{
					cTokenizer.Victim( sLine );

					while( !( ( sToken = cTokenizer.Next() ).empty() ) )
					{
						if ( dWordCount >= dWordTotal )
							break;

						dData = CArcTools::StringToHex( sToken );
						IoctlDevice( ASTROPCI_HCVR_DATA, dData );
						dWordCount++;
					}
				}	// End if _DATA P
			}	// End while wordCount < wordTotal
			
			break;

		}	// End if strstr != null
	}	// End while not eof

	//
	// Set the PCI data size transfer mode
	//
	if ( bPCITransferModeSet )
	{
		dHctrValue = GetHctr();
		SetHctr( ( dHctrValue & 0xCFF ) | 0x900 );
	}

	//
	// Wait for the PCI DSP to finish initialization
	//
	if ( bPCITransferModeSet )
	{
		int dRetVal = IoctlDevice( ASTROPCI_PCI_DOWNLOAD_WAIT,
								   PCI_ID );

		//
		// Make sure a DON is received
		//
		if ( dRetVal != DON )
		{
			CArcTools::ThrowException( "CArcPCI",
									   "LoadDeviceFile",
									   "PCI download failed. Reply: 0x%X",
										dRetVal );
		}
	}

	//
	// Test PCI communications
	//
	int dReply = 0;

	for ( int i=0; i<MAX_PCI_COMM_TEST; i++ )
	{
		dData  = PCI_COM_TEST_VALUE * i;
		dReply = Command( PCI_ID, TDL, dData );

		if ( dReply != dData )
			dFailedCount++;
	}

	//
	// If ALL the communication tests failed, then report an error.
	//
	if ( dFailedCount >= MAX_PCI_COMM_TEST )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "LoadDeviceFile",
								   "PCI communications test failed." );
	}

	inFile.close();
}


// +----------------------------------------------------------------------------
// |  Command
// +----------------------------------------------------------------------------
// |  Send a command to the controller timing or utility board. Returns the
// |  controller reply, typically DON.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dBoardId       - Command board id ( PCI, TIM or UTIL )
// |  <IN>  -> dCommand       - Board command
// |  <IN>  -> dArg0 to dArg3 - Command arguments ( optional )
// +----------------------------------------------------------------------------
int CArcPCI::Command( int dBoardId, int dCommand, int dArg0, int dArg1, int dArg2, int dArg3 )
{
	int dCmdData[ CTLR_CMD_MAX ]	= { -1 };
	int dNumberOfArgs				= 0;
	int dHeader						= 0;
	int dReply						= 0;

	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "Command",
								   "Not connected to any device!" );
	}

	if      ( dArg0 == -1 ) dNumberOfArgs = 2;
	else if ( dArg1 == -1 ) dNumberOfArgs = 3;
	else if ( dArg2 == -1 ) dNumberOfArgs = 4;
	else if ( dArg3 == -1 ) dNumberOfArgs = 5;
	else dNumberOfArgs = 6;

	dHeader       = ( ( dBoardId << 8 ) | dNumberOfArgs );
	dCmdData[ 0 ] = dHeader;
	dCmdData[ 1 ] = dCommand;
	dCmdData[ 2 ] = dArg0;
	dCmdData[ 3 ] = dArg1;
	dCmdData[ 4 ] = dArg2;
	dCmdData[ 5 ] = dArg3;

	int dSuccess = Arc_IOCtl( m_hDevice,
							  ASTROPCI_COMMAND,
							  dCmdData,
							  sizeof( dCmdData ) );

	dReply = dCmdData[ 0 ];

	if ( !dSuccess )
	{
		string sCmd = CArcTools::CmdToString( dReply,
											  dBoardId,
			 								  dCommand,
											  dArg0,
											  dArg1,
											  dArg2,
											  dArg3,
											  Arc_ErrorCode() );
		if ( m_bStoreCmds )
		{
			m_cApiLog.Put( sCmd.c_str() );
		}

		CArcTools::ThrowException( "CArcPCI",
								   "Command",
								    sCmd );
    }

	// Set the debug message queue. Can't pass dCmdData[ 0 ] because
	// linux/unix systems overwrite this in the driver with the reply.
	if ( m_bStoreCmds )
	{
		string sCmd = CArcTools::CmdToString( dReply,
											  dBoardId,
			 								  dCommand,
											  dArg0,
											  dArg1,
											  dArg2,
											  dArg3 );
		m_cApiLog.Put( sCmd.c_str() );
	}

	if ( dReply == CNR )
	{
		CArcTools::ThrowException(
						"CArcPCI",
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
int CArcPCI::GetControllerId()
{
	//
	//  Check the controller id  ... And YES,
	//  PCI_ID is correct for ARC-12!
	// +--------------------------------------+
	int dId = Command( PCI_ID, SID );

	if ( !IS_ARC12( dId ) )
	{
		dId = Command( TIM_ID, SID );
	}

	return dId;
}

// +----------------------------------------------------------------------------
// |  ResetController
// +----------------------------------------------------------------------------
// |  Resets the controller.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCI::ResetController()
{
	int dRetVal = PCICommand( RESET_CONTROLLER );

	if ( dRetVal != SYR )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "ResetController",
								   "Reset controller failed. Reply: 0x%X",
									dRetVal );
	}
}

// +----------------------------------------------------------------------------
// | IsControllerConnected
// +----------------------------------------------------------------------------
// |  Returns 'true' if a controller is connected to the PCI board. This is
// |  tested by sending a TDL to the controller. If it succeeds, then the
// |  controller is ready.
// |
// |  Throws std::runtime_error on error ( indirectly via Command() )
// +----------------------------------------------------------------------------
bool CArcPCI::IsControllerConnected()
{
	bool bIsSetup	= false;
	int dVal     	= 0x112233;
	int dRetVal  	= 0;

	try
	{
		dRetVal = Command( TIM_ID, TDL, dVal );

		if ( dRetVal == dVal )
		{
			bIsSetup = true;
		}
	}
	catch ( ... ) {}

	return bIsSetup;
}

// +----------------------------------------------------------------------------
// |  StopExposure
// +----------------------------------------------------------------------------
// |  Stops the current exposure.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCI::StopExposure()
{
	PCICommand( ABORT_READOUT );
}


// +----------------------------------------------------------------------------
// |  IsReadout
// +----------------------------------------------------------------------------
// |  Returns 'true' if the controller is in readout.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
bool CArcPCI::IsReadout()
{
	int dStatus = ( GetHstr() & HTF_BIT_MASK ) >> 3;

	return ( dStatus == READOUT_STATUS ? true : false );
}

// +----------------------------------------------------------------------------
// |  GetPixelCount
// +----------------------------------------------------------------------------
// |  Returns the current pixel count.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcPCI::GetPixelCount()
{
	int dRetVal = 0;

	//
	// Verify device connection
	// -------------------------------------------------------------------
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "GetPixelCount",
								   "Not connected to any device." );
	}
	else
	{
		dRetVal = IoctlDevice( ASTROPCI_GET_PROGRESS );
	}
	return dRetVal;
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
int CArcPCI::GetCRPixelCount()
{
	return IoctlDevice( ASTROPCI_GET_CR_PROGRESS );
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
int CArcPCI::GetFrameCount()
{
	return IoctlDevice( ASTROPCI_GET_FRAMES_READ );
}

// +----------------------------------------------------------------------------
// |  SetHctr
// +----------------------------------------------------------------------------
// |  Sets the current value of the PCI boards HCTR register.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> dVal - The value to set the PCI board HCTR register to.
// +----------------------------------------------------------------------------
void CArcPCI::SetHctr( int dVal )
{
	//
	// Verify device connection
	// -------------------------------------------------------------------
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "SetHctr",
								   "Not connected to any device." );
	}
	else
	{
		IoctlDevice( ASTROPCI_SET_HCTR, dVal );
	}
}

// +----------------------------------------------------------------------------
// |  GetHstr
// +----------------------------------------------------------------------------
// |  Returns the current value of the PCI boards HSTR register.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcPCI::GetHstr()
{
	int dRetVal = 0;

	//
	// Verify device connection
	// -------------------------------------------------------------------
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "GetHstr",
								   "Not connected to any device." );
	}
	else
	{
		dRetVal = IoctlDevice( ASTROPCI_GET_HSTR );
	}

	return dRetVal;
}

// +----------------------------------------------------------------------------
// |  GetHctr
// +----------------------------------------------------------------------------
// |  Returns the current value of the PCI boards HCTR register.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcPCI::GetHctr()
{
	int dRetVal = 0;

	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "GetHctr",
								   "Not connected to any device." );
	}
	else
	{
		dRetVal = IoctlDevice( ASTROPCI_GET_HCTR );
	}

	return dRetVal;
}

// +----------------------------------------------------------------------------
// |  PCICommand
// +----------------------------------------------------------------------------
// |  Send a command to the PCI board.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> command - PCI command
// +----------------------------------------------------------------------------
int CArcPCI::PCICommand( int dCommand )
{
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "PCICommand",
								   "Not connected to any device." );
	}

	int dRetVal = dCommand;
	int dSuccess = Arc_IOCtl( m_hDevice,
							  ASTROPCI_SET_HCVR,
							  &dRetVal,
							  sizeof( dRetVal ) );

	if ( !dSuccess )
	{
		if ( m_bStoreCmds )
		{
			string sCmd = FormatPCICommand( dCommand,
											dRetVal );

			m_cApiLog.Put( sCmd.c_str() );
		}

		const string sErr = FormatPCICommand( dCommand,
											  dRetVal,
											  Arc_ErrorCode() );

		CArcTools::ThrowException( "CArcPCI",
								   "PCICommand",
								    sErr.c_str() );
	}

	// Set the debug message queue
	if ( m_bStoreCmds )
	{
		string sCmd = FormatPCICommand( dCommand,
										dRetVal );

		m_cApiLog.Put( sCmd.c_str() );
	}

	return dRetVal;
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
void CArcPCI::LoadGen23ControllerFile( const string sFilename, bool bValidate, const bool& bAbort )
{
	int  dBoardId		= 0;
	int  dType			= 0;
	int  dAddr			= 0;
	int  dData			= 0;
	int  dReply			= 0;
	int  dPciStatus		= 0;
	char typeChar		= ' ';
	bool bPciStatusSet	= false;
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
		CArcTools::ThrowException( "CArcPCI",
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
		CArcTools::ThrowException( "CArcPCI",
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
					"CArcPCI",
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
					"CArcPCI",
					"LoadGen23ControllerFile",
					"Stop ('STP') controller failed. Reply: 0x%X",
					 dReply );
	}

	if ( bAbort ) { inFile.close(); return; }

	//
	// Set the PCI status bit #1 (X:0 bit 1 = 1).
	// -------------------------------------------
	bPciStatusSet = true;
	dPciStatus = Command( PCI_ID, RDM, ( X_MEM | 0 ) );

	dReply = Command( PCI_ID,
					  WRM,
					  ( X_MEM | 0 ),
					  ( dPciStatus | 0x00000002 ) );

	if ( dReply != DON )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "LoadGen23ControllerFile",
								   "Set PCI status bit 1 failed. Reply: 0x%X",
									dReply );
	}

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
											"CArcPCI",
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
										"CArcPCI",
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
	//
	// NOTE: This should be undone for sure. Otherwise, the board may
	// be stuck until the computer is turned off and unplugged!! Also,
	// "cmdStatus" is used in place of "status" because this set of
	// commands may override "status" with API_OK, when it should be
	// API_ERROR from a previous error.
	
	if ( bPciStatusSet )
	{
		int dPciStatus = Command( PCI_ID, RDM, ( X_MEM | 0 ) );

		dReply = Command( PCI_ID,
						  WRM,
						  ( X_MEM | 0 ),
						  ( dPciStatus & 0xFFFFFFFD ) );

		if ( dReply != DON )
		{
			CArcTools::ThrowException( "CArcPCI",
									   "LoadGen23ControllerFile",
									   "Clear PCI status bit 1 failed. Reply: 0x%X",
										dReply );
		}
	}

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
							"CArcPCI",
							"LoadGen23ControllerFile",
							"Jump from boot code failed. Reply: 0x%X",
							 dReply );
		}
	}
}


// +----------------------------------------------------------------------------
// |  GetContinuousImageSize
// +----------------------------------------------------------------------------
// |  Returns the boundary adjusted image size for continuous readout.  The PCI
// |  card ( ARC-63/64 ) requires that each image within the image buffer starts
// |  on a 1024 byte boundary.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dImageSize - The boundary adjusted image size ( in bytes ).
// +----------------------------------------------------------------------------
int CArcPCI::GetContinuousImageSize( int dImageSize )
{
	int dBoundedImageSize = 0;

	if ( ( dImageSize & 0x3FF ) != 0 )
	{
		dBoundedImageSize = dImageSize - ( dImageSize & 0x3FF ) + 1024;
	}
	else
	{
		dBoundedImageSize = dImageSize;
	}

	return dBoundedImageSize;
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
int CArcPCI::SmallCamDLoad( int dBoardId, vector<int>& vData )
{
	int dHeader	= 0;
	int dReply	= 0;

	//
	//  Report error if device reports readout in progress
	// +------------------------------------------------------+
	if ( IsReadout() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "SmallCamDLoad",
								   "Device reports readout in progress! Status: 0x%X",
								    GetStatus() );
	}

	//
	//  Verify the size of the data, cannot be greater than 6
	// +------------------------------------------------------+
	if ( vData.size() > 6 )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "SmallCamDLoad",
								   "Data vector too large: 0x%X! Must be less than 6!",
								    vData.size() );
	}

	//
	//  Verify the board id equals smallcam download id
	// +------------------------------------------------------+
	if ( dBoardId != SMALLCAM_DLOAD_ID )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "SmallCamDLoad",
								   "Invalid board id: %d! Must be: %d",
								    dBoardId,
									SMALLCAM_DLOAD_ID );
	}

	try
	{
		//
		//  Send Header
		// +-------------------------------------------------+
		dHeader = ( ( dBoardId << 8 ) | static_cast<int>( vData.size() + 1 ) );

		IoctlDevice( ASTROPCI_HCVR_DATA, dHeader );

		//
		//  Send the data
		// +-------------------------------------------------+
		for ( vector<int>::size_type i=0; i<vData.size(); i++ )
		{
			IoctlDevice( ASTROPCI_HCVR_DATA,
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
		dReply = ( GetHstr() & HTF_BIT_MASK ) >> 3;
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
			<< "Exception Details:" << endl
			<< "0x" << hex << dHeader << dec;

		for ( vector<int>::size_type i=0; i<vData.size(); i++ )
		{
			oss << " 0x" << hex << vData.at( i ) << dec;
		}

		oss	<< ends;

		CArcTools::ThrowException( "CArcPCI", "SmallCamDLoad", oss.str() );
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
// |  Turns the PCI hardware byte-swapping on if system architecture is solaris.
// |  Otherwise, does nothing; compiles to empty function.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCI::SetByteSwapping()
{
#if !defined( WIN32 ) && !defined( linux ) && !defined( __linux ) && !defined( __APPLE__ )

	//
	// Test if byte swapping is available
	//
	int dReply = Command( PCI_ID, TBS );

	if ( ContainsError( dReply ) )
	{
		string sCmdMsg = CArcTools::CmdToString( dReply, PCI_ID, TBS );

		CArcTools::ThrowException( "CArcPCI",
								   "SetPCIByteSwapping",
								    sCmdMsg );
	}

	//
	// Turn hardware byte swapping ON
	//
	dReply = Command( PCI_ID, SBS, 1 );

	if ( ContainsError( dReply ) )
	{
		string sCmdMsg = CArcTools::CmdToString( dReply, PCI_ID, SBS, 1 );

		CArcTools::ThrowException( "CArcPCI",
								   "SetPCIByteSwapping",
								    sCmdMsg );
	}

	if ( m_bStoreCmds )
	{
		m_cApiLog.Put( ( char * )"Hardware byte swapping on!" );
	}

#endif
}

// +----------------------------------------------------------------------------
// |  IoctlDevice
// +----------------------------------------------------------------------------
// |  Send a command to the device driver. Returns the device driver reply.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> ioctlCmd - Device driver ioctl command
// |  <IN>  -> boardID  - Command board id (TIM or UTIL)
// |  <IN>  -> arg      - Command argument (optional)
// +----------------------------------------------------------------------------
int CArcPCI::IoctlDevice( int dIoctlCmd, int dArg )
{
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "IoctlDevice",
								   "Not conntected to any device." );
	}

	int dRetVal = dArg;
	int dSuccess = Arc_IOCtl( m_hDevice,
							  dIoctlCmd,
							  &dRetVal,
							  sizeof( dRetVal ) );

	if ( !dSuccess )
	{
		if ( m_bStoreCmds )
		{
			string sCmd = FormatPCICommand( dIoctlCmd,
											dRetVal,
											dArg );

			m_cApiLog.Put( sCmd.c_str() );
		}

		CArcTools::ThrowException( "CArcPCI",
								   "IoctlDevice",
								   "Ioctl failed cmd: 0x%X arg: 0x%X : %e",
									dIoctlCmd,
									dArg,
									Arc_ErrorCode() );
    }

	// Set the debug message queue
	if ( m_bStoreCmds )
	{
			string sCmd = FormatPCICommand( dIoctlCmd,
										    dRetVal,
											dArg );

			m_cApiLog.Put( sCmd.c_str() );
	}

	return dRetVal;
}

// +----------------------------------------------------------------------------
// |  IoctlDevice
// +----------------------------------------------------------------------------
// |  Send a command to the device driver. Returns the device driver reply.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dIoctlCmd - Device driver ioctl command
// |  <IN>  -> dArg      - Array of command arguments
// |  <IN>  -> dArgCount - The number of arguments in the dArg array parameter
// +----------------------------------------------------------------------------
int CArcPCI::IoctlDevice( int dIoctlCmd, int dArg[], int dArgCount )
{
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "IoctlDevice",
								   "Not conntected to any device." );
	}

	int* pArgCopy = new int[ dArgCount ];

	if ( pArgCopy == NULL )
	{
		CArcTools::ThrowException( "CArcPCI",
								   "IoctlDevice",
								   "Failed to allocate argument buffer!" );
	}

	Arc_CopyMemory( pArgCopy, dArg, dArgCount );

	int dSuccess = Arc_IOCtl( m_hDevice,
							  dIoctlCmd,
							  dArg,
							  dArgCount );

	if ( !dSuccess )
	{
		if ( m_bStoreCmds )
		{
			string sCmd = FormatPCICommand( dIoctlCmd,
											dArg[ 0 ],
											pArgCopy,
											dArgCount );

			m_cApiLog.Put( sCmd.c_str() );
		}

		ostringstream oss;

		oss << "Ioctl failed cmd: 0x" << hex << dIoctlCmd << dec;

		for ( int i=0; i<dArgCount; i++ )
		{
			oss << " arg: 0x" << hex << pArgCopy[ i ] << dec;
		}

		oss << " : " << Arc_ErrorCode() << ends;

 		CArcTools::ThrowException( "CArcPCI",
								   "IoctlDevice",
								    oss.str() );
   	}

	// Set the debug message queue
	if ( m_bStoreCmds )
	{
		string sCmd = FormatPCICommand( dIoctlCmd,
										dArg[ 0 ],
										pArgCopy,
										dArgCount );

		m_cApiLog.Put( sCmd.c_str() );
	}

	if ( pArgCopy != NULL )
	{
		delete [] pArgCopy;
	}

	return dArg[ 0 ];
}

// +----------------------------------------------------------------------------
// |  GetCommonBufferProperties
// +----------------------------------------------------------------------------
// |  Fills in the image buffer structure with its properties, such as
// |  physical address and size.
// |
// |  Throws NOTHING on error. No error handling.
// +----------------------------------------------------------------------------
bool CArcPCI::GetCommonBufferProperties()
{
	try
	{
		m_tImgBuffer.dSize = IoctlDevice( ASTROPCI_GET_DMA_SIZE );

		m_tImgBuffer.ulPhysicalAddr = IoctlDevice( ASTROPCI_GET_DMA_ADDR );
	}
	catch ( ... )
	{
		return false;
	}

	return true;
}

// +----------------------------------------------------------------------------
// |  FormatPCICommand
// +----------------------------------------------------------------------------
// |  Formats an IOCTL command into a string that can be passed into a throw
// |  exception method.
// |
// |  <IN> -> dCmd    : The command value.
// |  <IN> -> dReply  : The received command reply value.
// |  <IN> -> dArg    : Any argument that may go with the specified command.
// |  <IN> -> dSysErr : A system error number, used to get system message.
// +----------------------------------------------------------------------------
const string CArcPCI::FormatPCICommand( int dCmd, int dReply, int dArg, int dSysErr )
{
	ostringstream oss;

	oss.setf( ios::hex, ios::basefield );
	oss.setf( ios::uppercase );

	oss << "[ ";

	     if ( dCmd == ASTROPCI_GET_HCTR ) { oss << "ASTROPCI_GET_HCTR"; }
	else if ( dCmd == ASTROPCI_GET_PROGRESS ) { oss << "ASTROPCI_GET_PROGRESS"; }
	else if ( dCmd == ASTROPCI_GET_DMA_ADDR ) { oss << "ASTROPCI_GET_DMA_ADDR"; }
	else if ( dCmd == ASTROPCI_GET_HSTR ) { oss << "ASTROPCI_GET_HSTR"; }
	else if ( dCmd == ASTROPCI_GET_DMA_SIZE ) { oss << "ASTROPCI_GET_DMA_SIZE"; }
	else if ( dCmd == ASTROPCI_GET_FRAMES_READ ) { oss << "ASTROPCI_GET_FRAMES_READ"; }
	else if ( dCmd == ASTROPCI_HCVR_DATA ) { oss << "ASTROPCI_HCVR_DATA"; }
	else if ( dCmd == ASTROPCI_SET_HCTR ) { oss << "ASTROPCI_SET_HCTR"; }
	else if ( dCmd == ASTROPCI_SET_HCVR ) { oss << "ASTROPCI_SET_HCVR"; }
	else if ( dCmd == ASTROPCI_PCI_DOWNLOAD ) { oss << "ASTROPCI_PCI_DOWNLOAD"; }
	else if ( dCmd == ASTROPCI_PCI_DOWNLOAD_WAIT ) { oss << "ASTROPCI_PCI_DOWNLOAD_WAIT"; }
	else if ( dCmd == ASTROPCI_COMMAND ) { oss << "ASTROPCI_COMMAND"; }
	else if ( dCmd == ASTROPCI_GET_CONFIG_BYTE ) { oss << "ASTROPCI_GET_CONFIG_BYTE"; }
	else if ( dCmd == ASTROPCI_GET_CONFIG_WORD ) { oss << "ASTROPCI_GET_CONFIG_WORD"; }
	else if ( dCmd == ASTROPCI_GET_CONFIG_DWORD ) { oss << "ASTROPCI_GET_CONFIG_DWORD"; }
	else if ( dCmd == ASTROPCI_SET_CONFIG_BYTE ) { oss << "ASTROPCI_SET_CONFIG_BYTE"; }
	else if ( dCmd == ASTROPCI_SET_CONFIG_WORD ) { oss << "ASTROPCI_SET_CONFIG_WORD"; }
	else if ( dCmd == ASTROPCI_SET_CONFIG_DWORD ) { oss << "ASTROPCI_SET_CONFIG_DWORD"; }

#ifdef WIN32
	else if ( dCmd == ASTROPCI_MEM_MAP ) { oss << "ASTROPCI_MEM_MAP"; }
	else if ( dCmd == ASTROPCI_MEM_UNMAP ) { oss << "ASTROPCI_MEM_UNMAP"; }
#endif

	else oss << "0x" << dCmd;

	if ( dArg != -1 )
	{
		oss << " 0x" << dArg;
	}

	oss << " -> 0x" << dReply << " ]";

	if ( dSysErr > 0 )
	{
		oss << endl << dec
			<< CArcTools::GetSystemMessage( dSysErr );
	}

	oss << ends;

	return oss.str();
}

// +----------------------------------------------------------------------------
// |  FormatPCICommand
// +----------------------------------------------------------------------------
// |  Formats an IOCTL command into a string that can be passed into a throw
// |  exception method.
// |
// |  <IN> -> dCmd      : The command value.
// |  <IN> -> dReply    : The received command reply value.
// |  <IN> -> dArg[]    : Array of arguments that go with the specified command.
// |  <IN> -> dArgCount : The number of elements in argument array.
// |  <IN> -> dSysErr   : A system error number, used to get system message.
// +----------------------------------------------------------------------------
const string CArcPCI::FormatPCICommand( int dCmd, int dReply, int dArg[], int dArgCount, int dSysErr )
{
	ostringstream oss;

	oss.setf( ios::hex, ios::basefield );
	oss.setf( ios::uppercase );

	oss << "[ ";

	     if ( dCmd == ASTROPCI_GET_HCTR ) { oss << "ASTROPCI_GET_HCTR"; }
	else if ( dCmd == ASTROPCI_GET_PROGRESS ) { oss << "ASTROPCI_GET_PROGRESS"; }
	else if ( dCmd == ASTROPCI_GET_DMA_ADDR ) { oss << "ASTROPCI_GET_DMA_ADDR"; }
	else if ( dCmd == ASTROPCI_GET_HSTR ) { oss << "ASTROPCI_GET_HSTR"; }
	else if ( dCmd == ASTROPCI_GET_DMA_SIZE ) { oss << "ASTROPCI_GET_DMA_SIZE"; }
	else if ( dCmd == ASTROPCI_GET_FRAMES_READ ) { oss << "ASTROPCI_GET_FRAMES_READ"; }
	else if ( dCmd == ASTROPCI_HCVR_DATA ) { oss << "ASTROPCI_HCVR_DATA"; }
	else if ( dCmd == ASTROPCI_SET_HCTR ) { oss << "ASTROPCI_SET_HCTR"; }
	else if ( dCmd == ASTROPCI_SET_HCVR ) { oss << "ASTROPCI_SET_HCVR"; }
	else if ( dCmd == ASTROPCI_PCI_DOWNLOAD ) { oss << "ASTROPCI_PCI_DOWNLOAD"; }
	else if ( dCmd == ASTROPCI_PCI_DOWNLOAD_WAIT ) { oss << "ASTROPCI_PCI_DOWNLOAD_WAIT"; }
	else if ( dCmd == ASTROPCI_COMMAND ) { oss << "ASTROPCI_COMMAND"; }
	else if ( dCmd == ASTROPCI_GET_CONFIG_BYTE ) { oss << "ASTROPCI_GET_CONFIG_BYTE"; }
	else if ( dCmd == ASTROPCI_GET_CONFIG_WORD ) { oss << "ASTROPCI_GET_CONFIG_WORD"; }
	else if ( dCmd == ASTROPCI_GET_CONFIG_DWORD ) { oss << "ASTROPCI_GET_CONFIG_DWORD"; }
	else if ( dCmd == ASTROPCI_SET_CONFIG_BYTE ) { oss << "ASTROPCI_SET_CONFIG_BYTE"; }
	else if ( dCmd == ASTROPCI_SET_CONFIG_WORD ) { oss << "ASTROPCI_SET_CONFIG_WORD"; }
	else if ( dCmd == ASTROPCI_SET_CONFIG_DWORD ) { oss << "ASTROPCI_SET_CONFIG_DWORD"; }

#ifdef WIN32
	else if ( dCmd == ASTROPCI_MEM_MAP ) { oss << "ASTROPCI_MEM_MAP"; }
	else if ( dCmd == ASTROPCI_MEM_UNMAP ) { oss << "ASTROPCI_MEM_UNMAP"; }
#endif

	else oss << "0x" << dCmd;

	for ( int i=0; i<dArgCount; i++ )
	{
		oss << " 0x" << dArg[ i ];
	}

	oss << " -> 0x" << dReply << " ]";

	if ( dSysErr > 0 )
	{
		oss << endl << dec
			<< CArcTools::GetSystemMessage( dSysErr );
	}

	oss << ends;

	return oss.str();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//             PCI CONFIGURATION SPACE TESTS
/////////////////////////////////////////////////////////////////////////////////////////////////
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
int CArcPCI::GetCfgSpByte( int dOffset )
{
	return IoctlDevice( ASTROPCI_GET_CONFIG_BYTE, dOffset );
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
int CArcPCI::GetCfgSpWord( int dOffset )
{
	return IoctlDevice( ASTROPCI_GET_CONFIG_WORD, dOffset );
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
int CArcPCI::GetCfgSpDWord( int dOffset )
{
	return IoctlDevice( ASTROPCI_GET_CONFIG_DWORD, dOffset );
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
void CArcPCI::SetCfgSpByte( int dOffset, int dValue )
{
	int dCfgData[ 2 ] = { dOffset, dValue };

	IoctlDevice( ASTROPCI_SET_CONFIG_BYTE, dCfgData, 2 );
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
void CArcPCI::SetCfgSpWord( int dOffset, int dValue )
{
	int dCfgData[ 2 ] = { dOffset, dValue };

	IoctlDevice( ASTROPCI_SET_CONFIG_WORD, dCfgData, 2 );
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
void CArcPCI::SetCfgSpDWord( int dOffset, int dValue )
{
	int dCfgData[ 2 ] = { dOffset, dValue };

	IoctlDevice( ASTROPCI_SET_CONFIG_DWORD, dCfgData, 2 );
}

// +----------------------------------------------------------------------------
// |  GetCfgSp
// +----------------------------------------------------------------------------
// |  Reads and parses the entire PCI configuration space header into readable
// |  text and bit definitions that are stored in a member list variable. The
// |  public methods of this class allow access to this list. This method will
// |  create the member list if it doesn't already exist and clears it if it
// |  does.
// +----------------------------------------------------------------------------
void CArcPCI::GetCfgSp()
{
	CArcPCIBase::GetCfgSp();
}

// +----------------------------------------------------------------------------
// |  GetBarSp
// +----------------------------------------------------------------------------
// |  Reads and parses the entire PCI Base Address Registers (BAR) into readable
// |  text and bit definitions that are stored in a member list variable. The
// |  public methods of this class allow access to this list. This method will
// |  create the member list if it doesn't already exist and clears it if it
// |  does. NOTE: Not all BARS or PCI boards have data.
// +----------------------------------------------------------------------------
void CArcPCI::GetBarSp()
{
	CArcPCIBase::GetBarSp();

    //
    //  Access the register data
    // +-------------------------------------------------------+
    PCIRegList* pList = new PCIRegList();

	int dRegValue = IoctlDevice( ASTROPCI_GET_HCTR );
	AddRegItem( pList,
				0x10,
                "Host Control Register ( HCTR )",
				dRegValue );

	dRegValue = IoctlDevice( ASTROPCI_GET_HSTR );
	AddRegItem( pList,
				0x14,
                "Host Status Register ( HSTR )",
				dRegValue,
				GetHSTRBitList( dRegValue ) );

    AddBarItem( "DSP Regs ( BAR0 )", pList );
}

// +----------------------------------------------------------------------------
// |  GetHSTRBitList
// +----------------------------------------------------------------------------
// |  Sets the bit list strings for the PCI DSP HSTR register.
// |
// |  <IN> -> dData  - The PCI cfg sp CLASS CODE and REV ID register value.
// |  <IN> -> bDrawSeparator - 'true' to include a line separator within the
// |                            bit list strings.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
CStringList* CArcPCI::GetHSTRBitList( int dData, bool bDrawSeparator )
{
    CStringList* pBitList = new CStringList;

    if ( bDrawSeparator )
	{
        *pBitList << "____________________________________________________";
	}

	int dHstr = ( dData & HTF_BIT_MASK ) >> 3;

	if ( dHstr == DONE_STATUS )
	{
		*pBitList << CArcTools::FormatString( "Status: 0x%X [ DON ]", dHstr );
	}

	else if ( dHstr == READ_REPLY_STATUS )
	{
		*pBitList << CArcTools::FormatString( "Status: 0x%X [ READ REPLY ]", dHstr );
	}

	else if ( dHstr == ERROR_STATUS )
	{
		*pBitList << CArcTools::FormatString( "Status: 0x%X [ ERR ]", dHstr );
	}
	
	else if ( dHstr == SYSTEM_RESET_STATUS )
	{
		*pBitList << CArcTools::FormatString( "Status: 0x%X [ SYR ]", dHstr );
	}

	else if ( dHstr == READOUT_STATUS )
	{
		*pBitList << CArcTools::FormatString( "Status: 0x%X [ READOUT ]", dHstr );
	}

	else if ( dHstr == BUSY_STATUS )
	{
		*pBitList << CArcTools::FormatString( "Status: 0x%X [ BUSY ]", dHstr );
	}

	else if ( dHstr == TIMEOUT_STATUS )
	{
		*pBitList << CArcTools::FormatString( "Status: 0x%X [ IDLE / TIMEOUT ]", dHstr );
	}

	else
	{
		*pBitList << CArcTools::FormatString( "Status: 0x%X [ UNKNOWN ]", dHstr );
	}

    return pBitList;
}
