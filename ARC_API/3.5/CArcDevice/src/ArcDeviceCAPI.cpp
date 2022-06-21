// ArcDeviceCAPI.cpp : Defines the exported functions for the DLL application.
//
#ifndef WIN32
	#include <cstring>
#endif

#include <stdexcept>
#include "DllMain.h"
#include "ArcDeviceCAPI.h"
#include "CArcDevice.h"
#include "CArcTools.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"

using namespace std;
using namespace arc;
using namespace arc::device;


// +------------------------------------------------------------------------------------+
// | nullptr wasn't added to gcc until version 4.6, CentOS still uses 4.4
// +------------------------------------------------------------------------------------+
#if ( defined( __linux__ ) && ( __GNUC__ < 4 || __GNUC_MINOR__ < 6 ) )
	#define nullptr		NULL
#endif


// +------------------------------------------------------------------------------------+
// | Create exposure interface
// +------------------------------------------------------------------------------------+
class IFExpose :  public CExpIFace
{
public:
	IFExpose( void ( *pExposeCall )( float ) = NULL, void ( *pReadCall )( int ) = NULL )
	{
		pECFunc = pExposeCall;
		pRCFunc = pReadCall;
	}

	void ExposeCallback( float fElapsedTime )
	{
		( *pECFunc )( fElapsedTime );
	}

	void ReadCallback( int dPixelCount )
	{
		( *pRCFunc )( dPixelCount );
	}

	void ( *pECFunc )( float );
	void ( *pRCFunc )( int );
};


// +------------------------------------------------------------------------------------+
// | Create continuous exposure interface
// +------------------------------------------------------------------------------------+
class IFConExp :  public CConIFace
{
public:
	IFConExp( void ( *pFrameCall )( int, int, int, int, void* ) = NULL )
	{
		pFCFunc = pFrameCall;
	}

	void FrameCallback( int   dFPBCount,
						int   dPCIFrameCount,
						int   dRows,
						int   dCols,
						void* pBuffer )
	{
		( *pFCFunc )( dFPBCount, dPCIFrameCount, dRows, dCols, pBuffer );
	}

	void ( *pFCFunc )( int, int, int, int, void* );
};


// +------------------------------------------------------------------------------------+
// | Globals
// +------------------------------------------------------------------------------------+
static auto_ptr<CArcDevice> g_pCDevice( 0 );

static char g_szErrMsg[ ARC_ERROR_MSG_SIZE ];
static char g_szTmpBuf[ ARC_MSG_SIZE ];

static char**	g_pDeviceList	= NULL;
static int		g_dDeviceCount	= 0;

static bool		g_bAbort		= false;


// +------------------------------------------------------------------------------------+
// | Define system dependent macros
// +------------------------------------------------------------------------------------+
#ifndef ARC_SPRINTF
#define ARC_SPRINTF

	#ifdef WIN32
		#define ArcSprintf( dst, size, fmt, msg )	sprintf_s( dst, size, fmt, msg )
	#else
		#define ArcSprintf( dst, size, fmt, msg )	sprintf( dst, fmt, msg )
	#endif

#endif

#ifndef ARC_ZEROMEM
#define ARC_ZEROMEM

	#ifdef WIN32
		#define ArcZeroMemory( mem, size )	ZeroMemory( mem, size )
	#else
		#define ArcZeroMemory( mem, size )	memset( mem, 0, size )
	#endif

#endif


// +------------------------------------------------------------------------------------+
// | Verify class pointer macro
// +------------------------------------------------------------------------------------+
#define VERIFY_CLASS_PTR( fnc, ptr )											\
						if ( ptr.get() == NULL )								\
						{														\
							throw runtime_error( string( "( " ) + fnc +			\
							string( " ): Invalid class pointer!" ) );			\
						}


// +------------------------------------------------------------------------------------+
// | Device class constants
// +------------------------------------------------------------------------------------+
const int DEVICE_NOPARAM	= CArcDevice::NOPARAM;


// +----------------------------------------------------------------------------
// |  ToString
// +----------------------------------------------------------------------------
// |  Returns a string that represents the device controlled by this library.
// |
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API const char* ArcDevice_ToString( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	ArcZeroMemory( g_szTmpBuf, ARC_MSG_SIZE );

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_ToString", g_pCDevice )

		ArcSprintf( g_szTmpBuf,
					ARC_MSG_SIZE,
					"%s",
					g_pCDevice.get()->ToString().c_str() );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return g_szTmpBuf;
}


// +----------------------------------------------------------------------------
// |  FindDevices
// +----------------------------------------------------------------------------
// |  Searches for available ARC, Inc PCIe devices and stores the list, which
// |  can be accessed via device number ( 0,1,2... ).
// |
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_FindDevices( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		CArcPCI::FindDevices();
		CArcPCIe::FindDevices();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  DeviceCount
// +----------------------------------------------------------------------------
// |  Returns the number of items in the device list. Must be called after
// |  FindDevices().
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_DeviceCount()
{
	return ( CArcPCI::DeviceCount() + CArcPCIe::DeviceCount() );
}


// +----------------------------------------------------------------------------
// |  GetDeviceStringList
// +----------------------------------------------------------------------------
// |  Returns a string list representation of the device list.
// |
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API const char** ArcDevice_GetDeviceStringList( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		if ( ArcDevice_DeviceCount() > 0 )
		{
			const string* sPCIList  = CArcPCI::GetDeviceStringList();
			const string* sPCIeList = CArcPCIe::GetDeviceStringList();

			if ( g_pDeviceList != NULL )
			{
				ArcDevice_FreeDeviceStringList();
			}

			g_pDeviceList = new char*[ ArcDevice_DeviceCount() ];

			if ( g_pDeviceList == NULL )
			{
				throw runtime_error(
					"( ArcDevice_GetDeviceStringList ): Failed to allocate list memory!" );
			}

			//  Get PCI list
			// +-----------------------------------------+
			for ( int i=0; i<CArcPCI::DeviceCount(); i++ )
			{
				g_pDeviceList[ i ] = new char[ 100 ];

				ArcSprintf( &g_pDeviceList[ i ][ 0 ], 100, "%s", sPCIList[ i ].c_str() );
			}

			//  Get PCIe list
			// +-----------------------------------------+
			for ( int i=CArcPCI::DeviceCount(); i<ArcDevice_DeviceCount(); i++ )
			{
				g_pDeviceList[ i ] = new char[ 100 ];

				ArcSprintf( &g_pDeviceList[ i ][ 0 ],
							100,
							"%s",
							sPCIeList[ i - CArcPCI::DeviceCount() ].c_str() );
			}
		}
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return const_cast<const char**>( g_pDeviceList );
}


// +----------------------------------------------------------------------------
// |  FreeDeviceStringList
// +----------------------------------------------------------------------------
// |  Frees the internal device list. Must be called after GetDeviceStringList()
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_FreeDeviceStringList()
{
	if ( g_pDeviceList != NULL )
	{
		for ( int i=0; i<g_dDeviceCount; i++ )
		{
			delete [] g_pDeviceList[ i ];
		}

		delete [] g_pDeviceList;
	}

	g_pDeviceList = NULL;
}


// +----------------------------------------------------------------------------
// |  IsOpen
// +----------------------------------------------------------------------------
// |  Returns 'true' if connected to a device; 'false' otherwise.
// |
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_IsOpen( int* pStatus )
{
	int dOpen = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_IsOpen", g_pCDevice )

		dOpen = ( g_pCDevice.get()->IsOpen() == true ? 1 : 0 );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dOpen;
}


// +----------------------------------------------------------------------------
// |  Open
// +----------------------------------------------------------------------------
// |  Opens a connection to the device driver associated with the specified
// |  device.
// |
// |  Returns ARC_STATUS_ERROR on error
// |
// |  <IN>  -> dDeviceNumber - Device number
// |  <OUT> -> pStatus       - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_Open( int dDeviceNumber, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		const char** pszDevList = ArcDevice_GetDeviceStringList( pStatus );

		if ( *pStatus == ARC_STATUS_ERROR )
		{
			throw runtime_error( g_szErrMsg );
		}

		CArcTools::CTokenizer cTokenizer;
		cTokenizer.Victim( pszDevList[ dDeviceNumber ] );

		string sDevice = cTokenizer.Next();

		cTokenizer.Next();		// Dump "Device" Text
				
		int dDevNum = atoi( cTokenizer.Next().c_str() );

		if ( sDevice.find( "PCIe" ) != string::npos )
		{
			g_pCDevice.reset( new CArcPCIe() );
		}

		else if ( sDevice.find( "PCI" ) != string::npos )
		{
			g_pCDevice.reset( new CArcPCI() );
		}

		else
		{
			throw runtime_error( "( ArcDevice_Open ): No ARC device found!" );
		}

		VERIFY_CLASS_PTR( "ArcDevice_Open", g_pCDevice )

		g_pCDevice.get()->Open( dDevNum );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	ArcDevice_FreeDeviceStringList();
}


// +----------------------------------------------------------------------------
// |  Open_I
// +----------------------------------------------------------------------------
// |  This version first calls Open, then MapCommonBuffer if Open
// |  succeeded. Basically, this function just combines the other two.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dDeviceNumber - PCI device number
// |  <IN>  -> dBytes        - The size of the kernel image buffer in bytes
// |  <OUT> -> pStatus       - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_Open_I( int dDeviceNumber, int dBytes, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		const char** pszDevList = ArcDevice_GetDeviceStringList( pStatus );

		if ( *pStatus == ARC_STATUS_ERROR )
		{
			throw runtime_error( g_szErrMsg );
		}

		CArcTools::CTokenizer cTokenizer;
		cTokenizer.Victim( pszDevList[ dDeviceNumber ] );

		string sDevice = cTokenizer.Next();

		cTokenizer.Next();		// Dump "Device" Text
				
		int dDevNum = atoi( cTokenizer.Next().c_str() );

		if ( sDevice.find( "PCIe" ) != string::npos )
		{
			g_pCDevice.reset( new CArcPCIe() );
		}

		else if ( sDevice.find( "PCI" ) != string::npos )
		{
			g_pCDevice.reset( new CArcPCI() );
		}

		else
		{
			throw runtime_error( "( ArcDevice_Open_I ): No ARC device found!" );
		}

		VERIFY_CLASS_PTR( "ArcDevice_Open_I", g_pCDevice )

		g_pCDevice.get()->Open( dDevNum, dBytes );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	ArcDevice_FreeDeviceStringList();
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
// |  <OUT> -> pStatus       - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API
void ArcDevice_Open_II( int dDeviceNumber, int dRows, int dCols, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		const char** pszDevList = ArcDevice_GetDeviceStringList( pStatus );

		if ( *pStatus == ARC_STATUS_ERROR )
		{
			throw runtime_error( g_szErrMsg );
		}

		CArcTools::CTokenizer cTokenizer;
		cTokenizer.Victim( pszDevList[ dDeviceNumber ] );

		string sDevice = cTokenizer.Next();

		cTokenizer.Next();		// Dump "Device" Text
				
		int dDevNum = atoi( cTokenizer.Next().c_str() );

		if ( sDevice.find( "PCIe" ) != string::npos )
		{
			g_pCDevice.reset( new CArcPCIe() );
		}

		else if ( sDevice.find( "PCI" ) != string::npos )
		{
			g_pCDevice.reset( new CArcPCI() );
		}

		else
		{
			throw runtime_error( "( ArcDevice_Open_I ): No ARC device found!" );
		}

		VERIFY_CLASS_PTR( "ArcDevice_Open_I", g_pCDevice )

		g_pCDevice.get()->Open( dDevNum, dRows, dCols );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	ArcDevice_FreeDeviceStringList();
}


// +----------------------------------------------------------------------------
// |  Close
// +----------------------------------------------------------------------------
// |  Closes the currently open driver that was opened with a call to Open.
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_Close()
{
	ArcDevice_FreeDeviceStringList();

	if ( g_pCDevice.get() != NULL )
	{
		g_pCDevice.get()->Close();

		g_pCDevice.reset( 0 );
	}
}


// +----------------------------------------------------------------------------
// |  Reset
// +----------------------------------------------------------------------------
// |  Resets the PCIe board.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_Reset( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_Reset", g_pCDevice )

		g_pCDevice.get()->Reset();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  MapCommonBuffer
// +----------------------------------------------------------------------------
// |  Map the device driver image buffer.
// |
// |  <IN>  -> bBytes - The number of bytes to map as an image buffer. Not
// |                    used by PCI/e.
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_MapCommonBuffer( int dBytes, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_MapCommonBuffer", g_pCDevice )

		g_pCDevice.get()->MapCommonBuffer( dBytes );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  UnMapCommonBuffer
// +----------------------------------------------------------------------------
// |  Un-Maps the device driver image buffer.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_UnMapCommonBuffer( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_UnMapCommonBuffer", g_pCDevice )

		g_pCDevice.get()->UnMapCommonBuffer();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  ReMapCommonBuffer
// +----------------------------------------------------------------------------
// |  Remaps the kernel image buffer by first calling UnMapCommonBuffer if
// |  necessary and then calls MapCommonBuffer.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_ReMapCommonBuffer( int dBytes, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_ReMapCommonBuffer", g_pCDevice )

		g_pCDevice.get()->ReMapCommonBuffer( dBytes );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  FillCommonBuffer
// +----------------------------------------------------------------------------
// |  Clears the image buffer by filling it with the specified value.
// |
// |  <IN>  -> u16Value - The 16-bit value to fill the buffer with. Default = 0
// |  <OUT> -> pStatus  - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_FillCommonBuffer( unsigned short u16Value, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_FillCommonBuffer", g_pCDevice )

		g_pCDevice.get()->FillCommonBuffer( u16Value );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  CommonBufferVA
// +----------------------------------------------------------------------------
// |  Returns the virtual address of the device driver image buffer.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void* ArcDevice_CommonBufferVA( int* pStatus )
{
	void* pVA = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_CommonBufferVA", g_pCDevice )

		pVA = g_pCDevice.get()->CommonBufferVA();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pVA;
}


// +----------------------------------------------------------------------------
// |  CommonBufferPA
// +----------------------------------------------------------------------------
// |  Returns the physical address of the device driver image buffer.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API unsigned long ArcDevice_CommonBufferPA( int* pStatus )
{
	unsigned long ulPA = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_CommonBufferPA", g_pCDevice )

		ulPA = g_pCDevice.get()->CommonBufferPA();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return ulPA;
}


// +----------------------------------------------------------------------------
// |  CommonBufferSize
// +----------------------------------------------------------------------------
// |  Returns the device driver image buffer size in bytes.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_CommonBufferSize( int* pStatus )
{
	int dBufSize = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_CommonBufferSize", g_pCDevice )

		dBufSize = g_pCDevice.get()->CommonBufferSize();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dBufSize;
}


// +----------------------------------------------------------------------------
// |  GetId
// +----------------------------------------------------------------------------
// |  Returns the board id
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_GetId( int* pStatus )
{
	int dId = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetId", g_pCDevice )

		dId = g_pCDevice.get()->GetId();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dId;
}


// +----------------------------------------------------------------------------
// |  GetStatus
// +----------------------------------------------------------------------------
// |  Returns the PCIe status register value.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_GetStatus( int* pStatus )
{
	int dStatus = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetStatus", g_pCDevice )

		dStatus = g_pCDevice.get()->GetStatus();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dStatus;
}


// +----------------------------------------------------------------------------
// |  ClearStatus
// +----------------------------------------------------------------------------
// |  Clears the PCIe status register.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_ClearStatus( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_ClearStatus", g_pCDevice )

		g_pCDevice.get()->ClearStatus();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  Set2xFOTransmitter
// +----------------------------------------------------------------------------
// |  Sets the controller to use two fiber optic transmitters.
// |
// |  <IN>  -> bOnOff  - True to enable dual transmitters; false otherwise.
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_Set2xFOTransmitter( int bOnOff, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_Set2xFOTransmitter", g_pCDevice )

		g_pCDevice.get()->Set2xFOTransmitter( ( bOnOff == 1 ? true : false ) );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  LoadDeviceFile
// +----------------------------------------------------------------------------
// |  Loads a device file, such as pci.lod. This function is not valid for PCIe.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +---------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_LoadDeviceFile( const char* pszFile, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_LoadDeviceFile", g_pCDevice )

		g_pCDevice.get()->LoadDeviceFile( pszFile );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  Command
// +----------------------------------------------------------------------------
// |  Send a command to the controller timing or utility board. Returns the
// |  controller reply, typically DON.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dBoardId - Command board id ( TIM or UTIL )
// |  <IN>  -> dCommand - Board command
// |  <OUT> -> pStatus  - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_Command( int dBoardId, int dCommand, int* pStatus )
{
	return ArcDevice_Command_IIII( dBoardId,
								   dCommand,
								   DEVICE_NOPARAM,
								   DEVICE_NOPARAM,
								   DEVICE_NOPARAM,
								   DEVICE_NOPARAM,
								   pStatus );
}


// +----------------------------------------------------------------------------
// |  Command_I
// +----------------------------------------------------------------------------
// |  Send a command to the controller timing or utility board. Returns the
// |  controller reply, typically DON.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dBoardId - Command board id ( TIM or UTIL )
// |  <IN>  -> dCommand - Board command
// |  <IN>  -> dArg1    - Command arguments
// |  <OUT> -> pStatus  - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_Command_I( int dBoardId, int dCommand, int dArg1, int* pStatus )
{
	return ArcDevice_Command_IIII( dBoardId,
								   dCommand,
								   dArg1,
								   DEVICE_NOPARAM,
								   DEVICE_NOPARAM,
								   DEVICE_NOPARAM,
								   pStatus );
}

// +----------------------------------------------------------------------------
// |  Command_II
// +----------------------------------------------------------------------------
// |  Send a command to the controller timing or utility board. Returns the
// |  controller reply, typically DON.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dBoardId - Command board id ( TIM or UTIL )
// |  <IN>  -> dCommand - Board command
// |  <IN>  -> dArg1/2  - Command arguments
// |  <OUT> -> pStatus  - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_Command_II( int dBoardId, int dCommand, int dArg1,
										 int dArg2, int* pStatus )
{
	return ArcDevice_Command_IIII( dBoardId,
								   dCommand,
								   dArg1,
								   dArg2,
								   DEVICE_NOPARAM,
								   DEVICE_NOPARAM,
								   pStatus );
}


// +----------------------------------------------------------------------------
// |  Command_III
// +----------------------------------------------------------------------------
// |  Send a command to the controller timing or utility board. Returns the
// |  controller reply, typically DON.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dBoardId  - Command board id ( TIM or UTIL )
// |  <IN>  -> dCommand  - Board command
// |  <IN>  -> dArg1/2/3 - Command arguments
// |  <OUT> -> pStatus   - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int  ArcDevice_Command_III( int dBoardId, int dCommand, int dArg1,
										   int dArg2, int dArg3, int* pStatus )
{
	return ArcDevice_Command_IIII( dBoardId,
								   dCommand,
								   dArg1,
								   dArg2,
								   dArg3,
								   DEVICE_NOPARAM,
								   pStatus );
}


// +----------------------------------------------------------------------------
// |  Command_IIII
// +----------------------------------------------------------------------------
// |  Send a command to the controller timing or utility board. Returns the
// |  controller reply, typically DON.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dBoardId    - Command board id ( TIM or UTIL )
// |  <IN>  -> dCommand    - Board command
// |  <IN>  -> dArg1/2/3/4 - Command arguments
// |  <OUT> -> pStatus     - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_Command_IIII( int dBoardId, int dCommand, int dArg1,
										   int dArg2, int dArg3, int dArg4,
										   int* pStatus )
{
	int dReply = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_Command", g_pCDevice )

		dReply = g_pCDevice.get()->Command( dBoardId,
											dCommand,
											dArg1,
											dArg2,
											dArg3,
											dArg4 );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dReply;
}


// +----------------------------------------------------------------------------
// |  GetControllerId
// +----------------------------------------------------------------------------
// |  Returns the controller ID or 'ERR' if none.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_GetControllerId( int* pStatus )
{
	int dCtlrId = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetControllerId", g_pCDevice )

		dCtlrId = g_pCDevice.get()->GetControllerId();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dCtlrId;
}


// +----------------------------------------------------------------------------
// |  ResetController
// +----------------------------------------------------------------------------
// |  Resets the controller.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_ResetController( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_ResetController", g_pCDevice )

		g_pCDevice.get()->ResetController();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// | IsControllerConnected
// +----------------------------------------------------------------------------
// |  Returns 'true' if a controller is connected to the PCIe board.  This is
// |  for fiber optic A only.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_IsControllerConnected( int* pStatus )
{
	int dConnected = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_IsControllerConnected", g_pCDevice )

		dConnected =
			( g_pCDevice.get()->IsControllerConnected() == true ? 1 : 0 );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dConnected;
}


// +----------------------------------------------------------------------------
// |  SetupController
// +----------------------------------------------------------------------------
// |  This is a convenience function that performs a controller setup given
// |  the specified parameters.
// |
// |  <IN>  -> bReset      - 'true' to reset the controller.
// |  <IN>  -> bTdl        - 'true' to send TDLs to the PCI board and any board
// |                          whose .lod file is not NULL.
// |  <IN>  -> bPower      - 'true' to power on the controller.
// |  <IN>  -> dRows       -  The image row dimension (in pixels).
// |  <IN>  -> dCols       -  The image col dimension (in pixels).
// |  <IN>  -> pszTimFile  - The timing board file to load (.lod file).
// |  <IN>  -> pszUtilFile - The utility board file to load (.lod file).
// |  <IN>  -> pszPciFile  - The pci board file to load (.lod file).
// |  <OUT> -> pStatus     - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_SetupController( int bReset, int bTdl, int bPower,
											   int dRows, int dCols, const char* pszTimFile,
											   const char* pszUtilFile, const char* pszPciFile,
											   int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_SetupController", g_pCDevice )

		g_pCDevice.get()->SetupController( ( bReset == 1 ? true : false ),
										   ( bTdl == 1 ? true : false ),
										   ( bPower == 1 ? true : false ),
										   dRows,
										   dCols,
										   ( pszTimFile == nullptr ? "" : pszTimFile ),
										   ( pszUtilFile == nullptr ? "" : pszUtilFile ),
										   ( pszPciFile == nullptr ? "" : pszPciFile ),
										   false );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  LoadControllerFile
// +----------------------------------------------------------------------------
// |  Loads a SmallCam/GenI/II/III timing or utility file (.lod) into the
// |  specified board.
// |
// |  <IN>  -> pszFilename - The SMALLCAM or GENI/II/III TIM or UTIL lod file
// |                         to load.
// |  <IN>  -> bValidate   - Set to 'true' if the download should be read back
// |                         and checked after every write.
// |  <OUT> -> pStatus     - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_LoadControllerFile( const char* pszFilename,
												  int bValidate,
												  int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_LoadControllerFile", g_pCDevice )

		g_pCDevice.get()->LoadControllerFile( pszFilename,
											  ( bValidate == 1 ? true : false ),
											  false );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// | SetImageSize
// +----------------------------------------------------------------------------
// | Sets the image size in pixels on the controller. This is used during setup,
// | subarray, binning, continuous readout, etc., whenever the image size is
// | changed. This method will attempt to re-map the image buffer if the
// | specified image size is greater than that of the image buffer.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dRows   - The number of rows in the image.
// |  <IN>  -> dCols   - The number of columns in the image.
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_SetImageSize( int dRows, int dCols, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_SetImageSize", g_pCDevice )

		g_pCDevice.get()->SetImageSize( dRows, dCols );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// | GetImageRows
// +----------------------------------------------------------------------------
// | Returns the image row size (pixels) that has been set on the controller.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_GetImageRows( int* pStatus )
{
	int dRows = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetImageRows", g_pCDevice )

		dRows = g_pCDevice.get()->GetImageRows();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dRows;
}


// +----------------------------------------------------------------------------
// | GetImageCols
// +----------------------------------------------------------------------------
// | Returns the image column size (pixels) that has been set on the controller.
// |
// | <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_GetImageCols( int* pStatus )
{
	int dCols = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetImageCols", g_pCDevice )

		dCols = g_pCDevice.get()->GetImageCols();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dCols;
}


// +----------------------------------------------------------------------------
// |  GetCCParams
// +----------------------------------------------------------------------------
// |  Returns the available configuration parameters. The parameter bit
// |  definitions are specified in ArcDefs.h.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_GetCCParams( int* pStatus )
{
	int dCCParams = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetCCParams", g_pCDevice )

		dCCParams = g_pCDevice.get()->GetCCParams();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dCCParams;
}


// +----------------------------------------------------------------------------
// |  IsCCParamSupported
// +----------------------------------------------------------------------------
// |  Returns 'true' if the specified configuration parameter is available on
// |  the controller.  Returns 'false' otherwise. The parameter bit definitions
// |  are specified in ArcDefs.h.
// |
// |  <IN>  -> dParameter - The controller parameter to check.
// |  <OUT> -> pStatus    - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_IsCCParamSupported( int dParameter, int* pStatus )
{
	int dSupported = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_IsCCParamSupported", g_pCDevice )

		dSupported =
			( g_pCDevice.get()->IsCCParamSupported( dParameter ) ? 1 : 0 );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dSupported;
}


// +----------------------------------------------------------------------------
// |  IsCCD
// +----------------------------------------------------------------------------
// |  Returns false if the controller contains an IR video processor board.
// |  Returns true otherwise.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_IsCCD( int* pStatus )
{
	int dIsCCD = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_IsCCD", g_pCDevice )

		dIsCCD = ( g_pCDevice.get()->IsCCD() ? 1 : 0 );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dIsCCD;
}


// +----------------------------------------------------------------------------
// |  IsBinningSet
// +----------------------------------------------------------------------------
// |  Returns 'true' if binning is set on the controller, returns 'false'
// |  otherwise.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_IsBinningSet( int* pStatus )
{
	int dIsBin = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_IsBinningSet", g_pCDevice )

		dIsBin = ( g_pCDevice.get()->IsBinningSet() ? 1 : 0 );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dIsBin;
}


// +----------------------------------------------------------------------------
// |  SetBinning
// +----------------------------------------------------------------------------
// |  Sets the camera controller binning factors for both the rows and columns.
// |  Binning causes each axis to combine xxxFactor number of pixels. For
// |  example, if rows = 1000, cols = 1000, rowFactor = 2, colFactor = 4
// |  then binRows = 500, binCols = 250.
// |
// |  <IN>  -> dRows      - The pre-binning number of image rows
// |  <IN>  -> dCols      - The pre-binning number of images columns
// |  <IN>  -> dRowFactor - The binning row factor
// |  <IN>  -> dColFactor - The binning column factor
// |  <OUT> -> pBinRows   - Pointer that will get binned image row value
// |  <OUT> -> pBinCols   - Pointer that will get binned image col value
// |  <OUT> -> pStatus    - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_SetBinning( int dRows, int dCols, int dRowFactor,
										  int dColFactor, int* pBinRows, int* pBinCols,
										  int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_SetBinning", g_pCDevice )

		g_pCDevice.get()->SetBinning( dRows,
									  dCols,
									  dRowFactor,
									  dColFactor,
									  pBinRows,
									  pBinCols );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  UnsetBinning
// +----------------------------------------------------------------------------
// |  Unsets the camera controller binning factors previously set by a call to
// |  CArcDevice::SetBinning().
// |
// |  <IN>  -> dRows   - The image rows to set on the controller 
// |  <IN>  -> dCols   - The image cols to set on the controller
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_UnSetBinning( int dRows, int dCols, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_UnSetBinning", g_pCDevice )

		g_pCDevice.get()->UnSetBinning( dRows, dCols );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  SetSubArray
// +----------------------------------------------------------------------------
// |  Sets the camera controller to sub-array mode. All parameters are in
// |  pixels.
// |
// |	+-----------------------+------------+
// |	|						|            |
// |	|          BOX	        |    BIAS    |
// |    |<----------------5------->          |
// |    |        <--4-->        |  |         |
// |	|       +-------+^      |  +-------+ |
// |	|       |       ||      |  |       | |
// |	|<------|-->2   |3      |  |<--6-->| |
// |	|       |   ^   ||		|  |	   | |
// |	|       +---|---+v      |  +-------+ |
// |	|			|	        |            |
// |	|			|	        |            |
// |	|			1	        |            |
// |	|			|	        |            |
// |	|			v	        |            |
// |	+-----------------------+------------+
// |
// |  <OUT> -> dOldRows - Returns the original row size set on the controller
// |  <OUT> -> dOldCols - Returns the original col size set on the controller
// |  <IN>  -> #1 - dRow - The row # of the center of the sub-array in pixels
// |  <IN>  -> #2 - dCol - The col # of the center of the sub-array in pixels
// |  <IN>  -> #3 - dSubRows - The sub-array row count in pixels
// |  <IN>  -> #4 - dSubCols - The sub-array col count in pixels
// |  <IN>  -> #5 - dBiasOffset - The offset of the bias region in pixels
// |  <IN>  -> #6 - dBiasCols - The col count of the bias region in pixels
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_SetSubArray( int* pOldRows, int* pOldCols, int dRow,
										   int dCol, int dSubRows, int dSubCols,
										   int dBiasOffset, int dBiasWidth, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_SetSubArray", g_pCDevice )

		g_pCDevice.get()->SetSubArray( *pOldRows,
									   *pOldCols,
									   dRow,
									   dCol,
									   dSubRows,
									   dSubCols,
									   dBiasOffset,
									   dBiasWidth );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  UnSetSubArray
// +----------------------------------------------------------------------------
// |  Unsets the camera controller from sub-array mode.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dRows   - The row size to set on the controller
// |  <IN>  -> dCols   - The col size to set on the controller
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_UnSetSubArray( int dRows, int dCols, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_UnSetSubArray", g_pCDevice )

		g_pCDevice.get()->UnSetSubArray( dRows, dCols );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  IsSyntheticImageMode
// +----------------------------------------------------------------------------
// |  Returns true if synthetic readout is turned on. Otherwise, returns
// |  'false'. A synthetic image looks like: 0, 1, 2, ..., 65535, 0, 1, 2,
// |  ..., 65353, etc
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_IsSyntheticImageMode( int* pStatus )
{
	int dIsSyn = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_IsSyntheticImageMode", g_pCDevice )

		dIsSyn = ( g_pCDevice.get()->IsSyntheticImageMode() ? 1 : 0 );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dIsSyn;
}


// +----------------------------------------------------------------------------
// |  SetSyntheticImageMode
// +----------------------------------------------------------------------------
// |  If mode is 'true', then synthetic readout will be turned on. Set mode
// |  to 'false' to turn it off. A synthetic image looks like: 0, 1, 2, ...,
// |  65535, 0, 1, 2, ..., 65353, etc
// |
// |  <IN>  -> bMode   - 'true' to turn it on, 'false' to turn if off.
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_SetSyntheticImageMode( int bMode, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_SetSyntheticImageMode", g_pCDevice )

		g_pCDevice.get()->SetSyntheticImageMode(
										( bMode == 1 ? true : false ) );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  SetOpenShutter
// +----------------------------------------------------------------------------
// |  Sets whether or not the shutter will open when an exposure is started
// |  ( using SEX ).
// |
// |  <IN>  -> shouldOpen - Set to 'true' if the shutter should open during
// |                        expose. Set to 'false' to keep the shutter closed.
// |  <OUT> -> pStatus    - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_SetOpenShutter( int bShouldOpen, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_SetOpenShutter", g_pCDevice )

		g_pCDevice.get()->SetOpenShutter(
									( bShouldOpen == 1 ? true : false ) );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  Expose
// +----------------------------------------------------------------------------
// |  Starts an exposure using the specified exposure time and whether or not
// |  to open the shutter. Callbacks for the elapsed exposure time and image
// |  data readout can be used.
// |
// |  <IN> -> fExpTime - The exposure time ( in seconds ).
// |  <IN> -> bOpenShutter - Set to 'true' if the shutter should open during the
// |                         exposure. Set to 'false' to keep the shutter closed.
// |  <IN> -> dRows - The image row size ( in pixels ).
// |  <IN> -> dCols - The image column size ( in pixels ).
// |  <IN> -> fExpTime - The exposure time ( in seconds ).
// |  <IN> -> pExpIFace - Function pointer to CExpIFace class. NULL by default.
// |  <OUT> -> pStatus  - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API
void ArcDevice_Expose( float fExpTime, int dRows, int dCols,
					   void ( *pExposeCall )( float ), void ( *pReadCall )( int ),
					   int bOpenShutter, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_Expose", g_pCDevice )

		IFExpose ExpIFace( pExposeCall, pReadCall );

		g_pCDevice.get()->Expose( fExpTime,
								  dRows,
								  dCols,
								  g_bAbort,
								  &ExpIFace,
								  ( bOpenShutter == 1 ? true : false ) );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  StopExposure
// +----------------------------------------------------------------------------
// |  Stops the current exposure.
// |
// |  NOTE: The command is sent manually and NOT using the Command() method
// |        because the Command() method checks for readout and fails.
// |
// |  <OUT> -> pStatus  - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_StopExposure( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	g_bAbort = true;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_StopExposure", g_pCDevice )

		g_pCDevice.get()->StopExposure();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  Continuous
// +----------------------------------------------------------------------------
// |  This method can be called to start continuous readout.  A callback for 
// |  each frame read out can be used to process the frame.
// |
// |  <IN> -> dRows - The image row size ( in pixels ).
// |  <IN> -> dCols - The image column size ( in pixels ).
// |  <IN> -> dNumOfFrames - The number of frames to take.
// |  <IN> -> fExpTime - The exposure time ( in seconds ).
// |  <IN> -> pFrameCall - Function pointer to callback for frame completion.
// |                       NULL by default.
// |  <IN> -> bOpenShutter - 'true' to open the shutter during expose; 'false'
// |                         otherwise.
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API
void ArcDevice_Continuous( int dRows, int dCols, int dNumOfFrames, float fExpTime,
						   void ( *pFrameCall )( int, int, int, int, void * ),
						   int bOpenShutter, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_Continuous", g_pCDevice )

		IFConExp ConIFace( pFrameCall );

		g_pCDevice.get()->Continuous( dRows,
									  dCols,
									  dNumOfFrames,
									  fExpTime,
									  g_bAbort,
									  &ConIFace,
									  ( bOpenShutter == 1 ? true : false ) );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  StopContinuous
// +----------------------------------------------------------------------------
// |  Sends abort expose/readout and sets the controller back into single
// |  read mode.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_StopContinuous( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	g_bAbort = true;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_StopContinuous", g_pCDevice )

		g_pCDevice.get()->StopContinuous();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  IsReadout
// +----------------------------------------------------------------------------
// |  Returns 'true' if the controller is in readout.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_IsReadout( int* pStatus )
{
	int dIsROUT = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_IsReadout", g_pCDevice )

		dIsROUT = ( g_pCDevice.get()->IsReadout() ? 1 : 0 );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dIsROUT;
}


// +----------------------------------------------------------------------------
// |  GetPixelCount
// +----------------------------------------------------------------------------
// |  Returns the current pixel count.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_GetPixelCount( int* pStatus )
{
	int dPixCnt = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetPixelCount", g_pCDevice )

		dPixCnt = g_pCDevice.get()->GetPixelCount();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
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
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_GetCRPixelCount( int* pStatus )
{
	int dPixCnt = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetCRPixelCount", g_pCDevice )

		dPixCnt = g_pCDevice.get()->GetCRPixelCount();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dPixCnt;
}


// +----------------------------------------------------------------------------
// |  GetFrameCount
// +----------------------------------------------------------------------------
// |  Returns the current frame count. The camera MUST be set for continuous
// |  readout for this to work. This code needs to execute fast, so no 
// |  pre-checking, such as IsControllerConnected() is done.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_GetFrameCount( int* pStatus )
{
	int dFrameCnt = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetFrameCount", g_pCDevice )

		dFrameCnt = g_pCDevice.get()->GetFrameCount();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dFrameCnt;
}


// +----------------------------------------------------------------------------
// |  Check the specified value for error replies:
// |  TOUT, ROUT, HERR, ERR, SYR, RST
// +----------------------------------------------------------------------------
// |  Returns 'true' if the specified value matches 'TOUT, 'ROUT', 'HERR',
// |  'ERR', 'SYR' or 'RST'. Returns 'false' otherwise.
// |
// |  <IN>  -> dWord   - The value to check
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_ContainsError( int dWord, int* pStatus )
{
	int dError = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_ContainsError", g_pCDevice )

		dError = ( g_pCDevice.get()->ContainsError( dWord ) ? 1 : 0 );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dError;
}


// +----------------------------------------------------------------------------
// |  Check that the specified value falls within the specified range.
// +----------------------------------------------------------------------------
// |  Returns 'true' if the specified value falls within the specified range.
// |  Returns 'false' otherwise.
// |
// |  <IN>  -> dWord    - The value to check
// |  <IN>  -> dWordMin - The minimum range value ( not inclusive )
// |  <IN>  -> dWordMax - The maximum range value ( not inclusive )
// |  <OUT> -> pStatus  - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API
int ArcDevice_ContainsError_I( int dWord, int dWordMin, int dWordMax, int* pStatus )
{
	int dError = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_ContainsError_I", g_pCDevice )

		dError =
			( g_pCDevice.get()->ContainsError( dWord, dWordMin, dWordMax ) ? 1 : 0 );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dError;
}


// +----------------------------------------------------------------------------
// |  GetNextLoggedCmd
// +----------------------------------------------------------------------------
// |  Pops the first message from the command logger and returns it. If the
// |  logger is empty, then NULL is returned.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API const char* ArcDevice_GetNextLoggedCmd( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetNextLoggedCmd", g_pCDevice )

		ArcSprintf( g_szTmpBuf,
					ARC_MSG_SIZE,
					"%s",
					g_pCDevice.get()->GetNextLoggedCmd().c_str() );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return g_szTmpBuf;
}


// +----------------------------------------------------------------------------
// |  GetLoggedCmdCount
// +----------------------------------------------------------------------------
// |  Returns the available command message count.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API int ArcDevice_GetLoggedCmdCount( int* pStatus )
{
	int dCount = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetLoggedCmdCount", g_pCDevice )

		dCount = g_pCDevice.get()->GetLoggedCmdCount();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dCount;
}


// +----------------------------------------------------------------------------
// |  SetLogCmds
// +----------------------------------------------------------------------------
// |  Turns command logging on/off. This logging can be used for debugging to
// |  see command details in the following form:
// |
// |  <header> <cmd> <arg1> ... <arg4> -> <controller reply>
// |  Example: 0x203 TDL 0x112233 -> 0x444E4F
// |
// |  <IN>  -> bOnOff  - 'true' to turn loggin on; 'false' otherwise.
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_SetLogCmds( int bOnOff, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_SetLogCmds", g_pCDevice )

		g_pCDevice.get()->SetLogCmds( ( bOnOff == 1 ? true : false ) );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  GetArrayTemperature
// +----------------------------------------------------------------------------
// |  Returns the array temperature.
// |
// |  Returns the average temperature value ( in Celcius ). The temperature is
// |  read gTmpCtrl_SDNumberOfReads times and averaged. Also, for a read to be
// |  included in the average the difference between the target temperature and
// |  the actual temperature must be less than the tolerance specified by
// |  gTmpCtrl_SDTolerance.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API double ArcDevice_GetArrayTemperature( int* pStatus )
{
	double gTemp = 0.0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetArrayTemperature", g_pCDevice )

		gTemp = g_pCDevice.get()->GetArrayTemperature();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return gTemp;
}


// +----------------------------------------------------------------------------
// |  GetArrayTemperatureDN
// +----------------------------------------------------------------------------
// |  Returns the digital number associated with the current array temperature
// |  value.
// |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API double ArcDevice_GetArrayTemperatureDN( int* pStatus )
{
	double gTempDN = 0.0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_GetArrayTemperatureDN", g_pCDevice )

		gTempDN = g_pCDevice.get()->GetArrayTemperatureDN();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return gTempDN;
}


// +----------------------------------------------------------------------------
// |  SetArrayTemperature
// +----------------------------------------------------------------------------
// |  Sets the array temperature to regulate around.
// |
// |  <IN>  -> gTempVal - The temperature value ( in Celcius ) to regulate
// |                      around.
// |  <OUT> -> pStatus  - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API void ArcDevice_SetArrayTemperature( double gTempVal, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_SetArrayTemperature", g_pCDevice )

		g_pCDevice.get()->SetArrayTemperature( gTempVal );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  LoadTemperatureCtrlData
// +----------------------------------------------------------------------------
// |  Loads temperature control constants from the specified file.  The default
// |  constants are stored in TempCtrl.h and cannot be permanently overwritten.
// |
// |  <IN> -> pszFilename  - The filename of the temperature control constants
// |                         file.
// |  <OUT> -> pStatus     - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API
void ArcDevice_LoadTemperatureCtrlData( const char* pszFilename, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_LoadTemperatureCtrlData", g_pCDevice )

		g_pCDevice.get()->LoadTemperatureCtrlData( pszFilename );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  SaveTemperatureCtrlData
// +----------------------------------------------------------------------------
// |  Saves the current temperature control constants to the specified file.
// |  These may be different from the values in TempCtrl.h if another
// |  temperature control file has been previously loaded.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> pszFilename  - The filename of the temperature control constants
// |                         file to save.
// |  <OUT> -> pStatus     - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
CARCDEVICE_API
void ArcDevice_SaveTemperatureCtrlData( const char* pszFilename, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDevice_SaveTemperatureCtrlData", g_pCDevice )

		g_pCDevice.get()->SaveTemperatureCtrlData( pszFilename );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// | GetLastError
// +----------------------------------------------------------------------------
// | Returns the last error message reported.
// +----------------------------------------------------------------------------
CARCDEVICE_API const char* ArcDevice_GetLastError()
{
	return const_cast<const char *>( g_szErrMsg );
}
