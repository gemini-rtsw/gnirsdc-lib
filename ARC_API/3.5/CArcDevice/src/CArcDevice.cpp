#ifdef WIN32
	#include <windows.h>
#else
	#include <sys/ioctl.h>
	#include <sys/mman.h>
	#include <unistd.h>
	#include <errno.h>
	#include <cstring>
	#include <cstdlib>
#endif

#include <memory>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "CArcDevice.h"
#include "CArcTools.h"
#include "ArcOSDefs.h"
#include "ArcDefs.h"
#include "TempCtrl.h"

using namespace std;
using namespace arc::device;


// +---------------------------------------------------+
// |  Shared Pointer Array Deleter
// +---------------------------------------------------+
template<typename T> class ArrayDeleter
{
	public:
		void operator () ( T* d ) const
		{
			delete [] d;
		}
}; 


// +----------------------------------------------------------------------------
// |  Class Constructor
// +----------------------------------------------------------------------------
CArcDevice::CArcDevice( void )
{
	m_hDevice    = INVALID_HANDLE_VALUE;
	m_dCCParam   = 0;
	m_bStoreCmds = false;

	Arc_ZeroMemory( &m_tImgBuffer, sizeof( ImgBuf_t ) );

	SetDefaultTemperatureValues();
}

// +----------------------------------------------------------------------------
// |  Class Destructor
// +----------------------------------------------------------------------------
CArcDevice::~CArcDevice( void )
{
}

// +----------------------------------------------------------------------------
// |  IsOpen
// +----------------------------------------------------------------------------
// |  Returns 'true' if connected to a device; 'false' otherwise.
// |
// |  Throws NOTHING on error. No error handling.
// +----------------------------------------------------------------------------
bool CArcDevice::IsOpen()
{
	return ( ( m_hDevice != INVALID_HANDLE_VALUE ) ? true : false );
}

// +----------------------------------------------------------------------------
// |  ReMapCommonBuffer
// +----------------------------------------------------------------------------
// |  Remaps the kernel image buffer by first calling UnMapCommonBuffer if
// |  necessary and then calls MapCommonBuffer.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcDevice::ReMapCommonBuffer( int dBytes )
{
	UnMapCommonBuffer();
	MapCommonBuffer( dBytes );
}

// +----------------------------------------------------------------------------
// |  FillCommonBuffer
// +----------------------------------------------------------------------------
// |  Clears the image buffer by filling it with the specified value.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> u16Value - The 16-bit value to fill the buffer with. Default = 0
// +----------------------------------------------------------------------------
void CArcDevice::FillCommonBuffer( unsigned short u16Value )
{
	if ( m_tImgBuffer.pUserAddr == ( void * )NULL )
	{
		CArcTools::ThrowException(
						"CArcDevice",
						"FillCommonBuffer",
						 string( "NULL image buffer! Check that a device " ) +
						 string( "is open and common buffer has been " ) +
						 string( "allocated and mapped!" ) );
	}

	unsigned short* pU16Buf =
				( unsigned short * )m_tImgBuffer.pUserAddr;

	int dPixCount =
				int( m_tImgBuffer.dSize / sizeof( unsigned short ) );

	//  Perform a quicker fill if we can
	// +-------------------------------------------------+
	if ( ( dPixCount % 4 ) == 0 )
	{
		for ( int i=0; i<dPixCount; i+=4 )
		{
			pU16Buf[ i + 0 ] = u16Value;
			pU16Buf[ i + 1 ] = u16Value;
			pU16Buf[ i + 2 ] = u16Value;
			pU16Buf[ i + 3 ] = u16Value;
		}
	}
	else
	{
		for ( int i=0; i<dPixCount; i++ )
		{
			pU16Buf[ i ] = u16Value;
		}
	}
}

// +----------------------------------------------------------------------------
// |  CommonBufferVA
// +----------------------------------------------------------------------------
// |  Returns the virtual address of the device driver image buffer.
// +----------------------------------------------------------------------------
void* CArcDevice::CommonBufferVA()
{
    return ( void * )m_tImgBuffer.pUserAddr;
}

// +----------------------------------------------------------------------------
// |  CommonBufferPA
// +----------------------------------------------------------------------------
// |  Returns the physical address of the device driver image buffer.
// +----------------------------------------------------------------------------
arc::ulong CArcDevice::CommonBufferPA()
{
    return m_tImgBuffer.ulPhysicalAddr;
}

// +----------------------------------------------------------------------------
// |  CommonBufferSize
// +----------------------------------------------------------------------------
// |  Returns the device driver image buffer size in bytes.
// +----------------------------------------------------------------------------
int CArcDevice::CommonBufferSize()
{
    return int( m_tImgBuffer.dSize );
}

// +----------------------------------------------------------------------------
// |  SetupController
// +----------------------------------------------------------------------------
// |  This is a convenience function that performs a controller setup given
// |  the specified parameters.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> bReset    - 'true' to reset the controller.
// |  <IN> -> bTdl      - 'true' to send TDLs to the PCI board and any board
// |                       whose .lod file is not NULL.
// |  <IN> -> bPower    - 'true' to power on the controller.
// |  <IN> -> dRows     -  The image row dimension (in pixels).
// |  <IN> -> dCols     -  The image col dimension (in pixels).
// |  <IN> -> sTimFile  - The timing board file to load (.lod file).
// |  <IN> -> sUtilFile - The utility board file to load (.lod file).
// |  <IN> -> sPciFile  - The pci board file to load (.lod file).
// |  <IN> -> bAbort    - 'true' to abort; 'false' otherwise. Default: false
// +----------------------------------------------------------------------------
void CArcDevice::SetupController( bool bReset, bool bTdl, bool bPower, int dRows,
								  int dCols, const string sTimFile, const string sUtilFile,
								  const string sPciFile, const bool& bAbort )
{
	int dRetVal = 0;

	if ( bAbort ) { return; }

	//
	// Clear status register
	// +-------------------------------------------------+
	ClearStatus();

	//
	// PCI download
	// +-------------------------------------------------+
	if ( !sPciFile.empty() )
	{
		LoadDeviceFile( sPciFile );
	}

	if ( bAbort ) { return; }

	//
	// Reset controller
	// +-------------------------------------------------+
	if ( bReset )
	{
		ResetController();
	}

	if ( bAbort ) { return; }

	//
	// Hardware tests ( TDL )
	// +-------------------------------------------------+
	if ( bTdl )
	{
		//
		// PCI
		// +---------------------------------------------+
		for ( int i=0; i<1234; i++ )
		{
			if ( bAbort ) { return; }

			dRetVal = Command( PCI_ID, TDL, i );

			if ( dRetVal != i )
			{
				CArcTools::ThrowException(
								"CArcDevice",
								"SetupController",
								"PCI TDL %d/1234 failed. Sent: %d Reply: %d",
								i,
								i,
								dRetVal );
			}
		}

		if ( bAbort ) { return; }

		//
		// TIM
		// +---------------------------------------------+
		if ( !sTimFile.empty() )
		{
			for ( int i=0; i<1234; i++ )
			{
				if ( bAbort ) { return; }

				dRetVal = Command( TIM_ID, TDL, i );

				if ( dRetVal != i )
				{
					CArcTools::ThrowException(
									"CArcDevice",
									"SetupController",
									"TIM TDL %d/1234 failed. Sent: %d Reply: %d",
									i,
									i,
									dRetVal );
				}
			}
		}

		if ( bAbort ) { return; }

		//
		// UTIL
		// +---------------------------------------------+
		if ( !sUtilFile.empty() )
		{
			for ( int i=0; i<1234; i++ )
			{
				if ( bAbort ) { return; }

				dRetVal = Command( UTIL_ID, TDL, i );

				if ( dRetVal != i )
				{
					CArcTools::ThrowException(
									"CArcDevice",
									"SetupController",
									"UTIL TDL %d/1234 failed. Sent: %d Reply: %d",
									i,
									i,
									dRetVal );
				}
			}
		}
	}

	if ( bAbort ) { return; }

	//
	// TIM download
	// +-------------------------------------------------+
	if ( !sTimFile.empty() )
	{
		LoadControllerFile( sTimFile );
	}

	if ( bAbort ) { return; }

	//
	// UTIL download
	// +-------------------------------------------------+
	if ( !sUtilFile.empty() )
	{
		LoadControllerFile( sUtilFile );
	}

	if ( bAbort ) { return; }

	//
	// Power on
	// +-------------------------------------------------+
	if ( bPower )
	{
		dRetVal = Command( TIM_ID, PON );

		if ( dRetVal != DON )
		{
			CArcTools::ThrowException(
							"CArcDevice",
							"SetupController",
							"Power on failed! Reply: 0x%X",
							 dRetVal );
		}
	}

	if ( bAbort ) { return; }

	//
	// Set image dimensions
	// +-------------------------------------------------+
	if ( dRows > 0 && dCols > 0 )
	{
		SetImageSize( dRows, dCols );
	}
	else
	{
		CArcTools::ThrowException(
						"CArcDevice",
						"SetupController",
						"Invalid image dimensions, rows: %d cols: %d",
						 dRows,
						 dCols );
	}
}

// +----------------------------------------------------------------------------
// |  LoadControllerFile
// +----------------------------------------------------------------------------
// |  Loads a SmallCam/GenI/II/III timing or utility file (.lod) into the
// |  specified board.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> sFilename - The SMALLCAM or GENI/II/III TIM or UTIL lod file to
// |                      load.
// |  <IN> -> bValidate - Set to 'true' if the download should be read back
// |                      and checked after every write.
// |  <IN> -> bAbort    - 'true' to stop; 'false' otherwise. Default: false
// +----------------------------------------------------------------------------
void CArcDevice::LoadControllerFile( const string sFilename, bool bValidate, const bool& bAbort )
{
	int dReply = 0;

	if ( bAbort ) { return; }

	//
	// Set the PCI image byte-swapping if SUN hardware.
	//
	SetByteSwapping();

	if ( bAbort ) { return; }

	//
	// Check the system id  ... And YES, PCI_ID is correct!
	//
	dReply = GetControllerId();

	if ( bAbort ) { return; }

	if ( IS_ARC12( dReply ) )
	{
		ResetController();

		if ( bAbort ) { return; }

		LoadSmallCamControllerFile( sFilename, false, bAbort );
	}
	else
	{
		LoadGen23ControllerFile( sFilename, bValidate, bAbort );
	}

	if ( bAbort ) { return; }

	//
	// Wait 10ms. The controller DSP takes 5ms to start processing
	// commands after the download is complete.  This time was
	// measured using the logic analyzer on a PCIe <-> SmallCam
	// system on Nov 16, 2011 at 12:50pm by Bob and Yoating.
	//
	// Without this delay the RCC command that follows will cause
	// problems that result in the DSP not responding to any further
	// commands.

	if ( bAbort ) { return; }

	//
	// Auto-get CC Params after setup
	//
	// Loop N number of times while waiting for the GetCCParams
	// method to return a value other than CNR.  Any value other
	// than CNR will cause the code to continue on.  This is
	// primarily for use on a PCIe <-> SmallCam system.
	m_dCCParam = CNR;

	int dTryCount = 0;

	while ( m_dCCParam == CNR && dTryCount < 500 )
	{
		if ( bAbort ) { break; }

		try
		{
			m_dCCParam = GetCCParams();
		}
		catch ( ... )
		{
		}

		dTryCount++;

		Arc_Sleep( 100 );
	}
}

// +----------------------------------------------------------------------------
// |  LoadSmallCamControllerFile
// +----------------------------------------------------------------------------
// |  Loads a timing or utility file (.lod) into a SmallCam controller.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> sFilename   - The TIM or UTIL lod file to load.
// |  <IN> -> bValidate   - Set to 1 if the download should be read back and
// |                        checked after every write.
// |  <IN> -> bAbort      - 'true' to stop; 'false' otherwise. Default: false
// +----------------------------------------------------------------------------
void CArcDevice::LoadSmallCamControllerFile( const string sFilename, bool bValidate, const bool& bAbort )
{
	int dAddr	= 0;
	int dData	= 0;
	int dReply	= 0;

	std::string sLine;
	std::string sToken;
	CArcTools::CTokenizer cTokenizer;

	std::queue<int>  qData;
	std::vector<int> vData;

	if ( bAbort ) { return; }

	//
	// Verify device connection
	// -------------------------------------------------------------------
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "LoadSmallCamControllerFile",
								   "Not connected to any device." );
	}

	if ( bAbort ) { return; }

	//
	// Open the file for reading.
	// -------------------------------------------------------------------
	ifstream inFile( sFilename.c_str() );

	if ( !inFile.is_open() )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "LoadSmallCamControllerFile",
								   "Cannot open file: %s",
								    sFilename.c_str() );
	}

	if ( bAbort ) { inFile.close(); return; }

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
		if ( sLine.find( "_DATA " ) != std::string::npos )
		{
			cTokenizer.Victim( sLine );
			cTokenizer.Next();	// Dump _DATA string

			//
			// Get the memory type and start address
			// ---------------------------------------------
			cTokenizer.Next();	// Don't need to know memory type
			dAddr = CArcTools::StringToHex( cTokenizer.Next() );

			//
			// The start address must be less than MAX_DSP_START_LOAD_ADDR
			// -------------------------------------------------------------
			if ( dAddr < MAX_DSP_START_LOAD_ADDR )
			{
				//
				// Read the data block and store it in a Q
				// ----------------------------------------
				while ( !inFile.eof() && inFile.peek() != '_' )
				{
					if ( bAbort ) { inFile.close(); return; }

					getline( inFile, sLine );
					cTokenizer.Victim( sLine );

					while ( !( ( sToken = cTokenizer.Next() ).empty() ) )
					{
						if ( bAbort ) { inFile.close(); return; }

						dData = CArcTools::StringToHex( sToken );

						qData.push( dData );

						dAddr++;

					} // while tokenizer next
				} // while not EOF or '_'
			}	// if address < 0x4000
		}	// if not '_DATA'
	}	// while not EOF

	//
	// Write the data from the Q
	// ----------------------------------------
	while ( !qData.empty() )
	{
		if ( vData.size() == 6 )
		{
			//
			// Write the data to the controller ( PCIe actually )
			// --------------------------------------------------------------
			dReply = SmallCamDLoad( SMALLCAM_DLOAD_ID, vData );

			if ( dReply != DON )
			{
				CArcTools::ThrowException(
								"CArcDevice",
								"LoadSmallCamControllerFile",
								"Write to controller TIMING board failed. %s",
								 FormatDLoadString( dReply, SMALLCAM_DLOAD_ID, vData ).c_str() );
			}

			vData.clear();
		}

		vData.push_back( qData.front() );

		qData.pop();
	}

	//
	// Write 'extra' data to the controller ( PCIe actually )
	// --------------------------------------------------------------
	dReply = SmallCamDLoad( SMALLCAM_DLOAD_ID, vData );

	if ( dReply != DON )
	{
		CArcTools::ThrowException(
							"CArcDevice",
							"LoadSmallCamControllerFile",
							"Write to controller TIMING board failed. %s",
							 FormatDLoadString( dReply, SMALLCAM_DLOAD_ID, vData ).c_str() );
	}

	inFile.close();
}

// +----------------------------------------------------------------------------
// |  FormatDLoadString
// +----------------------------------------------------------------------------
// |  Method to bundle command values into a string that can be used to throw
// |  a std::runtime_error exception.
// |
// |  <IN> -> dReply      : The received command reply value.
// |  <IN> -> dBoardId    : The command header.
// |  <IN> -> vData       : The data vector.
// +----------------------------------------------------------------------------
string CArcDevice::FormatDLoadString( int dReply, int dBoardId, vector<int>& vData )
{
	ostringstream oss;

	oss.setf( ios::hex, ios::basefield );
	oss.setf( ios::uppercase );

	oss << "[ 0x" << ( ( dBoardId << 8 ) | ( vData.size() + 1 ) );	// Header

	for ( vector<int>::size_type i=0; i<vData.size(); i++ )
	{
		oss << " 0x" << vData.at( i );
	}

	oss << " -> 0x" << dReply << " ]";					// Reply

	return oss.str();
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
// |  <IN> -> dRows  - The number of rows in the image.
// |  <IN> -> dCols  - The number of columns in the image.
// +----------------------------------------------------------------------------
void CArcDevice::SetImageSize( int dRows, int dCols )
{
	int dReply  = 0;

	//
	// Rows
	// ---------------------------------------
	dReply = Command( TIM_ID, WRM, ( Y_MEM | 2 ), dRows );

	if ( dReply != DON )
	{
		CArcTools::ThrowException(
						"CArcDevice",
						"SetImageSize",
					    "Write image rows: %d -> reply: 0x%X",
						 dRows,
						 dReply );
	}

	//
	// Cols
	// ---------------------------------------
	dReply = Command( TIM_ID, WRM, ( Y_MEM | 1 ), dCols );

	if ( dReply != DON )
	{
		CArcTools::ThrowException(
						"CArcDevice",
						"SetImageSize",
						"Write image cols: %d -> reply: 0x%X",
						 dCols,
						 dReply );
	}

	//
	// Attempt to remap the image buffer if needed
	//
#ifdef WIN32
	if ( ( dRows * dCols ) <= ( 4200 * 4200 ) )
	{
#endif
		int dNewSize = ( dRows * dCols * sizeof( unsigned short ) );

		if ( dNewSize > CommonBufferSize() )
		{
			ReMapCommonBuffer( dNewSize );
		}
#ifdef WIN32
	}
#endif
}

// +----------------------------------------------------------------------------
// | GetImageRows
// +----------------------------------------------------------------------------
// | Returns the image row size (pixels) that has been set on the controller.
// |
// | Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcDevice::GetImageRows()
{
	int dRows = 0;

	dRows = Command( TIM_ID, RDM, ( Y_MEM | 2 ) );

	if ( ContainsError( dRows ) )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "GetImageRows",
							       "Command failed!, reply: 0x%X",
							        dRows );
	}

	return dRows;
}

// +----------------------------------------------------------------------------
// | GetImageCols
// +----------------------------------------------------------------------------
// | Returns the image column size (pixels) that has been set on the controller.
// |
// | Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcDevice::GetImageCols()
{
	int dCols = 0;

	dCols = Command( TIM_ID, RDM, ( Y_MEM | 1 ) );

	if ( ContainsError( dCols ) )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "GetImageCols",
								   "Command failed!, reply: 0x%X",
									dCols );
	}

	return dCols;
}

// +----------------------------------------------------------------------------
// |  GetCCParams
// +----------------------------------------------------------------------------
// |  Returns the available configuration parameters. The parameter bit
// |  definitions are specified in ArcDefs.h.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
int CArcDevice::GetCCParams()
{
	m_dCCParam = Command( TIM_ID, RCC );

	if ( ContainsError( m_dCCParam ) )
	{
		CArcTools::ThrowException(
						"CArcDevice",
						"GetCCParams",
						"Read controller configuration parameters failed. Read: 0x%X",
						 m_dCCParam );
	}

	return m_dCCParam;
}

// +----------------------------------------------------------------------------
// |  IsCCParamSupported
// +----------------------------------------------------------------------------
// |  Returns 'true' if the specified configuration parameter is available on
// |  the controller.  Returns 'false' otherwise. The parameter bit definitions
// |  are specified in ArcDefs.h.
// |
// |  <IN> -> dParameter - The controller parameter to check.
// +----------------------------------------------------------------------------
bool CArcDevice::IsCCParamSupported( int dParameter )
{
	bool bIsSupported = false;

	//
	// Read the CC Param word if it conains an error
	//
	if ( ContainsError( m_dCCParam ) )
	{
		GetCCParams();
	}

	//  Bits 0, 1, 2
	// +-------------------------------------------+
	if ( ( m_dCCParam & 0x7 ) == dParameter )
	{
		bIsSupported = true;
	}

	//  Bits 3, 4
	// +-------------------------------------------+
	else if ( ( m_dCCParam & 0x18 ) == dParameter )
	{
		bIsSupported = true;
	}

	//  Bits 5, 6
	// +-------------------------------------------+
	else if ( ( m_dCCParam & 0x60 ) == dParameter )
	{
		bIsSupported = true;
	}

	//  Bit 7
	// +-------------------------------------------+
	else if ( ( m_dCCParam & 0x80 ) == dParameter )
	{
		bIsSupported = true;
	}

	//  Bits 8, 9
	// +-------------------------------------------+
	else if ( ( m_dCCParam & 0x300 ) == dParameter )
	{
		bIsSupported = true;
	}

	//  Bit 10
	// +-------------------------------------------+
	else if ( ( m_dCCParam & 0x400 ) == dParameter )
	{
		bIsSupported = true;
	}

	//  Bit 11
	// +-------------------------------------------+
	else if ( ( m_dCCParam & 0x800 ) == dParameter )
	{
		bIsSupported = true;
	}

	//  Bits 12, 13
	// +-------------------------------------------+
	else if ( ( m_dCCParam & 0x3000 ) == dParameter )
	{
		bIsSupported = true;
	}

	//  Bit 14
	// +-------------------------------------------+
	else if ( ( m_dCCParam & 0x4000 ) == dParameter )
	{
		bIsSupported = true;
	}

	//  Bits 15, 16
	// +-------------------------------------------+
	else if ( ( m_dCCParam & 0x18000 ) == dParameter )
	{
		bIsSupported = true;
	}

	//  Bits 17, 18, 19
	// +-------------------------------------------+
	else if ( ( m_dCCParam & 0xE0000 ) == dParameter )
	{
		bIsSupported = true;
	}

	//  Bits 20, 21, 22, 23
	// +-------------------------------------------+
	else if ( ( m_dCCParam & 0xF00000 ) == dParameter )
	{
		bIsSupported = true;
	}

	return bIsSupported;
}

// +----------------------------------------------------------------------------
// |  IsCCD
// +----------------------------------------------------------------------------
// |  Returns false if the controller contains an IR video processor board.
// |  Returns true otherwise.
// +----------------------------------------------------------------------------
bool CArcDevice::IsCCD()
{
	bool bIRREV4 = IsCCParamSupported( IRREV4 );
	bool bARC46  = IsCCParamSupported( ARC46  );
	bool bIR8X   = IsCCParamSupported( IR8X   );

	if ( bIRREV4 || bARC46 || bIR8X )
	{
		return false;
	}

	return true;
}

// +----------------------------------------------------------------------------
// |  IsBinningSet
// +----------------------------------------------------------------------------
// |  Returns 'true' if binning is set on the controller, returns 'false'
// |  otherwise.
// +----------------------------------------------------------------------------
bool CArcDevice::IsBinningSet()
{
	int dBinFactor = 0;
	bool bIsSet  = true;

	//
	// Verify device connection
	// -------------------------------------------------------------------
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "IsBinningSet",
								   "Not connected to any device." );
	}

	//
	// Read the column factor from timing board Y:5
	// -------------------------------------------------------------
	dBinFactor = Command( TIM_ID, RDM, ( Y_MEM | 0x5 ) );

	if ( dBinFactor == 1 )
	{
		//
		// Read the row factor from timing board Y:6
		// ---------------------------------------------------------
		dBinFactor = Command( TIM_ID, RDM, ( Y_MEM | 0x6 ) );

		if ( dBinFactor == 1 )
		{
			bIsSet = false;
		}
	}

	return bIsSet;
}

// +----------------------------------------------------------------------------
// |  SetBinning
// +----------------------------------------------------------------------------
// |  Sets the camera controller binning factors for both the rows and columns.
// |  Binning causes each axis to combine xxxFactor number of pixels. For
// |  example, if rows = 1000, cols = 1000, rowFactor = 2, colFactor = 4
// |  then binRows = 500, binCols = 250.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dRows      - The pre-binning number of image rows
// |  <IN>  -> dCols      - The pre-binning number of images columns
// |  <IN>  -> dRowFactor - The binning row factor
// |  <IN>  -> dColFactor - The binning column factor
// |  <OUT> -> pBinRows   - Pointer that will get binned image row value
// |  <OUT> -> pBinCols   - Pointer that will get binned image col value
// +----------------------------------------------------------------------------
void CArcDevice::SetBinning( int dRows, int dCols, int dRowFactor,
							 int dColFactor, int* pBinRows, int* pBinCols )
{
	int dRetVal     = 0;
	int dBinnedRows = dRows;
	int dBinnedCols = dCols;

	//
	// Verify device connection
	// -------------------------------------------------------------------
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "SetBinning",
								   "Not connected to any device." );
	}

	//
	// Write the column factor to timing board Y:5 ( if different )
	// -------------------------------------------------------------
	dRetVal = Command( TIM_ID, WRM, ( Y_MEM | 0x5 ), dColFactor );

	if ( dRetVal != DON )
	{
		CArcTools::ThrowException(
				"CArcDevice",
				"SetBinning",
				"Failed to set binning column factor ( %d ). Command reply: 0x%X",
				 dColFactor,
				 dRetVal );
	}

	dBinnedCols = dCols / dColFactor;

	//
	// Write the row factor to timing board Y:6 ( if different )
	// -------------------------------------------------------------
	dRetVal = Command( TIM_ID, WRM, ( Y_MEM | 0x6 ), dRowFactor );

	if ( dRetVal != DON )
	{
		CArcTools::ThrowException(
				"CArcDevice",
				"SetBinning",
				"Failed to set binning row factor ( %d ). Command reply: 0x%X",
				 dRowFactor,
				 dRetVal );
	}

	dBinnedRows = dRows / dRowFactor;

	if ( pBinRows != NULL ) { *pBinRows = dBinnedRows; }
	if ( pBinCols != NULL ) { *pBinCols = dBinnedCols; }

	SetImageSize( dBinnedRows, dBinnedCols );
}

// +----------------------------------------------------------------------------
// |  UnsetBinning
// +----------------------------------------------------------------------------
// |  Unsets the camera controller binning factors previously set by a call to
// |  CArcDevice::SetBinning().
// |
// |  Throws std::runtime_error on error
// |
// |  <IN>  -> dRows  - The image rows to set on the controller 
// |  <IN>  -> dCols  - The image cols to set on the controller
// +----------------------------------------------------------------------------
void CArcDevice::UnSetBinning( int dRows, int dCols )
{
	int dRetVal = 0;

	//
	// Verify device connection
	// -------------------------------------------------------------------
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "UnSetBinning",
								   "Not connected to any device." );
	}

	//
	// Write the column factor to timing board Y:5
	// -------------------------------------------------------------
	dRetVal = Command( TIM_ID, WRM, ( Y_MEM | 0x5 ), 1 );

	if ( dRetVal != DON )
	{
		CArcTools::ThrowException(
				"CArcDevice",
				"UnSetBinning",
				"Failed to set binning column factor ( 1 ). Command reply: 0x%X",
				 dRetVal );
	}

	//
	// Write the row factor to timing board Y:6
	// -------------------------------------------------------------
	dRetVal = Command( TIM_ID, WRM, ( Y_MEM | 0x6 ), 1 );

	if ( dRetVal != DON )
	{
		CArcTools::ThrowException(
				"CArcDevice",
				"UnSetBinning",
				"Failed to set binning row factor ( 1 ). Command reply: 0x%X",
				 dRetVal );
	}

	//
	// Update the image dimensions on the controller
	// -------------------------------------------------------------
	SetImageSize( dRows, dCols );
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
// |  Throws std::runtime_error on error
// |
// |  <OUT> -> dOldRows - Returns the original row size set on the controller
// |  <OUT> -> dOldCols - Returns the original col size set on the controller
// |  <IN> -> #1 - dRow - The row # of the center of the sub-array in pixels
// |  <IN> -> #2 - dCol - The col # of the center of the sub-array in pixels
// |  <IN> -> #3 - dSubRows - The sub-array row count in pixels
// |  <IN> -> #4 - dSubCols - The sub-array col count in pixels
// |  <IN> -> #5 - dBiasOffset - The offset of the bias region in pixels
// |  <IN> -> #6 - dBiasCols - The col count of the bias region in pixels
// +----------------------------------------------------------------------------
void CArcDevice::SetSubArray( int& dOldRows, int& dOldCols, int dRow, int dCol,
								 int dSubRows, int dSubCols, int dBiasOffset,
								 int dBiasCols )
{
	int dRetVal = 0;

	//
	// Verify device connection
	// -------------------------------------------------------------------
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "SetSubArray",
								   "Not connected to any device." );
	}

	//
	// Get the current image dimensions
	//
	dOldRows = GetImageRows();
	dOldCols = GetImageCols();

	//
	// Set the new image dimensions
	//
	SetImageSize( dSubRows, dSubCols + dBiasCols );

	//
	// Set the sub-array size
	//
	dRetVal = Command( TIM_ID, SSS, dBiasCols, dSubCols, dSubRows );

	if ( dRetVal != DON )
	{
		CArcTools::ThrowException(
						"CArcDevice",
						"SetSubArray",
						"Failed to set sub-array SIZE on controller. Reply: 0x%X",
						 dRetVal );
	}

	//
	// Set the sub-array position
	//
	dRetVal = Command( TIM_ID,
					   SSP,
					   dRow - ( dSubRows / 2 ),
					   dCol - ( dSubCols / 2 ),
					   dBiasOffset - dCol - ( dSubCols / 2 ) );

	if ( dRetVal != DON )
	{
		CArcTools::ThrowException(
						"CArcDevice",
						"SetSubArray",
						"Failed to set sub-array POSITION on controller. Reply: 0x%X",
						 dRetVal );
	}
}

// +----------------------------------------------------------------------------
// |  UnSetSubArray
// +----------------------------------------------------------------------------
// |  Unsets the camera controller from sub-array mode.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> dRows  - The row size to set on the controller
// |  <IN> -> dCols  - The col size to set on the controller
// +----------------------------------------------------------------------------
void CArcDevice::UnSetSubArray( int dRows, int dCols )
{
	int dRetVal = 0;

	//
	// Verify device connection
	// -------------------------------------------------------------------
	if ( !IsOpen() )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "UnSetSubArray",
								   "Not connected to any device." );
	}

	//
	// Set the new image dimensions
	//
	SetImageSize( dRows, dCols );

	//
	// Set the sub-array size to zero
	//
	dRetVal = Command( TIM_ID, SSS, 0, 0, 0 );

	if ( dRetVal != DON )
	{
		CArcTools::ThrowException(
					"CArcDevice",
					"UnSetSubArray",
					"Failed to set sub-array SIZE on controller. Reply: 0x%X",
					 dRetVal );
	}
}

// +----------------------------------------------------------------------------
// |  IsSyntheticImageMode
// +----------------------------------------------------------------------------
// |  Returns true if synthetic readout is turned on. Otherwise, returns
// |  'false'. A synthetic image looks like: 0, 1, 2, ..., 65535, 0, 1, 2,
// |  ..., 65353, etc
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
bool CArcDevice::IsSyntheticImageMode()
{
	int  dSynBitMask = 0x00000400;
	int  dStatus     = 0;
	bool bIsSet      = false;

	//
	// Read the controller status word from the TIM X:0
	//
	dStatus = Command( TIM_ID, RDM, ( X_MEM | 0 ) );

	if ( ContainsError( dStatus ) )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "IsSyntheticImageMode",
								   "Failed to read controller status: 0x%X",
								    dStatus );
	}

	//
	// Check the PCI board for synthetic image mode. Bit 10
	// of TIM X:0 should be zero.
	//
	if ( ( dStatus & dSynBitMask ) == dSynBitMask )
	{
		bIsSet = true;
	}

	return bIsSet;
}

// +----------------------------------------------------------------------------
// |  SetSyntheticImageMode
// +----------------------------------------------------------------------------
// |  If mode is 'true', then synthetic readout will be turned on. Set mode
// |  to 'false' to turn it off. A synthetic image looks like: 0, 1, 2, ...,
// |  65535, 0, 1, 2, ..., 65353, etc
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> bMode  - 'true' to turn it on, 'false' to turn if off.
// +----------------------------------------------------------------------------
void CArcDevice::SetSyntheticImageMode( bool bMode )
{
	int dClearBitMask	= 0x00000400;
	int dStatus			= 0;
	int dReply			= 0;

	//
	// Read the controller status word from the TIM X:0
	//
	dStatus = Command( TIM_ID, RDM, ( X_MEM | 0 ) );

	if ( ContainsError( dStatus ) )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "SetSyntheticImageMode",
								   "Failed to read controller status: 0x%X",
								    dStatus );
	}

	//
	// Set the PCI board to synthetic image mode.
	// Set/clear bit 10 of TIM X:0
	if ( bMode )
	{
		dReply = Command( TIM_ID,
						  WRM,
						  ( X_MEM | 0 ),
						  ( dStatus | dClearBitMask ) );
	}
	else
	{
		dReply = Command( TIM_ID,
						  WRM,
						  ( X_MEM | 0 ),
						  ( dStatus & ~dClearBitMask ) );
	}

	if ( dReply != DON )
	{
		if ( bMode )
		{
			CArcTools::ThrowException(
								"CArcDevice",
								"SetSyntheticImageMode",
								"Controller not set to synthetic image mode." );
		}
		else
		{
			CArcTools::ThrowException(
								"CArcDevice",
								"SetSyntheticImageMode",
								"Controller not set to normal image mode." );
		}
	}
}


// +----------------------------------------------------------------------------
// |  SetOpenShutter
// +----------------------------------------------------------------------------
// |  Sets whether or not the shutter will open when an exposure is started
// |  ( using SEX ).
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> shouldOpen - Set to 'true' if the shutter should open during
// |                       expose. Set to 'false' to keep the shutter closed.
// +----------------------------------------------------------------------------
void CArcDevice::SetOpenShutter( bool bShouldOpen )
{
	int dShutterVal = -1;
	int dRetVal     = -1;

	dShutterVal = Command( TIM_ID,
						   RDM,
						   ( X_MEM | 0 ) );

	if ( dShutterVal >= 0 )
	{
		if ( bShouldOpen )
		{
			dRetVal = Command(  TIM_ID,
								WRM,
								( X_MEM | 0 ),
								( dShutterVal | OPEN_SHUTTER_POSITION ) );
		}	
		else
		{
			dRetVal = Command(  TIM_ID,
								WRM,
								( X_MEM | 0 ),
								( dShutterVal & CLOSED_SHUTTER_POSITION ) );
		}

		if ( dRetVal != DON )
		{
			CArcTools::ThrowException(
							"CArcDevice",
							"SetOpenShutter",
							"Shutter position failed to be set! reply: 0x%X",
							 dRetVal );
		}
	}
	else
	{
		CArcTools::ThrowException(
							"CArcDevice",
							"SetOpenShutter",
							"Failed to read shutter position value at X:0!" );
	}
}

// +----------------------------------------------------------------------------
// |  Expose
// +----------------------------------------------------------------------------
// |  Starts an exposure using the specified exposure time and whether or not
// |  to open the shutter. Callbacks for the elapsed exposure time and image
// |  data readout can be used.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> fExpTime - The exposure time ( in seconds ).
// |  <IN> -> bOpenShutter - Set to 'true' if the shutter should open during the
// |                         exposure. Set to 'false' to keep the shutter closed.
// |  <IN> -> dRows - The image row size ( in pixels ).
// |  <IN> -> dCols - The image column size ( in pixels ).
// |  <IN> -> fExpTime - The exposure time ( in seconds ).
// |  <IN> -> bAbort - Pointer to boolean value that can cause the readout
// |                   method to abort/stop either exposing or image readout.
// |                   NULL by default.
// |  <IN> -> pExpIFace - Function pointer to CExpIFace class. NULL by default.
// +----------------------------------------------------------------------------
void CArcDevice::Expose( float fExpTime, int dRows, int dCols, const bool& bAbort,
						 CExpIFace* pExpIFace, bool bOpenShutter )
{
	float fElapsedTime		= fExpTime;
	bool  bInReadout		= false;
	int   dTimeoutCounter	= 0;
	int   dLastPixelCount	= 0;
	int   dPixelCount		= 0;
	int   dExposeCounter	= 0;

	//
	// Check for adequate buffer size
	//
	if ( int( dRows * dCols * sizeof( unsigned short ) ) > CommonBufferSize() )
	{
		CArcTools::ThrowException(
							"CArcDevice",
							"Expose",
							"Image dimensions [ %d x %d ] exceed buffer size: %d. %s.",
							dCols,
							dRows,
							CommonBufferSize(),
							"Try calling ReMapCommonBuffer()" );
	}

	//
	// Set the shutter position
	//
	SetOpenShutter( bOpenShutter );

	//
	// Set the exposure time
	//
	int dRetVal  = Command( TIM_ID,
							SET,
							int( fExpTime * 1000.0 ) );

	if ( dRetVal != DON )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "Expose",
								   "Set exposure time failed. Reply: 0x%X",
									dRetVal );
	}

	//
	// Start the exposure
	//
	dRetVal = Command( TIM_ID, SEX );

	if ( dRetVal != DON )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "Expose",
								   "Start exposure command failed. Reply: 0x%X",
									dRetVal );
	}

	while ( dPixelCount < ( dRows * dCols ) )
	{
		if ( IsReadout() )
		{
			bInReadout = true;
		}

		// ----------------------------
		// READ ELAPSED EXPOSURE TIME
		// ----------------------------
		// Checking the elapsed time > 1 sec. is to prevent race conditions with
		// sending RET while the PCI board is going into readout. Added check
		// for exposure_time > 1 sec. to prevent RET error.
		if ( !bInReadout && fElapsedTime > 1.1f && dExposeCounter >= 5 && fExpTime > 1.0f )
		{
			// Ignore all RET timeouts
			try
			{
				// Read the elapsed exposure time.
				dRetVal = Command( TIM_ID, RET );

				if ( dRetVal != ROUT )
				{
					if ( ContainsError( dRetVal ) || ContainsError( dRetVal, 0, int( fExpTime * 1000 ) ) )
					{
						StopExposure();

						CArcTools::ThrowException( "CArcDevice",
												   "Expose",
												   "Failed to read elapsed time!" );
					}

					if ( bAbort )
					{
						StopExposure();

						CArcTools::ThrowException( "CArcDevice",
												   "Expose",
												   "Expose Aborted!" );
					}

					dExposeCounter  = 0;
					fElapsedTime    = fExpTime - float(dRetVal)/ 1000 ;

					if ( pExpIFace != NULL )
					{
						pExpIFace->ExposeCallback( fElapsedTime );
					}
				}
			}
			catch ( ... ) {}
		}

		dExposeCounter++;

		// ----------------------------
		// READOUT PIXEL COUNT
		// ----------------------------
		if ( bAbort )
		{
			StopExposure();

			CArcTools::ThrowException( "CArcDevice",
									   "Expose",
									   "Expose Aborted!" );
		}

		// Save the last pixel count for use by the timeout counter.
		dLastPixelCount = dPixelCount;
		dPixelCount = GetPixelCount();

		if ( ContainsError( dPixelCount ) )
		{
			StopExposure();

			CArcTools::ThrowException( "CArcDevice",
									   "Expose",
									   "Failed to read pixel count!" );
		}

		if ( bAbort )
		{
			StopExposure();

			CArcTools::ThrowException( "CArcDevice",
									   "Expose",
									   "Expose Aborted!" );
		}

		if ( bInReadout && pExpIFace != NULL )
		{
			pExpIFace->ReadCallback( dPixelCount );
		}

		if ( bAbort )
		{
			StopExposure();

			CArcTools::ThrowException( "CArcDevice",
									   "Expose",
									   "Expose Aborted!" );
		}

		// If the controller's in READOUT, then increment the timeout
		// counter. Checking for readout prevents timeouts when clearing
		// large and/or slow arrays.
		if ( bInReadout && dPixelCount == dLastPixelCount )
		{
			dTimeoutCounter++;
		}
		else
		{
			dTimeoutCounter = 0;
		}

		if ( bAbort )
		{
			StopExposure();

			CArcTools::ThrowException( "CArcDevice",
									   "Expose",
									   "Expose Aborted!" );
		}

		if ( dTimeoutCounter >= READ_TIMEOUT )
		{
			StopExposure();

			CArcTools::ThrowException( "CArcDevice",
									   "Expose",
									   "Read Timeout!" );
		}

		Arc_Sleep( 25 );
	}
}

// +----------------------------------------------------------------------------
// |  Continuous
// +----------------------------------------------------------------------------
// |  This method can be called to start continuous readout.  A callback for 
// |  each frame read out can be used to process the frame.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> dRows - The image row size ( in pixels ).
// |  <IN> -> dCols - The image column size ( in pixels ).
// |  <IN> -> dNumOfFrames - The number of frames to take.
// |  <IN> -> fExpTime - The exposure time ( in seconds ).
// |  <IN> -> bAbort - 'true' to cause the readout method to abort/stop either
// |                    exposing or image readout. Default: false
// |  <IN> -> pConIFace - Function pointer to callback for frame completion.
// |                       NULL by default.
// |  <IN> -> bOpenShutter - 'true' to open the shutter during expose; 'false'
// |                         otherwise.
// +----------------------------------------------------------------------------
void CArcDevice::Continuous( int dRows, int dCols, int dNumOfFrames, float fExpTime,
							 const bool& bAbort, CConIFace* pConIFace, bool bOpenShutter )
{
	int dFramesPerBuffer   = 0;
	int dPCIFrameCount     = 0;
	int dLastPCIFrameCount = 0;
	int dFPBCount          = 0;

	int dImageSize         = dRows * dCols * sizeof( unsigned short );
	int dBoundedImageSize  = GetContinuousImageSize( dImageSize );

	//
	// Check for adequate buffer size
	//
	if ( dImageSize > CommonBufferSize() )
	{
		CArcTools::ThrowException(
							"CArcDevice",
							"Continuous",
							"Image dimensions [ %d x %d ] exceed buffer size: %d. %s.",
							dCols,
							dRows,
							CommonBufferSize(),
							"Try calling ReMapCommonBuffer()" );
	}

	//
	// Check for valid frame count
	//
	if ( dNumOfFrames <= 0 )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "Continuous",
								   "Number of frames must be > 0" );
	}

	if ( bAbort )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "Continuous",
								   "Continuous readout aborted by user!" );
	}

	dFramesPerBuffer = ( int )floor( float( CommonBufferSize() / dBoundedImageSize ) );

	if ( bAbort )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "Continuous",
								   "Continuous readout aborted by user!" );
	}

	try
	{
		// Set the frames-per-buffer
		int dRetVal = Command( TIM_ID, FPB, dFramesPerBuffer );

		if ( dRetVal != DON )
		{
			CArcTools::ThrowException(
						"CArcDevice",
						"Continuous",
						"Failed to set the frames per buffer (FPB). Reply: 0x%X",
						 dRetVal );
		}

		if ( bAbort )
		{
			CArcTools::ThrowException( "CArcDevice",
									   "Continuous",
									   "Continuous readout aborted by user!" );
		}

		// Set the number of frames-to-take
		dRetVal = Command( TIM_ID, SNF, dNumOfFrames );

		if ( dRetVal != DON )
		{
			CArcTools::ThrowException(
						"CArcDevice",
						"Continuous",
						"Failed to set the number of frames (SNF). Reply: 0x%X",
						 dRetVal );
		}

		if ( bAbort )
		{
			CArcTools::ThrowException( "CArcDevice",
									   "Continuous",
									   "Continuous readout aborted by user!" );
		}

		//
		// Set the shutter position
		//
		SetOpenShutter( bOpenShutter );

		//
		// Set the exposure time
		//
		int dExpTime = ( int )( fExpTime * 1000.0 );
		dRetVal = Command( TIM_ID, SET, dExpTime );

		if ( dRetVal != DON )
		{
			CArcTools::ThrowException( "CArcDevice",
									   "Continuous",
									   "Set exposure time failed. Reply: 0x%X",
										dRetVal );
		}

		//
		// Start the exposure
		//
		dRetVal = Command( TIM_ID, SEX );

		if ( dRetVal != DON )
		{
			CArcTools::ThrowException( "CArcDevice",
									   "Continuous",
									   "Start exposure command failed. Reply: 0x%X",
										dRetVal );
		}

		if ( bAbort )
		{
			CArcTools::ThrowException( "CArcDevice",
									   "Continuous",
									   "Continuous readout aborted by user!" );
		}

		// Read the images
		while ( dPCIFrameCount < dNumOfFrames )
		{
			if ( bAbort )
			{
				CArcTools::ThrowException( "CArcDevice",
										   "Continuous",
										   "Continuous readout aborted by user!" );
			}

			dPCIFrameCount = GetFrameCount();

			if ( bAbort )
			{
				CArcTools::ThrowException( "CArcDevice",
										   "Continuous",
										   "Continuous readout aborted by user!" );
			}

			if ( dFPBCount >= dFramesPerBuffer )
			{
					dFPBCount = 0;
			}

			if ( dPCIFrameCount > dLastPCIFrameCount )
			{
				// Call external deinterlace and fits file functions here
				if ( pConIFace != NULL )
				{
					pConIFace->FrameCallback( dFPBCount,
											  dPCIFrameCount,
											  dRows,
											  dCols,
											  ( ( unsigned char * )CommonBufferVA() )
											  + dFPBCount * dBoundedImageSize );
				}

				dLastPCIFrameCount = dPCIFrameCount;
				dFPBCount++;
			}
		}

		// Set back to single image mode
		dRetVal = Command( TIM_ID, SNF, 1 );

		if ( dRetVal != DON )
		{
			CArcTools::ThrowException(
						"CArcDevice",
						"Continuous",
						"Failed to set number of frames (SNF) to 1. Reply: 0x%X",
						 dRetVal );
		}
	}
	catch ( std::runtime_error &e )
	{
		// Set back to single image mode
		StopContinuous();

		throw e;
	}
}

// +----------------------------------------------------------------------------
// |  StopContinuous
// +----------------------------------------------------------------------------
// |  Sends abort expose/readout and sets the controller back into single
// |  read mode.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcDevice::StopContinuous()
{
	StopExposure();

	int dRetVal = Command( TIM_ID, SNF, 1 );

	if ( dRetVal != DON )
	{
		CArcTools::ThrowException(
					"CArcDevice",
					"StopContinuous",
					"Failed to set number of frames ( SNF ) to 1. Reply: 0x%X",
					 dRetVal );
	}
}

// +----------------------------------------------------------------------------
// |  Check the specified value for error replies:
// |  TOUT, ROUT, HERR, ERR, SYR, RST
// +----------------------------------------------------------------------------
// |  Returns 'true' if the specified value matches 'TOUT, 'ROUT', 'HERR',
// |  'ERR', 'SYR' or 'RST'. Returns 'false' otherwise.
// |
// |  <IN> -> dWord - The value to check
// +----------------------------------------------------------------------------
bool CArcDevice::ContainsError( int dWord )
{
	bool bError = false;

	if ( dWord == TOUT || dWord == ERR || dWord == SYR  ||
		 dWord == CNR  || dWord == RST || dWord == ROUT ||
		 dWord == HERR )
	{
		bError = true;
	}

	return bError;
}

// +----------------------------------------------------------------------------
// |  Check that the specified value falls within the specified range.
// +----------------------------------------------------------------------------
// |  Returns 'true' if the specified value falls within the specified range.
// |  Returns 'false' otherwise.
// |
// |  <IN> -> dWord    - The value to check
// |  <IN> -> dWordMin - The minimum range value ( not inclusive )
// |  <IN> -> dWordMax - The maximum range value ( not inclusive )
// +----------------------------------------------------------------------------
bool CArcDevice::ContainsError( int dWord, int dWordMin, int dWordMax )
{
	bool bError = false;

	if ( dWord < dWordMin || dWord > dWordMax )
	{
		bError = true;
	}

	return bError;
}

// +----------------------------------------------------------------------------
// |  GetNextLoggedCmd
// +----------------------------------------------------------------------------
// |  Pops the first message from the command logger and returns it. If the
// |  logger is empty, then NULL is returned.
// +----------------------------------------------------------------------------
const string CArcDevice::GetNextLoggedCmd()
{
	if ( !m_cApiLog.Empty() )
	{
		return string( m_cApiLog.GetNext() );
	}
	else
	{
		return string( "" );
	}
}

// +----------------------------------------------------------------------------
// |  GetLoggedCmdCount
// +----------------------------------------------------------------------------
// |  Returns the available command message count.
// +----------------------------------------------------------------------------
int CArcDevice::GetLoggedCmdCount()
{
	return m_cApiLog.GetLogCount();
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
// |  <IN> -> bOnOff - 'true' to turn loggin on; 'false' otherwise.
// +----------------------------------------------------------------------------
void CArcDevice::SetLogCmds( bool bOnOff )
{
	m_bStoreCmds = bOnOff;
}

////////////////////////////////////////////////////////////////////////////////
//	TEMPERATURE
////////////////////////////////////////////////////////////////////////////////
// +----------------------------------------------------------------------------
// |  LoadTemperatureCtrlData
// +----------------------------------------------------------------------------
// |  Loads temperature control constants from the specified file.  The default
// |  constants are stored in TempCtrl.h and cannot be permanently overwritten.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> sFilename  - The filename of the temperature control constants
// |                       file.
// +----------------------------------------------------------------------------
void CArcDevice::LoadTemperatureCtrlData( const string sFilename )
{
	ifstream ifs( sFilename.c_str() );
	string sBuf;

	if ( ifs.is_open() )
	{
		while ( !ifs.eof() )
		{
			getline( ifs, sBuf );

			if ( sBuf.find( "//" ) != string::npos )
				continue;

			if ( sBuf.find( TMPCTRL_DT670_COEFF_1_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				gTmpCtrl_DT670Coeff1 = atof( sBuf.c_str() );
			}

			else if ( sBuf.find( TMPCTRL_DT670_COEFF_2_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				gTmpCtrl_DT670Coeff2 = atof( sBuf.c_str() );
			}

			else if ( sBuf.find( TMPCTRL_SDADU_OFFSET_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				gTmpCtrl_SDAduOffset = atof( sBuf.c_str() );
			}

			else if ( sBuf.find( TMPCTRL_SDADU_PER_VOLT_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				gTmpCtrl_SDAduPerVolt = atof( sBuf.c_str() );
			}

			else if ( sBuf.find( TMPCTRL_HGADU_OFFSET_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				gTmpCtrl_HGAduOffset = atof( sBuf.c_str() );
			}

			else if ( sBuf.find( TMPCTRL_HGADU_PER_VOLT_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				gTmpCtrl_HGAduPerVolt = atof( sBuf.c_str() );
			}

			else if ( sBuf.find( TMPCTRL_SDNUMBER_OF_READS_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				gTmpCtrl_SDNumberOfReads = atoi( sBuf.c_str() );
			}

			else if ( sBuf.find( TMPCTRL_SDVOLT_TOLERANCE_TRIALS_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				gTmpCtrl_SDVoltToleranceTrials = atoi( sBuf.c_str() );
			}

			else if ( sBuf.find( TMPCTRL_SDVOLT_TOLERANCE_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				gTmpCtrl_SDVoltTolerance = atof( sBuf.c_str() );
			}

			else if ( sBuf.find( TMPCTRL_SDDEG_TOLERANCE_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				gTmpCtrl_SDDegTolerance = atof( sBuf.c_str() );
			}

			else if ( sBuf.find( TMPCTRL_SD2_12K_COEFF_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				TmpCtrl_SD_2_12K.vu = atof( sBuf.c_str() );

				getline( ifs, sBuf );
				TmpCtrl_SD_2_12K.vl = atof( sBuf.c_str() );

				getline( ifs, sBuf );
				TmpCtrl_SD_2_12K.count = atoi( sBuf.c_str() );

				for ( int i=0; i<TmpCtrl_SD_2_12K.count; i++ )
				{
					getline( ifs, sBuf );
					TmpCtrl_SD_2_12K.coeff[ i ] = atof( sBuf.c_str() );
				}
			}

			else if ( sBuf.find( TMPCTRL_SD12_24K_COEFF_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				TmpCtrl_SD_12_24K.vu = atof( sBuf.c_str() );

				getline( ifs, sBuf );
				TmpCtrl_SD_12_24K.vl = atof( sBuf.c_str() );

				getline( ifs, sBuf );
				TmpCtrl_SD_12_24K.count = atoi( sBuf.c_str() );

				for ( int i=0; i<TmpCtrl_SD_12_24K.count; i++ )
				{
					getline( ifs, sBuf );
					TmpCtrl_SD_12_24K.coeff[ i ] = atof( sBuf.c_str() );
				}
			}

			else if ( sBuf.find( TMPCTRL_SD24_100K_COEFF_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				TmpCtrl_SD_24_100K.vu = atof( sBuf.c_str() );

				getline( ifs, sBuf );
				TmpCtrl_SD_24_100K.vl = atof( sBuf.c_str() );

				getline( ifs, sBuf );
				TmpCtrl_SD_24_100K.count = atoi( sBuf.c_str() );

				for ( int i=0; i<TmpCtrl_SD_24_100K.count; i++ )
				{
					getline( ifs, sBuf );
					TmpCtrl_SD_24_100K.coeff[ i ] = atof( sBuf.c_str() );
				}
			}

			else if ( sBuf.find( TMPCTRL_SD100_475K_COEFF_KEY ) != string::npos )
			{
				getline( ifs, sBuf );
				TmpCtrl_SD_100_475K.vu = atof( sBuf.c_str() );

				getline( ifs, sBuf );
				TmpCtrl_SD_100_475K.vl = atof( sBuf.c_str() );

				getline( ifs, sBuf );
				TmpCtrl_SD_100_475K.count = atoi( sBuf.c_str() );

				for ( int i=0; i<TmpCtrl_SD_100_475K.count; i++ )
				{
					getline( ifs, sBuf );
					TmpCtrl_SD_100_475K.coeff[ i ] = atof( sBuf.c_str() );
				}
			}
		}
	}
	else
	{
		CArcTools::ThrowException( "CArcDevice",
								   "LoadTemperatureCtrlData",
								   "Failed to open temperature control file: %s",
								    sFilename.c_str() );
	}

	ifs.close();
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
// |  <IN> -> sFilename  - The filename of the temperature control constants
// |                       file to save.
// +----------------------------------------------------------------------------
void CArcDevice::SaveTemperatureCtrlData( const string sFilename )
{
	ofstream ofs( sFilename.c_str() );

	if ( ofs.is_open() )
	{
		ofs.precision( 10 );

		ofs << "// _____________________________________________________________" << endl;
		ofs << "//" << endl;
		ofs << "// TEMPERATURE CONTROL FILE" << endl;
		ofs << "// _____________________________________________________________" << endl;
		ofs << endl;

		ofs << "// +-----------------------------------------------------------" << endl;
		ofs << "// | Define Temperature Coeffients for DT-670 Sensor (SmallCam)" << endl;
		ofs << "// |" << endl;
		ofs << "// | [TMPCTRL_DT670_COEFF_1] - DT-670 coefficient #1." << endl;
		ofs << "// |" << endl;
		ofs << "// | [TMPCTRL_DT670_COEFF_2] - DT-670 coefficient #2." << endl;
		ofs << "// +-----------------------------------------------------------" << endl;
		ofs << TMPCTRL_DT670_COEFF_1_KEY << endl;
		ofs << gTmpCtrl_DT670Coeff1 << endl << endl;

		ofs << TMPCTRL_DT670_COEFF_2_KEY << endl;
		ofs << gTmpCtrl_DT670Coeff2 << endl << endl;

		ofs << "// +-----------------------------------------------------------" << endl;
		ofs << "// | Define Temperature Coeffients for CY7 Sensor" << endl;
		ofs << "// |" << endl;
		ofs << "// | [TMPCTRL_SDADU_OFFSET] - The standard silicon diode ADU" << endl;
		ofs << "// | offset." << endl;
		ofs << "// |" << endl;
		ofs << "// | [TMPCTRL_SDADU_PER_VOLT] - The standard silicon diode" << endl;
		ofs << "// | ADU / Volt." << endl;
		ofs << "// +-----------------------------------------------------------" << endl;
		ofs << TMPCTRL_SDADU_OFFSET_KEY << endl;
		ofs << gTmpCtrl_SDAduOffset << endl << endl;

		ofs << TMPCTRL_SDADU_PER_VOLT_KEY << endl;
		ofs << gTmpCtrl_SDAduPerVolt << endl << endl;

		ofs << "// +-----------------------------------------------------------" << endl;
		ofs << "// | Define Temperature Coeffients for High Gain Utility Board" << endl;
		ofs << "// |" << endl;
		ofs << "// | [TMPCTRL_HGADU_OFFSET] - The high gain utility board ADU" << endl;
		ofs << "// | offset." << endl;
		ofs << "// |" << endl;
		ofs << "// | [TMPCTRL_HGADU_PER_VOLT] - The high gain utility board" << endl;
		ofs << "// | ADU / Volt." << endl;
		ofs << "// +-----------------------------------------------------------" << endl;
		ofs << TMPCTRL_HGADU_OFFSET_KEY << endl;
		ofs << gTmpCtrl_HGAduOffset << endl << endl;

		ofs << TMPCTRL_HGADU_PER_VOLT_KEY << endl;
		ofs << gTmpCtrl_HGAduPerVolt << endl << endl;

		ofs << "// + ----------------------------------------------------------" << endl;
		ofs << "// | Define temperature control variables" << endl;
		ofs << "// |" << endl;
		ofs << "// | [TMPCTRL_SDNUMBER_OF_READS] - The number of temperature" << endl;
		ofs << "// | reads to average for each GetArrayTemperature call." << endl;
		ofs << "// |" << endl;
		ofs << "// | [TMPCTRL_SDVOLT_TOLERANCE_TRIALS] - The number of times to" << endl;
		ofs << "// | calculate the voltage." << endl;
		ofs << "// |" << endl;
		ofs << "// | [TMPCTRL_SDVOLT_TOLERANCE] - The voltage tolerance in" << endl;
		ofs << "// | volts." << endl;
		ofs << "// |" << endl;
		ofs << "// | [TMPCTRL_SDDEG_TOLERANCE] - The temperature tolerance in" << endl;
		ofs << "// | degrees celcius." << endl;
		ofs << "// + ----------------------------------------------------------" << endl;
		ofs << TMPCTRL_SDNUMBER_OF_READS_KEY << endl;
		ofs << gTmpCtrl_SDNumberOfReads << endl << endl;

		ofs << TMPCTRL_SDVOLT_TOLERANCE_TRIALS_KEY << endl;
		ofs << gTmpCtrl_SDVoltToleranceTrials << endl << endl;

		ofs << TMPCTRL_SDVOLT_TOLERANCE_KEY << endl;
		ofs << gTmpCtrl_SDVoltTolerance << endl << endl;

		ofs << TMPCTRL_SDDEG_TOLERANCE_KEY << endl;
		ofs << gTmpCtrl_SDDegTolerance << endl << endl;

		ofs << "// +-----------------------------------------------------------" << endl;
		ofs << "// | Define Temperature Coeffients for Non-linear Silicone" << endl;
		ofs << "// | Diode ( SD )" << endl;
		ofs << "// |" << endl;
		ofs << "// | [Coeff Set Name] can be one of the following:" << endl;
		ofs << "// | --------------------------------------------" << endl;
		ofs << "// | [TMPCTRL_SD2_12K_COEFF]    - Coefficients for 2 - 12K" << endl;
		ofs << "// | [TMPCTRL_SD12_24K_COEFF]   - Coefficients for 12 - 24K" << endl;
		ofs << "// | [TMPCTRL_SD24_100K_COEFF]  - Coefficients for 24 - 100K" << endl;
		ofs << "// | [TMPCTRL_SD100_475K_COEFF] - Coefficients for 100 - 475K" << endl;
		ofs << "// |" << endl;
		ofs << "// | FORMAT:" << endl;
		ofs << "// | ---------------------" << endl;
		ofs << "// | [Coeff Set Name]" << endl;
		ofs << "// | Voltage Upper Limit" << endl;
		ofs << "// | Voltage Lower Limit" << endl;
		ofs << "// | Coefficient Count (N)" << endl;
		ofs << "// | Coefficient #1" << endl;
		ofs << "// | ..." << endl;
		ofs << "// | Coefficient #N" << endl;
		ofs << "// +-----------------------------------------------------------" << endl;
		ofs << TMPCTRL_SD2_12K_COEFF_KEY << endl;
		ofs << TmpCtrl_SD_2_12K.vu << endl;
		ofs << TmpCtrl_SD_2_12K.vl << endl;
		ofs << TmpCtrl_SD_2_12K.count << endl;

		for ( int i=0; i<TmpCtrl_SD_2_12K.count; i++ )
		{
			ofs << TmpCtrl_SD_2_12K.coeff[ i ] << endl;
		}
		ofs << endl;

		ofs << TMPCTRL_SD12_24K_COEFF_KEY << endl;
		ofs << TmpCtrl_SD_12_24K.vu << endl;
		ofs << TmpCtrl_SD_12_24K.vl << endl;
		ofs << TmpCtrl_SD_12_24K.count << endl;

		for ( int i=0; i<TmpCtrl_SD_12_24K.count; i++ )
		{
			ofs << TmpCtrl_SD_12_24K.coeff[ i ] << endl;
		}
		ofs << endl;

		ofs << TMPCTRL_SD24_100K_COEFF_KEY << endl;
		ofs << TmpCtrl_SD_24_100K.vu << endl;
		ofs << TmpCtrl_SD_24_100K.vl << endl;
		ofs << TmpCtrl_SD_24_100K.count << endl;

		for ( int i=0; i<TmpCtrl_SD_24_100K.count; i++ )
		{
			ofs << TmpCtrl_SD_24_100K.coeff[ i ] << endl;
		}
		ofs << endl;

		ofs << TMPCTRL_SD100_475K_COEFF_KEY << endl;
		ofs << TmpCtrl_SD_100_475K.vu << endl;
		ofs << TmpCtrl_SD_100_475K.vl << endl;
		ofs << TmpCtrl_SD_100_475K.count << endl;

		for ( int i=0; i<TmpCtrl_SD_100_475K.count; i++ )
		{
			ofs << TmpCtrl_SD_100_475K.coeff[ i ] << endl;
		}
		ofs << endl;
	}
	else
	{
		CArcTools::ThrowException( "CArcDevice",
								   "SaveTemperatureCtrlData",
								   "Failed to save temperature control file: %s",
								    sFilename.c_str() );
	}

	ofs.close();
}

// +----------------------------------------------------------------------------
// |  SetDefaultTemperatureValues
// +----------------------------------------------------------------------------
// |  Sets the default array temperature values. Called by class constructor.
// +----------------------------------------------------------------------------
void CArcDevice::SetDefaultTemperatureValues()
{
	gTmpCtrl_DT670Coeff1			=	TMPCTRL_DT670_COEFF_1;
	gTmpCtrl_DT670Coeff2			=	TMPCTRL_DT670_COEFF_2;
	gTmpCtrl_SDAduOffset			=	TMPCTRL_SD_ADU_OFFSET;
	gTmpCtrl_SDAduPerVolt			=	TMPCTRL_SD_ADU_PER_VOLT;
	gTmpCtrl_HGAduOffset			=	TMPCTRL_HG_ADU_OFFSET;
	gTmpCtrl_HGAduPerVolt			=	TMPCTRL_HG_ADU_PER_VOLT;
	gTmpCtrl_SDNumberOfReads		=	TMPCTRL_SD_NUM_OF_READS;
	gTmpCtrl_SDVoltToleranceTrials	=	TMPCTRL_SD_VOLT_TOLERANCE_TRIALS;
	gTmpCtrl_SDVoltTolerance		=	TMPCTRL_SD_VOLT_TOLERANCE;
	gTmpCtrl_SDDegTolerance			=	TMPCTRL_SD_DEG_TOLERANCE;

	TmpCtrl_SD_2_12K.vu	   = TMPCTRL_SD_2_12K_VU;
	TmpCtrl_SD_2_12K.vl	   = TMPCTRL_SD_2_12K_VL;
	TmpCtrl_SD_2_12K.count = TMPCTRL_SD_2_12K_COUNT;

	for ( int i=0; i<TMPCTRL_SD_2_12K_COUNT; i++ )
	{
		TmpCtrl_SD_2_12K.coeff[ i ] = TMPCTRL_SD_2_12K_COEFF[ i ];
	}

	TmpCtrl_SD_12_24K.vu    = TMPCTRL_SD_12_24K_VU;
	TmpCtrl_SD_12_24K.vl    = TMPCTRL_SD_12_24K_VL;
	TmpCtrl_SD_12_24K.count = TMPCTRL_SD_12_24K_COUNT;

	for ( int i=0; i<TMPCTRL_SD_12_24K_COUNT; i++ )
	{
		TmpCtrl_SD_12_24K.coeff[ i ] = TMPCTRL_SD_12_24K_COEFF[ i ];
	}

	TmpCtrl_SD_24_100K.vu    = TMPCTRL_SD_24_100K_VU;
	TmpCtrl_SD_24_100K.vl    = TMPCTRL_SD_24_100K_VL;
	TmpCtrl_SD_24_100K.count = TMPCTRL_SD_24_100K_COUNT;

	for ( int i=0; i<TMPCTRL_SD_24_100K_COUNT; i++ )
	{
		TmpCtrl_SD_24_100K.coeff[ i ] = TMPCTRL_SD_24_100K_COEFF[ i ];
	}

	TmpCtrl_SD_100_475K.vu    = TMPCTRL_SD_100_475K_VU;
	TmpCtrl_SD_100_475K.vl    = TMPCTRL_SD_100_475K_VL;
	TmpCtrl_SD_100_475K.count = TMPCTRL_SD_100_475K_COUNT;

	for ( int i=0; i<TMPCTRL_SD_100_475K_COUNT; i++ )
	{
		TmpCtrl_SD_100_475K.coeff[ i ] = TMPCTRL_SD_100_475K_COEFF[ i ];
	}
}

// +----------------------------------------------------------------------------
// |  SetArrayTemperature
// +----------------------------------------------------------------------------
// |  Sets the array temperature to regulate around.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> gTempVal - The temperature value ( in Celcius ) to regulate
// |                     around.
// +----------------------------------------------------------------------------
void CArcDevice::SetArrayTemperature( double gTempVal )
{
	int dReply = 0;

	if ( IsReadout() )
	{
		CArcTools::ThrowException( "CArcDevice",
								   "SetArrayTemperature",
								   "Readout in progress!" );
	}

	if ( IsOpen() )
	{
		//
		// Check for SmallCam
		//
		bool bArc12 = IS_ARC12( GetControllerId() );

		//
		// Test for High Gain
		//
		bool bHighGain = ( Command( UTIL_ID, THG ) == 1 ? true : false );

		//
		// Calculate voltage/adu
		//
		double gVoltage = CalculateVoltage( gTempVal );

		int dAdu = ( int )VoltageToADU( gVoltage, bArc12, bHighGain );

		if ( bArc12 )
		{
			dReply = Command( TIM_ID, CDT, dAdu );
		}
		else
		{
			dReply = Command( UTIL_ID, WRM, ( Y_MEM | 0x1C ), dAdu );
		}

		if ( dReply != DON )
		{
			CArcTools::ThrowException(
							"CArcDevice",
							"SetArrayTemperature",
							"Failed to set array temperature. Command reply: 0x%X",
							 dReply );
		}
	}
	else
	{
		CArcTools::ThrowException( "CArcDevice",
								   "SetArrayTemperature",
								   "Not connected to any device!" );
	}
}

// +----------------------------------------------------------------------------
// |  GetArrayTemperature
// +----------------------------------------------------------------------------
// |  Returns the array temperature.
// |
// |  Throws std::runtime_error on error
// |
// |  Returns the average temperature value ( in Celcius ). The temperature is
// |  read gTmpCtrl_SDNumberOfReads times and averaged. Also, for a read to be
// |  included in the average the difference between the target temperature and
// |  the actual temperature must be less than the tolerance specified by
// |  gTmpCtrl_SDTolerance.
// +----------------------------------------------------------------------------
double CArcDevice::GetArrayTemperature()
{
	double gRetVal = 0.0;

	if ( IsReadout() )
	{
		CArcTools::ThrowException(
							"CArcDevice",
							"GetArrayTemperature",
							"Readout in progress, skipping temperature read!" );
	}

	if ( IsOpen() )
	{
		gRetVal = CalculateAverageTemperature();
	}
	else
	{
		CArcTools::ThrowException( "CArcDevice",
								   "GetArrayTemperature",
								   "Not connected to any device!" );
	}

	return gRetVal;
}

// +----------------------------------------------------------------------------
// |  GetArrayTemperatureDN
// +----------------------------------------------------------------------------
// |  Returns the digital number associated with the current array temperature
// |  value.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
double CArcDevice::GetArrayTemperatureDN()
{
	double gDn = 0.0;

	if ( IsReadout() )
	{
		CArcTools::ThrowException(
							"CArcDevice",
							"GetArrayTemperatureDN",
							"Readout in progress, skipping temperature read!" );
	}

	if ( IsOpen() )
	{
		if ( IS_ARC12( GetControllerId() ) )
		{
			gDn = ( double )Command( TIM_ID, RDC );
		}
		else
		{
			gDn = Command( UTIL_ID, RDM, ( Y_MEM | 0xC ) );
		}
	}
	else
	{
		CArcTools::ThrowException( "CArcDevice",
								   "GetArrayTemperatureDN",
								   "Not connected to any device!" );
	}

	return gDn;
}

// +----------------------------------------------------------------------------
// |  CalculateAverageTemperature
// +----------------------------------------------------------------------------
// |  Calculates the average array temperature.
// |
// |  Throws std::runtime_error on error
// |
// |  Returns the average temperature value ( in Celcius ). The temperature is
// |  read gTmpCtrl_SDNumberOfReads times and averaged. Also, for a read to be
// |  included in the average the difference between the target temperature and
// |  the actual temperature must be less than the tolerance specified by
// |  gTmpCtrl_SDDegTolerance ( default = 3.0 degrees celcius ).
// +----------------------------------------------------------------------------
double CArcDevice::CalculateAverageTemperature()
{
	shared_ptr<double> pgTemperature( new double[ gTmpCtrl_SDNumberOfReads ], ArrayDeleter<double>() );
	double gTemperatureSum		= 0.0;
	double gAvgTemperature		= 0.0;
	double gVoltage				= 0.0;

	int dNumberOfReads			= 0;
	int dNumberOfValidReads		= 0;
	int dAdu					= 0;

	vector<double> tempCoeffVector;

	if ( IsReadout() )
	{
		return gAvgTemperature;
	}

	//
	// Check the system id
	//
	bool bArc12 = IS_ARC12( GetControllerId() );

	//
	// Test for High Gain
	//
	bool bHighGain = ( Command( UTIL_ID, THG ) == 1 ? true : false );

	for ( int i=0; i<gTmpCtrl_SDNumberOfReads; i++ )
	{
		if ( IsReadout() )
		{
			break;
		}

		//
		// Read the temperature ADU from the controller
		// ------------------------------------------------
		if ( bArc12 )
		{
			dAdu = Command( TIM_ID, RDT );
		}
		else
		{
			dAdu = Command( UTIL_ID, RDM, ( Y_MEM | 0xC ) );
		}

		gVoltage = ADUToVoltage( dAdu, bArc12, bHighGain );

		if ( !ContainsError( dAdu ) )
		{
			//
			// Calculate the temperature from the given adu
			// ------------------------------------------------
			( pgTemperature.get() )[ i ] = CalculateTemperature( gVoltage );
			gTemperatureSum   += ( pgTemperature.get() )[ i ];
			dNumberOfReads++;

			//
			// Sleep for 2 milliseconds. The controller only
			// updates the temperature ADU value every 3 ms.
			// ------------------------------------------------
#ifdef WIN32
			Sleep( 2 );
#endif
		}
		else
		{
			CArcTools::ThrowException(
					"CArcDevice",
					"CalculateAverageTemperature",
					"Failed to read temperature from controller. Reply: 0x%X",
					 dAdu );
		}

		//
		// Don't average for SmallCam
		//
		if ( bArc12 )
		{
			break;
		}
	}

	//
	// Calculate the mean of the temperature values and average
	// only those that are within 3 degrees of the mean.
	//
	if ( bArc12 )
	{
		gAvgTemperature = ( pgTemperature.get() )[ 0 ];
	}
	else if ( dNumberOfReads > 0 )
	{
		double gMeanTemperature = ( gTemperatureSum / ( ( double )dNumberOfReads ) );

		gTemperatureSum = 0.0;

		for ( int i=0; i<dNumberOfReads; i++ )
		{
			if ( fabs( ( pgTemperature.get() )[ i ] - gMeanTemperature ) < gTmpCtrl_SDDegTolerance )
			{
				gTemperatureSum += ( pgTemperature.get() )[ i ];
				dNumberOfValidReads++;
			}
		}

		if ( dNumberOfValidReads > 0 )
		{
			gAvgTemperature = ( gTemperatureSum / ( ( double )dNumberOfValidReads ) );
		}
	}

	return gAvgTemperature;
}

// +----------------------------------------------------------------------------
// |  ADUToVoltage
// +----------------------------------------------------------------------------
// |  Calculates the voltage associated with the specified digital number (adu).
// |  There is a different calculation for SmallCam, which can be specified via
// |  the parameters.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> dAdu      - The digital number to use for the voltage calculation.
// |  <IN> -> bArc12    - 'true' to use the SmallCam calculation. Default=false.
// |  <IN> -> bHighGain - 'true' if High Gain is used. Default=false.
// +----------------------------------------------------------------------------
double CArcDevice::ADUToVoltage( int dAdu, bool bArc12, bool bHighGain )
{
	if ( bArc12 )
	{
		return ( gTmpCtrl_DT670Coeff1 +
				 gTmpCtrl_DT670Coeff2 * 
				 double( dAdu ) );
	}

	double aduOffset  = gTmpCtrl_SDAduOffset;
	double aduPerVolt = gTmpCtrl_SDAduPerVolt;

	if ( bHighGain )
	{
		aduOffset  = gTmpCtrl_HGAduOffset;
		aduPerVolt = gTmpCtrl_HGAduPerVolt;
	}

	return ( ( double( dAdu ) - aduOffset ) / aduPerVolt );
}

// +----------------------------------------------------------------------------
// |  VoltageToADU
// +----------------------------------------------------------------------------
// |  Calculates the digital number (adu) associated with the specified voltage.
// |  There is a different calculation for SmallCam, which can be specified via
// |  the parameters.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> gVoltage  - The digital number to use for the voltage calculation.
// |  <IN> -> bArc12    - 'true' to use the SmallCam calculation. Default=false.
// |  <IN> -> bHighGain - 'true' if High Gain is used. Default=false.
// +----------------------------------------------------------------------------
double CArcDevice::VoltageToADU( double gVoltage, bool bArc12, bool bHighGain )
{
	if ( bArc12 )
	{
		return ( ( gVoltage -
				   gTmpCtrl_DT670Coeff1 ) / 
				   gTmpCtrl_DT670Coeff2 );
	}

	double aduOffset  = gTmpCtrl_SDAduOffset;
	double aduPerVolt = gTmpCtrl_SDAduPerVolt;

	if ( bHighGain )
	{
		aduOffset  = gTmpCtrl_HGAduOffset;
		aduPerVolt = gTmpCtrl_HGAduPerVolt;
	}

	return ( gVoltage * aduPerVolt + aduOffset );
}

// +----------------------------------------------------------------------------
// |  CalculateTemperature
// +----------------------------------------------------------------------------
// |  Calculates the silicon diode temperature using a voltage ( converted 
// |  digital number read from the controller ) and a Chebychev polynomial 
// |  series.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> gVoltage  - The voltage to use for the temperature calculation.
// +----------------------------------------------------------------------------
double CArcDevice::CalculateTemperature( double gVoltage )
{
	double gTemperature		= -273.15;
	double gX				= 0.0;
	double gTempVU			= 0.0;
	double gTempVL			= 0.0;
	vector<double> tcVector;
	vector<double> tempCoeffVector;

	if ( gVoltage <= 0.0 )
	{
		CArcTools::ThrowException(
						"CArcDevice",
						"CalculateTemperature",
						"Voltage ( %f V ) out of range ( 0.4V - 1.0V )",
						 gVoltage );
	}

	// -------------------------------------
	// TEMP  = 2.0 to 12.0 K
	// VOLTS = 1.294390 to 1.680000
	// -------------------------------------
	if ( gVoltage >= TmpCtrl_SD_2_12K.vl && gVoltage <= TmpCtrl_SD_2_12K.vu )
	{
		gTempVU = TmpCtrl_SD_2_12K.vu;
		gTempVL = TmpCtrl_SD_2_12K.vl;

		for ( int t=0; t< TmpCtrl_SD_2_12K.count; t++ )
		{
			tempCoeffVector.push_back( TmpCtrl_SD_2_12K.coeff[ t ]  );
		}
	}

	// -------------------------------------
	// TEMP  = 12.0 to 24.5 K
	// VOLTS = 1.11230 to 1.38373
	// -------------------------------------
	else if ( gVoltage >= TmpCtrl_SD_12_24K.vl && gVoltage <= TmpCtrl_SD_12_24K.vu )
	{
		gTempVU = TmpCtrl_SD_12_24K.vu;
		gTempVL = TmpCtrl_SD_12_24K.vl;

		for ( int t=0; t< TmpCtrl_SD_12_24K.count; t++ )
		{
			tempCoeffVector.push_back( TmpCtrl_SD_12_24K.coeff[ t ]  );
		}
	}

	// -------------------------------------
	// TEMP  = 24.5 to 100.0 K
	// VOLTS = 0.909416 to 1.122751
	// -------------------------------------
	else if ( gVoltage >= TmpCtrl_SD_24_100K.vl && gVoltage <= TmpCtrl_SD_24_100K.vu )
	{
		gTempVU = TmpCtrl_SD_24_100K.vu;
		gTempVL = TmpCtrl_SD_24_100K.vl;

		for ( int t=0; t< TmpCtrl_SD_24_100K.count; t++ )
		{
			tempCoeffVector.push_back( TmpCtrl_SD_24_100K.coeff[ t ]  );
		}
	}

	// -------------------------------------
	// TEMP  = 100.0 to 475.0 K
	// VOLTS = 0.07000 to 0.99799
	// -------------------------------------
	else if ( gVoltage <= TmpCtrl_SD_100_475K.vu )
	{
		gTempVU = TmpCtrl_SD_100_475K.vu;
		gTempVL = TmpCtrl_SD_100_475K.vl;

		for ( int t=0; t< TmpCtrl_SD_100_475K.count; t++ )
		{
			tempCoeffVector.push_back( TmpCtrl_SD_100_475K.coeff[ t ]  );
		}
	}

	// -------------------------------------
	//  ERROR - No Coefficients Exist
	// -------------------------------------
	else
	{
		CArcTools::ThrowException(
						"CArcDevice",
						"CalculateTemperature",
						"Coefficients for the voltage ( %f V ) don't exist!",
						 gVoltage );
	}

	if ( tempCoeffVector.empty() || ( tempCoeffVector.size() > 3 ) || tcVector.empty() )
	{
		// Calculate dimensionless variable for the Chebychev series.
		gX = ( ( gVoltage - gTempVL ) - ( gTempVU - gVoltage ) );
		gX = gX / ( gTempVU - gTempVL );

		tcVector.push_back( 1 );
		tcVector.push_back( gX );

		gTemperature += tempCoeffVector[ 0 ] + ( tempCoeffVector[ 1 ] * gX );

		for ( int i=2; i<( ( int )tempCoeffVector.size() ); i++ )
		{
  			tcVector.push_back( 2.0 * gX * tcVector[ i - 1 ] - tcVector[ i - 2 ] );
			gTemperature += tempCoeffVector[ i ] * tcVector[ i ];
		}
	}

	return gTemperature;
}

// +----------------------------------------------------------------------------
// |  CalculateVoltage
// +----------------------------------------------------------------------------
// |  Calculates the voltage using the specified temperature. The voltage is
// |  determined using the number of trials specified by 
// |  gTmpCtrl_SDVoltToleranceTrials.  Also, the difference between the target
// |  temperature and the actual must be less than the tolerance specified by
// |  gTmpCtrl_SDVoltTolerance.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> gTemperature - The temperature to use for the calculation.
// +----------------------------------------------------------------------------
double CArcDevice::CalculateVoltage( double gTemperature )
{
	bool   bTolerance	= true;
	int    dTrials		= 0;
	double gVmid		= 0.0;
	double gTempVU		= 0.0;
	double gTempVL		= 0.0;
	double gTargetTemp	= -273.15;

	// -------------------------------------
	// TMP = 0.0 to 12.0 K
	// -------------------------------------
	if ( gTemperature < -261.15 )
	{
		gTempVU = TmpCtrl_SD_2_12K.vu;
		gTempVL = TmpCtrl_SD_2_12K.vl;
	}

	// -------------------------------------
	// TMP = 12.0 to 24.5 K
	// -------------------------------------
	else if ( gTemperature >= -261.15 && gTemperature < -248.65 )
	{
		gTempVU = TmpCtrl_SD_12_24K.vu;
		gTempVL = TmpCtrl_SD_12_24K.vl;
	}

	// -------------------------------------
	// TMP = 24.5 to 100.0 K
	// -------------------------------------
	else if ( gTemperature >= -248.65 && gTemperature < -173.15 )
	{
		gTempVU = TmpCtrl_SD_24_100K.vu;
		gTempVL = TmpCtrl_SD_24_100K.vl;
	}

	// -------------------------------------
	// TMP = 100.0 to 475.0 K
	// -------------------------------------
	else if ( gTemperature >= -173.15 )
	{
		gTempVU = TmpCtrl_SD_100_475K.vu;
		gTempVL = TmpCtrl_SD_100_475K.vl;
	}

	gVmid = ( gTempVL + gTempVU ) * 0.5;

	while ( bTolerance && ( dTrials < gTmpCtrl_SDVoltToleranceTrials ) )
	{
		gTargetTemp = CalculateTemperature( gVmid );

		if ( fabs( gTargetTemp - gTemperature ) <= gTmpCtrl_SDVoltTolerance )
			bTolerance = false;

		if ( bTolerance )
		{
			if ( gTargetTemp < gTemperature )
			{
        		gTempVU	= gVmid;
			}

			else if ( gTargetTemp > gTemperature )
			{
				gTempVL	= gVmid;
			}

       		gVmid	= ( gTempVL + gTempVU ) * 0.5;
    	}
    	dTrials++;
	}

	return gVmid;
}
