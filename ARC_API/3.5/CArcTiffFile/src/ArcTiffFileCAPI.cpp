// ArcTiffFileCAPI.cpp : Defines the exported functions for the DLL application.
//
#ifdef WIN32
#pragma warning( disable: 4251 )
#endif

#include <stdexcept>
#include <string>

#include "DllMain.h"
#include "ArcTiffFileCAPI.h"
#include "CArcTiffFile.h"

using namespace std;
using namespace arc::tiff;


// +------------------------------------------------------------------------------------+
// | Deinterlace constants
// +------------------------------------------------------------------------------------+
const int TIFF_READMODE						= CArcTiffFile::READMODE;
const int TIFF_WRITEMODE					= CArcTiffFile::WRITEMODE;

const int TIFF_BPP8							= CArcTiffFile::BPP8;
const int TIFF_BPP16						= CArcTiffFile::BPP16;

const int TIFF_RGB_SAMPLES_PER_PIXEL		= CArcTiffFile::RGB_SAMPLES_PER_PIXEL;
const int TIFF_RGBA_SAMPLES_PER_PIXEL		= CArcTiffFile::RGBA_SAMPLES_PER_PIXEL;


// +------------------------------------------------------------------------------------+
// | Globals                                                                            |
// +------------------------------------------------------------------------------------+
static auto_ptr<CArcTiffFile>	g_pCTiffFile( ( CArcTiffFile * )0 );
static char						g_szErrMsg[ ARC_ERROR_MSG_SIZE ];


// +------------------------------------------------------------------------------------+
// | Define system dependent macros                                                     |
// +------------------------------------------------------------------------------------+
#ifndef ARC_SPRINTF
#define ARC_SPRINTF

	#ifdef WIN32
		#define ArcSprintf( dst, size, fmt, msg )	sprintf_s( dst, size, fmt, msg )
	#else
		#define ArcSprintf( dst, size, fmt, msg )	sprintf( dst, fmt, msg )
	#endif

#endif


// +------------------------------------------------------------------------------------+
// | Verify class pointer macro                                                         |
// +------------------------------------------------------------------------------------+
#define VERIFY_CLASS_PTR( fnc, ptr )												\
						if ( ptr.get() == NULL )									\
						{															\
							throw runtime_error( string( "( " ) + fnc +				\
							string( " ): Invalid class pointer!" ) );				\
						}


// +---------------------------------------------------------------------------+
// |  Create                                                                   |
// +---------------------------------------------------------------------------+
// |  Creates a new TIFF file for writing.                                     |
// |                                                                           |
// |  Throws std::runtime_error on parameter or libtiff library error.         |
// |                                                                           |
// |  <IN>  -> pszFilename - The file to open, including path.                 |
// |  <OUT> -> pStatus     - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR   |
// +---------------------------------------------------------------------------+
DLLTIFFFILE_API void ArcTiffFile_Create( const char* pszFilename, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		g_pCTiffFile.reset(
					new CArcTiffFile( pszFilename, CArcTiffFile::WRITEMODE ) );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  Open                                                                     |
// +---------------------------------------------------------------------------+
// |  Opens an existing TIFF file depending based on the mode parameter.       |
// |                                                                           |
// |  Throws std::runtime_error on parameter or libtiff library error.         |
// |                                                                           |
// |  <IN> -> pszFilename - The file to open, including path.                  |
// |  <IN> -> dMode       - Read/Write mode, default = CArcTiffFile::WRITEMODE |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLTIFFFILE_API void ArcTiffFile_Open( const char* pszFilename, int dMode, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		g_pCTiffFile.reset( new CArcTiffFile( pszFilename, dMode ) );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  Close                                                                    |
// +---------------------------------------------------------------------------+
// |  Destroys the class. Deallocates any data buffers. Closes any open TIFF   |
// |  pointers.                                                                |
// +---------------------------------------------------------------------------+
DLLTIFFFILE_API void ArcTiffFile_Close()
{
	try
	{
		g_pCTiffFile.reset( ( CArcTiffFile * )0 );
	}
	catch ( exception& e )
	{
		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  GetRows                                                                  |
// +---------------------------------------------------------------------------+
// |  Returns the row dimension of the TIFF file currently open.               |
// |                                                                           |
// |  <RET> -> Number of rows in TIFF image.                                   |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLTIFFFILE_API int ArcTiffFile_GetRows( int* pStatus )
{
	int dRows = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcTiffFile_GetRows", g_pCTiffFile )

		dRows = g_pCTiffFile.get()->GetRows();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dRows;
}


// +---------------------------------------------------------------------------+
// |  GetCols                                                                  |
// +---------------------------------------------------------------------------+
// |  Returns the column dimension of the TIFF file currently open.            |
// |                                                                           |
// |  <RET> -> Number of columns in TIFF image.                                |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLTIFFFILE_API int ArcTiffFile_GetCols( int* pStatus )
{
	int dCols = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcTiffFile_GetCols", g_pCTiffFile )

		dCols = g_pCTiffFile.get()->GetCols();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dCols;
}


// +---------------------------------------------------------------------------+
// |  GetBpp                                                                   |
// +---------------------------------------------------------------------------+
// |  Returns the bits-per-pixel ( bits-per-sample ) of the TIFF file          |
// |  currently open.                                                          |
// |                                                                           |
// |  <RET> -> bits-per-pixel ( bits-per-sample ) in TIFF image.               |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLTIFFFILE_API int ArcTiffFile_GetBpp( int* pStatus )
{
	int dBpp = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcTiffFile_GetBpp", g_pCTiffFile )

		dBpp = g_pCTiffFile.get()->GetBpp();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dBpp;
}


// +---------------------------------------------------------------------------+
// |  Write                                                                    |
// +---------------------------------------------------------------------------+
// |  Writes 16-bit grayscale image to a 16 or 8-bit TIFF file. The output to  |
// |  the file is controlled by the bits-per-pixel ( dBpp ) parameter.         |
// |                                                                           |
// |  Throws std::runtime_error on parameter error or libtiff library error.   |
// |                                                                           |
// |  <IN>  -> pU16Buf - Pointer to 16-bit image data.                         |
// |  <IN>  -> dRows   - Number of rows in image data.                         |
// |  <IN>  -> dCols   - Number of cols in image data.                         |
// |  <IN>  -> dBpp    - The bits-per-pixel(/sample), default = 16.            |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLTIFFFILE_API void ArcTiffFile_Write( unsigned short* pU16Buf, int dRows,
									    int dCols, int dBpp, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcTiffFile_Write", g_pCTiffFile )

		g_pCTiffFile.get()->Write( pU16Buf, dRows, dCols, dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  Read                                                                     |
// +---------------------------------------------------------------------------+
// |  Reads from a TIFF file.                                                  |
// |                                                                           |
// |  Throws std::runtime_error on parameter or libtiff library error.         |
// |                                                                           |
// |  <RET> -> Pointer to the image data.                                      |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLTIFFFILE_API void* ArcTiffFile_Read( int* pStatus )
{
	void* pBuffer = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcTiffFile_Read", g_pCTiffFile )

		pBuffer = g_pCTiffFile.get()->Read();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pBuffer;
}


// +----------------------------------------------------------------------------
// | GetLastError
// +----------------------------------------------------------------------------
// | Returns the last error message reported.
// +----------------------------------------------------------------------------
DLLTIFFFILE_API const char* ArcTiffFile_GetLastError()
{
	return const_cast<const char *>( g_szErrMsg );
}
