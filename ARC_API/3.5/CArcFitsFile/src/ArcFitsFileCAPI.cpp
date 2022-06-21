// ArcFitsFileCAPI.cpp : Defines the exported functions for the DLL application.
//
#ifdef WIN32
#pragma warning( disable: 4251 )
#endif

#include <stdexcept>
#include <string>

#include "DllMain.h"
#include "ArcFitsFileCAPI.h"
#include "CArcFitsFile.h"

using namespace std;
using namespace arc::fits;


// +------------------------------------------------------------------------------------+
// | Deinterlace constants
// +------------------------------------------------------------------------------------+
const int FITS_READMODE			= CArcFitsFile::READMODE;
const int FITS_READWRITEMODE	= CArcFitsFile::READWRITEMODE;

const int FITS_BPP16			= CArcFitsFile::BPP16;
const int FITS_BPP32			= CArcFitsFile::BPP32;

const int FITS_NAXES_COL		= CArcFitsFile::NAXES_COL;
const int FITS_NAXES_ROW		= CArcFitsFile::NAXES_ROW;
const int FITS_NAXES_NOF		= CArcFitsFile::NAXES_NOF;
const int FITS_NAXES_SIZE		= CArcFitsFile::NAXES_SIZE;

const int FITS_STRING_KEY		= CArcFitsFile::FITS_STRING_KEY;
const int FITS_INTEGER_KEY		= CArcFitsFile::FITS_INTEGER_KEY;
const int FITS_DOUBLE_KEY		= CArcFitsFile::FITS_DOUBLE_KEY;
const int FITS_LOGICAL_KEY		= CArcFitsFile::FITS_LOGICAL_KEY;
const int FITS_COMMENT_KEY		= CArcFitsFile::FITS_COMMENT_KEY;
const int FITS_HISTORY_KEY		= CArcFitsFile::FITS_HISTORY_KEY;
const int FITS_DATE_KEY			= CArcFitsFile::FITS_DATE_KEY;


// +------------------------------------------------------------------------------------+
// | Globals                                                                            |
// +------------------------------------------------------------------------------------+
static auto_ptr<CArcFitsFile>		g_pCFitsFile( ( CArcFitsFile * )0 );
static char							g_szErrMsg[ ARC_ERROR_MSG_SIZE ];

static char**	g_pHeader		= NULL;
static int		g_dHeaderCount	= 0;


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
// |  Creates a FITS file the will contain a single image.                     |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN>  -> pszFilename   - The to create, including path.                  |
// |  <IN>  -> rows          - The number of rows in the image.                |
// |  <IN>  -> cols          - The number of columns in the image.             |
// |  <IN>  -> dBitsPerPixel - Can be 16 ( BPP16 ) or 32 ( BPP32 ).            |
// |  <IN>  -> is3D          - True to create a fits data cube.                |
// |  <OUT> -> pStatus       - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_Create( const char* pszFilename, int dRows,
										 int dCols, int dBpp, int dIs3D,
										 int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		g_pCFitsFile.reset(
						new CArcFitsFile( pszFilename,
										  dRows,
										  dCols,
										  dBpp,
										  ( dIs3D > 0 ? true : false ) ) );
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
// |  Opens an existing FITS file for reading. May contain a single image or   |
// |  a FITS data cube.                                                        |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN>  -> pszFilename - The file to open, including path.                 |
// |  <IN>  -> dMode       - Read/Write mode. Can be READMODE or READWRITEMODE |
// |  <OUT> -> pStatus     - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR   |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_Open( const char* pszFilename, int dMode, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		g_pCFitsFile.reset( new CArcFitsFile( pszFilename, dMode ) );
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
// |  Closes the FITS file.                                                    |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_Close()
{
	g_pCFitsFile.reset( ( CArcFitsFile * )0 );
}


// +---------------------------------------------------------------------------+
// |  GetFilename                                                              |
// +---------------------------------------------------------------------------+
// |  Returns the filename associated with this CArcFitsFile object.           |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API const char*	ArcFitsFile_GetFilename( int* pStatus )
{
	string sFilename;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_GetFilename", g_pCFitsFile )

		sFilename = g_pCFitsFile.get()->GetFilename();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return sFilename.c_str();
}


// +---------------------------------------------------------------------------+
// | GetHeader                                                                 |
// +---------------------------------------------------------------------------+
// |  Returns the FITS header as an array of strings.                          |
// |                                                                           |
// |  IMPORTANT NOTE: The user MUST free the header data by calling the        |
// |                  ArcFitsFile_FreeHeader() function.                       |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <OUT> -> pKeyCount - The number of keys in the returned header array.    |
// |  <OUT> -> pStatus     - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR   |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API const char** ArcFitsFile_GetHeader( int* pKeyCount, int* pStatus )
{
	string* pHeader = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_GetHeader", g_pCFitsFile )

		pHeader = g_pCFitsFile.get()->GetHeader( pKeyCount );

		if ( pHeader != NULL && *pKeyCount > 0 )
		{
			g_dHeaderCount = *pKeyCount;

			g_pHeader = new char*[ g_dHeaderCount ];

			for ( int i=0; i<g_dHeaderCount; i++ )
			{
				g_pHeader[ i ] = new char[ 100 ];

				ArcSprintf( &g_pHeader[ i ][ 0 ],
							100,
							"%s",
							pHeader[ i ].c_str() );
			}
		}
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return const_cast<const char **>( g_pHeader );
}


// +---------------------------------------------------------------------------+
// | FreeHeader                                                                |
// +---------------------------------------------------------------------------+
// |  Frees the FITS header as returned by ArcFitsFile_GetHeader().            |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_FreeHeader()
{
	if ( g_pHeader != NULL )
	{
		for ( int i=0; i<g_dHeaderCount; i++ )
		{
			delete [] g_pHeader[ i ];
		}

		delete [] g_pHeader;
	}

	g_dHeaderCount = 0;
}


// +---------------------------------------------------------------------------+
// |  WriteKeyword                                                             |
// +---------------------------------------------------------------------------+
// |  Writes a FITS keyword to an existing FITS file.  The keyword must be     |
// |  valid or an exception will be thrown. For list of valid FITS keywords,   |
// |  see:                                                                     |
// |                                                                           |
// |  http://heasarc.gsfc.nasa.gov/docs/fcg/standard_dict.html                 |
// |  http://archive.stsci.edu/fits/fits_standard/node38.html# \               |
// |  SECTION00940000000000000000                                              |
// |                                                                           |
// |  'HIERARCH' keyword NOTE: This text will be prefixed to any keyword by    |
// |                           the cfitsio library if the keyword is greater   |
// |                           than 8 characters, which is the standard FITS   |
// |                           keyword length. See the link below for details: |
// |   http://heasarc.gsfc.nasa.gov/docs/software/fitsio/c/f_user/node28.html  |
// |                                                                           |
// |   HIERARCH examples:                                                      |
// |   -----------------                                                       |
// |   HIERARCH LongKeyword = 47.5 / Keyword has > 8 characters & mixed case   |
// |   HIERARCH XTE$TEMP = 98.6 / Keyword contains the '$' character           |
// |   HIERARCH Earth is a star = F / Keyword contains embedded spaces         |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN>  -> pszKey    - The header keyword. Can be NULL. Ex: SIMPLE         |
// |  <IN>  -> pKeyVal   - The value associated with the key. Ex: T            |
// |  <IN>  -> dValType  - The keyword type, as defined in CArcFitsFile.h      |
// |  <IN>  -> pszComment - The comment to attach to the keyword.  Can be NULL |
// |  <OUT> -> pStatus   - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR     |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_WriteKeyword( char* pszKey, void* pKeyVal,
											   int dValType, char* pszComment,
											   int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_WriteKeyword", g_pCFitsFile )

		g_pCFitsFile.get()->WriteKeyword( pszKey, pKeyVal, dValType, pszComment );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  UpdateKeyword                                                            |
// +---------------------------------------------------------------------------+
// |  Updates an existing FITS keyword to an existing FITS file.  The keyword  |
// |  must be valid or an exception will be thrown. For list of valid FITS     |
// |  keywords, see:                                                           |
// |                                                                           |
// |  http://heasarc.gsfc.nasa.gov/docs/fcg/standard_dict.html                 |
// |  http://archive.stsci.edu/fits/fits_standard/node38.html# \               |
// |  SECTION00940000000000000000                                              |
// |                                                                           |
// |  'HIERARCH' keyword NOTE: This text will be prefixed to any keyword by    |
// |                           the cfitsio library if the keyword is greater   |
// |                           than 8 characters, which is the standard FITS   |
// |                           keyword length. See the link below for details: |
// |   http://heasarc.gsfc.nasa.gov/docs/software/fitsio/c/f_user/node28.html  |
// |                                                                           |
// |   HIERARCH examples:                                                      |
// |   -----------------                                                       |
// |   HIERARCH LongKeyword = 47.5 / Keyword has > 8 characters & mixed case   |
// |   HIERARCH XTE$TEMP = 98.6 / Keyword contains the '$' character           |
// |   HIERARCH Earth is a star = F / Keyword contains embedded spaces         |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN>  -> key      - The header keyword. Ex: SIMPLE                       |
// |  <IN>  -> keyVal   - The value associated with the key. Ex: T             |
// |  <IN>  -> valType  - The keyword value type, as defined in CArcFitsFile.h |
// |  <IN>  -> comment  - The comment to attach to the keyword.                |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_UpdateKeyword( char* pszKey, void* pKeyVal,
											    int dValType, char* pszComment,
												int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_UpdateKeyword", g_pCFitsFile )

		g_pCFitsFile.get()->UpdateKeyword( pszKey, pKeyVal, dValType, pszComment );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------+
// |  GetParameters                                                             |
// +----------------------------------------------------------------------------+
// |  Reads the lNAxes, dNAxis and bits-per-pixel header values from a FITS     |
// |  file.                                                                     |
// |                                                                            |
// |  Throws std::runtime_error on error.                                       |
// |                                                                            |
// |  <OUT> -> lNAxes        - MUST be a pointer to:  int lNAxes[ 3 ]. Index 0  |
// |                           will have column size, 1 will have row size, and |
// |                           2 will have number of frames if file is a data   |
// |                           cube. Or use NAXES_COL, NAXES_ROW, NAXES_NOF.    |
// |  <OUT> -> dNAxis        - Optional pointer to int for NAXIS keyword value  |
// |  <OUT> -> dBitsPerPixel - Optional pointer to int for BITPIX keyword value |
// |  <OUT> -> pStatus       - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR  |
// +----------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_GetParameters( long* pNaxes, int* pNaxis,
											    int* pBitsPerPixel, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_GetParameters", g_pCFitsFile )

		g_pCFitsFile.get()->GetParameters( pNaxes, pNaxis, pBitsPerPixel );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  GetNumberOfFrames                                                        |
// +---------------------------------------------------------------------------+
// |  Convenience method to get the column count ( NAXIS3 keyword ).  This     |
// |  returns the CArcFitsFile::NAXES_NOF element in the pNaxes array to the   |
// |  method GetParameters().  To get all parameters ( rows, cols,             |
// |  number-of-frames, naxis value and bits-per-pixel ) at one time ... call  |
// |  GetParameters() instead.                                                 |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API long ArcFitsFile_GetNumberOfFrames( int* pStatus )
{
	long lFrameCount = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_GetNumberOfFrames", g_pCFitsFile )

		lFrameCount = g_pCFitsFile.get()->GetNumberOfFrames();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return lFrameCount;
}


// +---------------------------------------------------------------------------+
// |  GetRows                                                                  |
// +---------------------------------------------------------------------------+
// |  Convenience method to get the column count ( NAXIS2 keyword ).  This     |
// |  returns the CArcFitsFile::NAXES_ROW element in the pNaxes array to the   |
// |  method GetParameters().  To get all parameters ( rows, cols,             |
// |  number-of-frames, naxis value and bits-per-pixel ) at one time ... call  |
// |  GetParameters() instead.                                                 |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API long ArcFitsFile_GetRows( int* pStatus )
{
	long lRows = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_GetRows", g_pCFitsFile )

		lRows = g_pCFitsFile.get()->GetRows();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return lRows;
}


// +---------------------------------------------------------------------------+
// |  GetCols                                                                  |
// +---------------------------------------------------------------------------+
// |  Convenience method to get the column count ( NAXIS1 keyword ).  This     |
// |  returns the CArcFitsFile::NAXES_COL element in the pNaxes array to the   |
// |  method GetParameters().  To get all parameters ( rows, cols,             |
// |  number-of-frames, naxis value and bits-per-pixel ) at one time ... call  |
// |  GetParameters() instead.                                                 |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API long ArcFitsFile_GetCols( int* pStatus )
{
	long lCols = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_GetCols", g_pCFitsFile )

		lCols = g_pCFitsFile.get()->GetCols();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return lCols;
}


// +---------------------------------------------------------------------------+
// |  GetNAxis                                                                 |
// +---------------------------------------------------------------------------+
// |  Convenience method to get the NAXIS keyword.  This returns the pNaxis    |
// |  paramter to the method GetParameters().  To get all parameters ( rows,   |
// |  cols, number-of-frames, naxis value and bits-per-pixel ) at one time ... |
// |  call GetParameters() instead.                                            |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API int	ArcFitsFile_GetNAxis( int* pStatus )
{
	int dNAxis = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_GetNAxis", g_pCFitsFile )

		dNAxis = g_pCFitsFile.get()->GetNAxis();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dNAxis;
}


// +---------------------------------------------------------------------------+
// |  GetBpp                                                                   |
// +---------------------------------------------------------------------------+
// |  Convenience method to get the bits-per-pixel ( BITPIX keyword ).  This   |
// |  returns the pBitsPerPixel paramter to the method GetParameters().  To    |
// |  get all parameters ( rows, cols, number-of-frames, naxis value and       |
// |  bits-per-pixel ) at one time ... call GetParameters() instead.           |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API int	ArcFitsFile_GetBpp( int* pStatus )
{
	int dBpp = 0;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_GetBpp", g_pCFitsFile )

		dBpp = g_pCFitsFile.get()->GetBpp();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return dBpp;
}


// +---------------------------------------------------------------------------+
// |  GenerateTestData                                                         |
// +---------------------------------------------------------------------------+
// |  Writes test data to the file. The data's in the form 0, 1, 2 ... etc.    |
// |  The purpose of the method is purely for testing when a FITS image is     |
// |  otherwise unavailable.                                                   |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_GenerateTestData( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_GenerateTestData", g_pCFitsFile )

		g_pCFitsFile.get()->GenerateTestData();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  ReOpen                                                                   |
// +---------------------------------------------------------------------------+
// |  Closes and re-opens the current FITS file.                               |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_ReOpen( int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_ReOpen", g_pCFitsFile )

		g_pCFitsFile.get()->ReOpen();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  Resize ( Single Image )                                                  |
// +---------------------------------------------------------------------------+
// |  Resizes a single image FITS file by modifying the NAXES keyword and      |
// |  increasing the image data portion of the file.                           |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> dRows - The number of rows the new FITS file will have.          |
// |  <IN> -> dCols - The number of cols the new FITS file will have.          |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_Resize( int dRows, int dCols, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_Resize", g_pCFitsFile )

		g_pCFitsFile.get()->Resize( dRows, dCols );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  Write ( Single Image )                                                   |
// +---------------------------------------------------------------------------+
// |  Writes to a FITS file the will contain a single image.                   |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> data     - Pointer to image data to write.                       |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_Write( void* pData, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_Write", g_pCFitsFile )

		g_pCFitsFile.get()->Write( pData );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  Write ( Single Image )                                                   |
// +---------------------------------------------------------------------------+
// |  Writes a specified number of bytes from the provided buffer to a FITS    |
// |  that contains a single image. The start position of the data within the  |
// |  FITS file image can be specified.                                        |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> pData         - Pointer to the data to write.                    |
// |  <IN> -> uBytesToWrite - The number of bytes to write.                    |
// |  <IN> -> dFPixl        - The start pixel within the FITS file image. This |
// |                          parameter is optional. If -1, then the next      |
// |                          write position will be at zero. If fPixel >= 0,  |
// |                          then data will be written there.                 |
// |  <OUT> -> pStatus      - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR  |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_WriteToOffset( void* pData, unsigned int uBytesToWrite,
												 int fPixel, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_WriteToOffset", g_pCFitsFile )

		g_pCFitsFile.get()->Write( pData, uBytesToWrite, fPixel );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  WriteSubImage ( Single Image )                                           |
// +---------------------------------------------------------------------------+
// |  Writes a sub-image to a FITS file.                                       |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> llrow        - The lower left row of the sub-image.              |
// |  <IN> -> llcol        - The lower left column of the sub-image.           |
// |  <IN> -> urrow        - The upper right row of the sub-image.             |
// |  <IN> -> urcol        - The upper right column of the sub-image.          |
// |  <IN> -> data         - The pixel data.                                   |
// |  <OUT> -> pStatus     - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR   |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_WriteSubImage( void* pData, int llrow, int llcol,
											    int urrow, int urcol, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_WriteSubImage", g_pCFitsFile )

		g_pCFitsFile.get()->WriteSubImage( pData, llrow, llcol, urrow, urcol );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  ReadSubImage ( Single Image )                                            |
// +---------------------------------------------------------------------------+
// |  Reads a sub-image from a FITS file.                                      |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> llrow        - The lower left row of the sub-image.              |
// |  <IN> -> llcol        - The lower left column of the sub-image.           |
// |  <IN> -> urrow        - The upper right row of the sub-image.             |
// |  <IN> -> urcol        - The upper right column of the sub-image.          |
// |  <OUT> -> pStatus     - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR   |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void* ArcFitsFile_ReadSubImage( int llrow, int llcol, int urrow, int urcol, int* pStatus )
{
	void* pBuffer = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_ReadSubImage", g_pCFitsFile )

		pBuffer = g_pCFitsFile.get()->ReadSubImage( llrow, llcol, urrow, urcol );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pBuffer;
}


// +---------------------------------------------------------------------------+
// |  Read ( Single Image )                                                    |
// +---------------------------------------------------------------------------+
// |  Reads the pixels from a FITS file containing a single image.             |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  Returns a pointer to the image data. The caller of this method is NOT    |
// |  responsible for freeing the memory allocated to the image data.          |
// |                                                                           |
// |  <OUT> -> pStatus     - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR   |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void* ArcFitsFile_Read( int* pStatus )
{
	void* pBuffer = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_Read", g_pCFitsFile )

		pBuffer = g_pCFitsFile.get()->Read();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pBuffer;
}


// +---------------------------------------------------------------------------+
// |  Write3D ( Data Cube )                                                    |
// +---------------------------------------------------------------------------+
// |  Add an image to the end of a FITS data cube.                             |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN>  -> pData   - A pointer to the image data.                          |
// |  <OUT> -> pStatus - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR       |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_Write3D( void* pData, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_Write3D", g_pCFitsFile )

		g_pCFitsFile.get()->Write3D( pData );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  ReWrite3D ( Data Cube )                                                  |
// +---------------------------------------------------------------------------+
// |  Re-writes an existing image in a FITS data cube. The image data MUST     |
// |  match in size to the exising images within the data cube. The image      |
// |  size is NOT checked for by this method.                                  |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN>  -> data         - A pointer to the image data.                     |
// |  <IN>  -> dImageNumber - The number of the data cube image to replace.    |
// |  <OUT> -> pStatus      - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR  |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_ReWrite3D( void* pData, int dImageNumber, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_ReWrite3D", g_pCFitsFile )

		g_pCFitsFile.get()->ReWrite3D( pData, dImageNumber );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +---------------------------------------------------------------------------+
// |  Read3D ( Data Cube )                                                     |
// +---------------------------------------------------------------------------+
// |  Reads an image from a FITS data cube.                                    |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN>  -> dImageNumber - The image number to read.                        |
// |  <OUT> -> void *       - A pointer to the image data.                     |
// |  <OUT> -> pStatus      - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR  |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API void* ArcFitsFile_Read3D( int dImageNumber, int* pStatus )
{
	void* pBuffer = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_Read3D", g_pCFitsFile )

		pBuffer = g_pCFitsFile.get()->Read3D( dImageNumber );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pBuffer;
}


// +---------------------------------------------------------------------------+
// |  GetBaseFile                                                              |
// +---------------------------------------------------------------------------+
// |  Returns the underlying cfitsio file pointer.  INTERNAL USE ONLY.         |
// +---------------------------------------------------------------------------+
DLLFITSFILE_API fitsfile* ArcFitsFile_GetBaseFile( int* pStatus )
{
	fitsfile* pFitsFile = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcFitsFile_GetBaseFile", g_pCFitsFile )

		pFitsFile = g_pCFitsFile.get()->GetBaseFile();
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pFitsFile;
}


// +--------------------------------------------------------------------------------------------
// | GetLastError
// +--------------------------------------------------------------------------------------------
// | Returns the last error message reported.
// +--------------------------------------------------------------------------------------------
DLLFITSFILE_API const char* ArcFitsFile_GetLastError()
{
	return const_cast<const char *>( g_szErrMsg );
}
