// +---------------------------------------------------------------------------+
// |  CArcFitsFile.cpp                                                         |
// +---------------------------------------------------------------------------+
// |  Defines the exported functions for the CArcFitsFile DLL.  Wraps the      |
// |  cfitsio library for convenience and for use with Owl.                    |
// |                                                                           |
// |  Scott Streit                                                             |
// |  Astronomical Research Cameras, Inc.                                      |
// +---------------------------------------------------------------------------+
#include <typeinfo>
#include <sstream>
#include <cmath>
#include "CArcFitsFile.h"

using namespace std;
using namespace arc::fits;


// +---------------------------------------------------------------------------+
// | ArrayDeleter                                                              |
// +---------------------------------------------------------------------------+
// | Called by std::shared_ptr to delete the temporary image buffer.           |
// | This method should NEVER be called directly by the user.                  |
// +---------------------------------------------------------------------------+
template<typename T> void CArcFitsFile::ArrayDeleter( T* p )
{
	if ( p != NULL )
	{
		delete [] p;
	}
}

// +---------------------------------------------------------------------------+
// |  Copy constructor                                                         |
// +---------------------------------------------------------------------------+
// |  Copies an existing CArcFitsFile, headers only.  The image data is not    |
// |  copied.                                                                  |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> rAnotherFitsFile - The file to copy.                             |
// +---------------------------------------------------------------------------+
CArcFitsFile::CArcFitsFile( CArcFitsFile& rAnotherFitsFile, const char* pszNewFileName )
{
    int			dFitsStatus = 0;	// Initialize status before calling fitsio routines
	fitsfile*	fptr		= NULL;

	//  Read the other file parameters
	// +------------------------------------------------------------+
	long lNAxes[ CArcFitsFile::NAXES_SIZE ]	= { 0, 0, 0 };
	int  dNAxis								= 0;
	int  dBitsPerPixel						= 0;

	rAnotherFitsFile.GetParameters( &lNAxes[ 0 ], &dNAxis, &dBitsPerPixel );

	//  Create the new file
	// +------------------------------------------------------------+
	string sFilename( pszNewFileName );

	if ( sFilename.empty() )
	{
		ThrowException( "CArcFitsFile",
						 string( "Invalid file name : " ) + sFilename );
	}

	sFilename = "!" + sFilename;	// ! = Force the file to be overwritten
	fits_create_file( &fptr, sFilename.c_str(), &dFitsStatus );

	if ( dFitsStatus )
	{
		m_fptr.reset();

		ThrowException( "CArcFitsFile", dFitsStatus );
	}

	m_fptr.reset( fptr );
	m_pDataHeader.reset();
	m_pDataBuffer.reset();

	m_lFPixel = 0;
	m_lFrame  = 0;

	//  Copy the FITS header
	// +------------------------------------------------------------+
	fits_copy_header( rAnotherFitsFile.GetBaseFile(),
					  m_fptr.get(),
					  &dFitsStatus );

	if ( dFitsStatus )
	{
		m_fptr.reset();

		ThrowException( "CArcFitsFile", dFitsStatus );
	}
}

// +---------------------------------------------------------------------------+
// |  Constructor for OPENING an existing FITS file for reading and/or writing |
// +---------------------------------------------------------------------------+
// |  Opens an existing FITS file for reading. May contain a single image or   |
// |  a FITS data cube.                                                        |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> pszFilename - The file to open, including path.                  |
// |  <IN> -> dMode       - Read/Write mode. Can be READMODE or READWRITEMODE. |
// +---------------------------------------------------------------------------+
CArcFitsFile::CArcFitsFile( const char* pszFilename, int dMode )
{
    int			dFitsStatus = 0;	// Initialize status before calling fitsio routines
	fitsfile*	fptr		= NULL;
	string		sFilename( pszFilename );

	// Verify filename and make sure the kernel image
	// buffer has been initialized.
	// --------------------------------------------------
	if ( sFilename.empty() )
	{
		ThrowException( "CArcFitsFile",
						 string( "Invalid file name : " ) + sFilename );
	}

	// Open the FITS file
	// --------------------------------------------------
	fits_open_file( &fptr, sFilename.c_str(), dMode, &dFitsStatus );

	if ( dFitsStatus )
	{
		m_fptr.reset();

		ThrowException( "CArcFitsFile", dFitsStatus );
	}

	m_fptr.reset( fptr );
	m_pDataHeader.reset();
	m_pDataBuffer.reset();

	m_lFPixel = 0;
	m_lFrame  = 0;
}

// +---------------------------------------------------------------------------+
// |  Constructor for CREATING a new FITS file for writing                     |
// +---------------------------------------------------------------------------+
// |  Creates a FITS file the will contain a single image.                     |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> pszFilename   - The to create, including path.                   |
// |  <IN> -> rows          - The number of rows in the image.                 |
// |  <IN> -> cols          - The number of columns in the image.              |
// |  <IN> -> dBitsPerPixel - Can be 16 ( BPP16 ) or 32 ( BPP32 ).             |
// |  <IN> -> is3D          - True to create a fits data cube.                 |
// +---------------------------------------------------------------------------+
CArcFitsFile::CArcFitsFile( const char* pszFilename, int dRows, int dCols,
						    int dBitsPerPixel, bool bIs3D )
{
	int			dFitsStatus = 0;		// Initialize status before calling fitsio routines
	long*		plNaxes     = ( bIs3D ? new long[ 3 ] : new long[ 2 ] );	// { COLS, ROWS, PLANE# }
	int			dNAxis      = ( bIs3D ? 3 : 2 );
	int			dImageType  = 0;
	fitsfile*	fptr		= NULL;

	string sFilename( pszFilename );

	if ( bIs3D )
	{
		plNaxes[ 0 ] = dCols;
		plNaxes[ 1 ] = dRows;
		plNaxes[ 2 ] = 1;
	}
	else
	{
		plNaxes[ 0 ] = dCols;
		plNaxes[ 1 ] = dRows;
	}

	// Verify image dimensions
	// --------------------------------------------------
	if ( dRows <= 0 )
	{
		ThrowException( "CArcFitsFile",
						"Row dimension should be greater than zero!" );
	}

	if ( dCols <= 0 )
	{
		ThrowException( "CArcFitsFile",
						"Column dimension should be greater than zero!" );
	}

	// Verify filename
	// --------------------------------------------------
	if ( sFilename.empty() )
	{
		ThrowException( "CArcFitsFile",
						 string( "Invalid file name : " ) + sFilename );
	}

	// Verify bits-per-pixel
	// --------------------------------------------------
	if ( dBitsPerPixel != CArcFitsFile::BPP16 && dBitsPerPixel != CArcFitsFile::BPP32 )
	{
		ThrowException( "CArcFitsFile",
			"Invalid dBitsPerPixel, should be 16 ( BPP16 ) or 32 ( BPP32 )." );
	}

	// Create a new FITS file
	// --------------------------------------------------
	sFilename = "!" + sFilename;	// ! = Force the file to be overwritten
	fits_create_file( &fptr, sFilename.c_str(), &dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "CArcFitsFile", dFitsStatus );
	}

	m_fptr.reset( fptr );

   	// Create the primary array image - 16-bit short integer pixels
	// -------------------------------------------------------------
	if ( dBitsPerPixel == CArcFitsFile::BPP16 ) { dImageType = USHORT_IMG; }
	else { dImageType = ULONG_IMG; }

	fits_create_img( m_fptr.get(), dImageType, dNAxis, plNaxes, &dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "CArcFitsFile", dFitsStatus );
	}

	m_pDataHeader.reset();
	m_pDataBuffer.reset();

	m_lFPixel = 0;
	m_lFrame  = 0;

	delete [] plNaxes;
}

// +---------------------------------------------------------------------------+
// |  Class destructor                                                         |
// +---------------------------------------------------------------------------+
// |  Destroys the class. Deallocates any header and data buffers. Closes any  |
// |  open FITS pointers.                                                      |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// +---------------------------------------------------------------------------+
CArcFitsFile::~CArcFitsFile()
{
	m_pDataHeader.reset();
	m_pDataBuffer.reset();

	//  Using release because reset causes errors with glib.
	//  Results in a "double free on pointer" error.
	// -----------------------------------------------------
	fitsfile* fptr = m_fptr.release();

	//  DeleteBuffer requires access to the file,
	//  so don't close the file until after DeleteBuffer
	//  has been called!!!
	// -----------------------------------------------------
	if ( fptr != NULL )
	{
		int dFitsStatus = 0;

		fits_close_file( fptr, &dFitsStatus );
	}

	m_lFPixel = 0;
	m_lFrame  = 0;
}

// +---------------------------------------------------------------------------+
// |  GetFilename                                                              |
// +---------------------------------------------------------------------------+
// |  Returns the filename associated with this CArcFitsFile object.           |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// +---------------------------------------------------------------------------+
const std::string CArcFitsFile::GetFilename()
{
	char szFilename[ 150 ];
	int  dFitsStatus = 0;

	// Verify FITS file handle
	// --------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "GetFilename", "Invalid FITS handle, no file open" );
	}

	fits_file_name( m_fptr.get(), szFilename, &dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "GetFilename", dFitsStatus );
	}

	return std::string( szFilename );
}

// +---------------------------------------------------------------------------+
// | GetHeader                                                                 |
// +---------------------------------------------------------------------------+
// |  Returns the FITS header as an array of strings.                          |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <OUT> -> pKeyCount - The number of keys in the returned header array.    |
// +---------------------------------------------------------------------------+
string* CArcFitsFile::GetHeader( int* pKeyCount )
{
	int dFitsStatus = 0;
	int dNumOfKeys  = 0;

	// Verify FITS file handle
	// --------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "GetHeader", "Invalid FITS handle, no file open" );
	}

	fits_get_hdrspace( m_fptr.get(), &dNumOfKeys, NULL, &dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "GetHeader", dFitsStatus );
	}

	m_pDataHeader.reset( new string[ dNumOfKeys ], &CArcFitsFile::ArrayDeleter<string> ); //ArrayDeleter<string>() ); //&CArcFitsFile::ArrayDeleter );

	char pszCard[ 100 ];

	for ( int i=0; i<dNumOfKeys; i++ )
	{
		fits_read_record( m_fptr.get(),
						  ( i + 1 ),
						  &pszCard[ 0 ],
						  &dFitsStatus );

		if ( dFitsStatus )
		{
			ThrowException( "GetHeader", dFitsStatus );
		}

		m_pDataHeader.get()[ i ] = string( pszCard );
	}

	*pKeyCount = dNumOfKeys;

	return m_pDataHeader.get();
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
// |  <IN> -> pszKey    - The header keyword. Can be NULL. Ex: SIMPLE          |
// |  <IN> -> pKeyVal   - The value associated with the key. Ex: T             |
// |  <IN> -> dValType  - The keyword value type, as defined in CArcFitsFile.h |
// |  <IN> -> pszComment - The comment to attach to the keyword.  Can be NULL. |
// +---------------------------------------------------------------------------+
void CArcFitsFile::WriteKeyword( char* pszKey, void* pKeyVal, int dValType, char* pszComment )
{
    int dFitsStatus = 0;

	// Verify FITS file handle
	// --------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "WriteKeyword",
						"Invalid FITS handle, no file open" );
	}

	// Verify value pointer
	// --------------------------------------------------
	if ( pKeyVal == NULL )
	{
		ThrowException( "WriteKeyword",
						"Invalid FITS value, cannot be NULL" );
	}

	// Update the fits header with the specified keys
	// -------------------------------------------------------------
	switch ( dValType )
	{
		case FITS_STRING_KEY:
		{
			dValType = TSTRING;
		}
		break;

		case FITS_INTEGER_KEY:
		{
			dValType = TINT;
		}
		break;

		case FITS_DOUBLE_KEY:
		{
			dValType = TDOUBLE;
		}
		break;

		case FITS_LOGICAL_KEY:
		{
			dValType = TLOGICAL;
		}
		break;

		case FITS_COMMENT_KEY:
		case FITS_HISTORY_KEY:
		case FITS_DATE_KEY:
		{
		}
		break;

		default:
		{
			dValType = -1;

			string msg =
					string( "Invalid FITS key type. Must be one of: " ) +
					string( "FITS_STRING_KEY, FITS_INTEGER_KEY, " )     +
					string( "FITS_DOUBLE_KEY, FITS_LOGICAL_KEY." )      +
					string( "FITS_COMMENT_KEY, FITS_HISTORY_KEY, " )    +
					string( "FITS_DATA_KEY" );

			ThrowException( "WriteKeyword", msg );
		}
	}

	//
	// Write ( append ) a COMMENT keyword to the header. The comment
	// string will be continued over multiple keywords if it is
	// longer than 70 characters. 
	//
	if ( dValType == FITS_COMMENT_KEY )
	{
		fits_write_comment( m_fptr.get(), ( char * )pKeyVal, &dFitsStatus );
	}

	//
	// Write ( append ) a HISTORY keyword to the header. The history
	// string will be continued over multiple keywords if it is
	// longer than 70 characters. 
	//
	else if ( dValType == FITS_HISTORY_KEY )
	{
		fits_write_history( m_fptr.get(), ( char * )pKeyVal, &dFitsStatus );
	}

	//
	// Write the DATE keyword to the header. The keyword value will contain
	// the current system date as a character string in 'yyyy-mm-ddThh:mm:ss'
	// format. If a DATE keyword already exists in the header, then this
	// routine will simply update the keyword value with the current date.
	//
	else if ( dValType == FITS_DATE_KEY )
	{
		fits_write_date( m_fptr.get(), &dFitsStatus );
	}

	//
	// Write a keyword of the appropriate data type into the header
	//
	else
	{
		fits_update_key( m_fptr.get(),
						 dValType,
						 pszKey,
						 pKeyVal,
						 pszComment,
						 &dFitsStatus );
	}

	if ( dFitsStatus )
	{
		ThrowException( "WriteKeyword", dFitsStatus );
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
// |  <IN> -> pszKey     - The header keyword. Ex: SIMPLE                      |
// |  <IN> -> pKeyVal    - The value associated with the key. Ex: T            |
// |  <IN> -> dValType   - The keyword type, as defined in CArcFitsFile.h      |
// |  <IN> -> pszComment - The comment to attach to the keyword.               |
// +---------------------------------------------------------------------------+
void CArcFitsFile::UpdateKeyword( char* pszKey, void* pKeyVal, int dValType, char* pszComment )
{
	WriteKeyword( pszKey, pKeyVal, dValType, pszComment );
}

// +---------------------------------------------------------------------------+
// |  GetParameters                                                            |
// +---------------------------------------------------------------------------+
// |  Reads the lNAxes, dNAxis and bits-per-pixel header values from a FITS    |
// |  file.                                                                    |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <OUT> -> lNAxes        - MUST be a pointer to:  int lNAxes[ 3 ]. Index 0 |
// |                          will have column size, 1 will have row size, and |
// |                          2 will have number of frames if file is a data   |
// |                          cube. Or use NAXES_COL, NAXES_ROW, NAXES_NOF.    |
// |  <OUT> -> dNAxis        - Optional pointer to int for NAXIS keyword value |
// |  <OUT> -> dBitsPerPixel - Optional pointer to int for BITPIX keyword value|
// +---------------------------------------------------------------------------+
void CArcFitsFile::GetParameters( long* pNaxes, int* pNaxis, int* pBitsPerPixel )
{
 	int  dFitsStatus     = 0;
	int  dTempBPP        = 0;
	int  dTempNAXIS      = 0;
	long lTempNAXES[ CArcFitsFile::NAXES_SIZE ] = { 0, 0, 0 };

	// Verify FITS file handle
	// ----------------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "GetParameters",
						"Invalid FITS handle, no file open ( GetParameters )" );
	}

	// Verify that at least one parameter is being requested
	// ----------------------------------------------------------
	if ( pNaxes == NULL && pNaxis == NULL && pBitsPerPixel == NULL )
	{
		ThrowException(
				"GetParameters",
				"All parameters are NULL. Why did you bother calling this method?" );
	}

	// Get the image parameters
	// ----------------------------------------------------------
	fits_get_img_param( m_fptr.get(),
						CArcFitsFile::NAXES_SIZE,
						&dTempBPP,
						&dTempNAXIS,
						lTempNAXES,
						&dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "GetParameters", dFitsStatus );
	}

	if ( pNaxes != NULL )
	{
		pNaxes[ CArcFitsFile::NAXES_COL ] = lTempNAXES[ CArcFitsFile::NAXES_COL ];
		pNaxes[ CArcFitsFile::NAXES_ROW ] = lTempNAXES[ CArcFitsFile::NAXES_ROW ];
		pNaxes[ CArcFitsFile::NAXES_NOF ] = lTempNAXES[ CArcFitsFile::NAXES_NOF ];
	}

	if ( pNaxis != NULL )
	{
		*pNaxis = dTempNAXIS;
	}

	if ( pBitsPerPixel != NULL )
	{
		*pBitsPerPixel = dTempBPP;
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
// +---------------------------------------------------------------------------+
long CArcFitsFile::GetNumberOfFrames()
{
	long lNAxes[ CArcFitsFile::NAXES_SIZE ] = { 0, 0, 0 };

	GetParameters( &lNAxes[ 0 ] );

	return lNAxes[ CArcFitsFile::NAXES_NOF ];
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
// +---------------------------------------------------------------------------+
long CArcFitsFile::GetRows()
{
	long lNAxes[ CArcFitsFile::NAXES_SIZE ] = { 0, 0, 0 };

	GetParameters( &lNAxes[ 0 ] );

	return lNAxes[ CArcFitsFile::NAXES_ROW ];
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
// +---------------------------------------------------------------------------+
long CArcFitsFile::GetCols()
{
	long lNAxes[ CArcFitsFile::NAXES_SIZE ] = { 0, 0, 0 };

	GetParameters( &lNAxes[ 0 ] );

	return lNAxes[ CArcFitsFile::NAXES_COL ];
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
// +---------------------------------------------------------------------------+
int CArcFitsFile::GetNAxis()
{
	int dNAxis = 0;

	GetParameters( NULL, &dNAxis, NULL );

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
// +---------------------------------------------------------------------------+
int CArcFitsFile::GetBpp()
{
	int dBpp = 0;

	GetParameters( NULL, NULL, &dBpp );

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
// +---------------------------------------------------------------------------+
void CArcFitsFile::GenerateTestData()
{
	int  dFitsStatus   = 0;	// Initialize status before calling fitsio routines
	int  dBitsPerPixel = 0;
	int  dNAxis        = 0;
	long lNAxes[ 2 ]   = { 0, 0 };
	long lNElements    = 0;
	int  dFPixel       = 1;

	// Verify FITS file handle
	// ----------------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "GenerateTestData",
						"Invalid FITS handle, no file open" );
	}

	// Get the image parameters
	// ----------------------------------------------------------
	fits_get_img_param( m_fptr.get(),
						2,
						&dBitsPerPixel,
						&dNAxis,
						lNAxes,
						&dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "GenerateTestData", dFitsStatus );
	}

	// Set number of pixels to write
	// ----------------------------------------------------------
	lNElements = lNAxes[ NAXES_ROW ] * lNAxes[ NAXES_COL ];

	if ( dBitsPerPixel == CArcFitsFile::BPP16 )
	{
		unsigned short *pU16Buf = NULL;

		try
		{
			pU16Buf = new unsigned short[ lNElements ];
		}
		catch ( bad_alloc &maEx )
		{
			ThrowException( "GenerateTestData",
							 maEx.what() );
		}

		for ( long i=0, j=0; i<lNElements; i++, j++ )
		{
			pU16Buf[ i ] = static_cast<unsigned short>( j );

			if ( j >= ( T_SIZE( unsigned short ) - 1 ) ) { j = 0; }
		}

		fits_write_img( m_fptr.get(),
						TUSHORT,
						dFPixel,
						lNElements,
						pU16Buf,
						&dFitsStatus );

		delete [] pU16Buf;
	}

	else if ( dBitsPerPixel == CArcFitsFile::BPP32 )
	{
		unsigned int *pUIntBuf = NULL;
		unsigned int  val       = 0;

		try
		{
			pUIntBuf = new unsigned int[ lNElements ];
		}
		catch ( bad_alloc &maEx )
		{
			ThrowException( "GenerateTestData",
							 maEx.what() );
		}

		for ( long i=0; i<lNElements; i++ )
		{
			pUIntBuf[ i ] = val;
			if ( val >= ( T_SIZE( unsigned int ) - 1 ) ) val = 0;
			val++;
		}

		fits_write_img( m_fptr.get(),
						TUINT,
						dFPixel,
						lNElements,
						pUIntBuf,
						&dFitsStatus );

		delete [] pUIntBuf;
	}

	// Invalid bits-per-pixel value
	// ---------------------------------------------------------------
	else
	{
		ThrowException( "GenerateTestData",
				string( "Invalid dBitsPerPixel, " ) +
				string( "should be 16 ( BPP16 ) or 32 ( BPP32 )." ) );
	}

	if ( dFitsStatus )
	{
		ThrowException( "GenerateTestData", dFitsStatus );
	}
}

// +---------------------------------------------------------------------------+
// |  ReOpen                                                                   |
// +---------------------------------------------------------------------------+
// |  Closes and re-opens the current FITS file.                               |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// +---------------------------------------------------------------------------+
void CArcFitsFile::ReOpen()
{
	fitsfile*	fptr		= NULL;
	int			dFitsStatus	= 0;
	int			dIOMode		= 0;

	std::string sFilename = GetFilename();

	fits_file_mode( m_fptr.get(), &dIOMode, &dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "ReOpen", dFitsStatus );
	}

	fits_close_file( m_fptr.get(), &dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "ReOpen", dFitsStatus );
	}

	fits_open_file( &fptr,
					sFilename.c_str(),
					dIOMode,
					&dFitsStatus );

	if ( dFitsStatus )
	{
		m_fptr.reset();

		ThrowException( "ReOpen", dFitsStatus );
	}

	if ( fptr == NULL )
	{
		ThrowException( "ReOpen", "Failed to ReOpen file!" );
	}

	m_pDataBuffer.reset();
	m_pDataHeader.reset();
	m_fptr.reset( fptr );
}


// +---------------------------------------------------------------------------+
// |  Compare ( Single Images )                                                |
// +---------------------------------------------------------------------------+
// |  Compares the data image in this FITS file to another one. Returns true   |
// |  if they are a match; false otherwise. The headers ARE NOT compared;      |
// |  except to verify image dimensions. NOTE: Only supports 16-bit image,     |
// |  but doesn't check for it.                                                |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> anotherCFitsFile - Reference to FITS file to compare.            |
// |                                                                           |
// |  Returns: true if the image data matches.                                 |
// +---------------------------------------------------------------------------+
bool CArcFitsFile::Compare( CArcFitsFile& anotherCFitsFile )
{
	bool bAreTheSame = true;

	//  Read the image dimensions from input file
	// +----------------------------------------------+
	long lAnotherNaxes[ 3 ] = { 0, 0, 0 };
	int  dAnotherBpp        = 0;

	anotherCFitsFile.GetParameters( lAnotherNaxes, NULL, &dAnotherBpp );

	//  Read the image dimensions from this file
	// +----------------------------------------------+
	long lNaxes[ 3 ] = { 0, 0, 0 };
	int  dBpp        = 0;

	this->GetParameters( lNaxes, NULL, &dBpp );

	//  Verify image dimensions match between files
	// +----------------------------------------------+
	if ( lAnotherNaxes[ CArcFitsFile::NAXES_ROW ] != lNaxes[ CArcFitsFile::NAXES_ROW ] ||
		 lAnotherNaxes[ CArcFitsFile::NAXES_COL ] != lNaxes[ CArcFitsFile::NAXES_COL ] )
	{
		ostringstream oss;

		oss << "Image dimensions of comparison files DO NOT match! FITS 1: "
			<< lNaxes[ CArcFitsFile::NAXES_ROW ] << "x" << lNaxes[ CArcFitsFile::NAXES_COL ]
			<< " FITS 2: " << lAnotherNaxes[ CArcFitsFile::NAXES_ROW ] << "x"
			<< lAnotherNaxes[ CArcFitsFile::NAXES_COL ] << ends;

		ThrowException( "Compare", oss.str() );
	}

	if ( dAnotherBpp != dBpp )
	{
		ostringstream oss;

		oss << "Image bits-per-pixel of comparison files DO NOT match! FITS 1: "
			<< dBpp	<< " FITS 2: " << dAnotherBpp << ends;

		ThrowException( "Compare", oss.str() );
	}

	//  Read input file image buffer
	// +----------------------------------------------+
	unsigned short *pAnotherUShortBuf =
				( unsigned short * )anotherCFitsFile.Read();

	if ( pAnotherUShortBuf == NULL )
	{
		ThrowException( "Compare",
						"Failed to read input FITS file image data!" );
	}

	//  Read this file image buffer
	// +----------------------------------------------+
	unsigned short *pU16Buf = ( unsigned short * )this->Read();

	if ( pAnotherUShortBuf == NULL )
	{
		ThrowException( "Compare",
						"Failed to read this FITS file image data!" );
	}

	//  Compare image buffers
	// +----------------------------------------------+
	for ( int i=0; i<( lNaxes[ CArcFitsFile::NAXES_ROW ] * lNaxes[ CArcFitsFile::NAXES_COL ] ); i++ )
	{
		if ( pU16Buf[ i ] != pAnotherUShortBuf[ i ] )
		{
			bAreTheSame = false;

			break;
		}
	}

	return bAreTheSame;
}

// +---------------------------------------------------------------------------+
// |  Write ( Single Image )                                                   |
// +---------------------------------------------------------------------------+
// |  Writes to a FITS file the will contain a single image.                   |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> data   - Pointer to image data to write.                         |
// +---------------------------------------------------------------------------+
void CArcFitsFile::Write( void* pData )
{
	int  dFitsStatus   = 0;	// Initialize status before calling fitsio routines
	int  dBitsPerPixel = 0;
	int  dNAxis        = 0;
	long lNAxes[ 2 ]   = { 0, 0 };
	long lNElements    = 0;
	int  dFPixel       = 1;

	// Verify FITS file handle
	// ----------------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "Write",
						"Invalid FITS handle, no file open" );
	}

	// Get the image parameters
	// ----------------------------------------------------------
	fits_get_img_param( m_fptr.get(),
						2,
						&dBitsPerPixel,
						&dNAxis,
						lNAxes,
						&dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "Write", dFitsStatus );
	}

	// Verify parameters
	// ----------------------------------------------------------
	if ( dNAxis != 2 )
	{
		ThrowException( "Write",
				string( "Invalid NAXIS value. " ) +
				string( "This method is only valid for a " ) +
				string( "file containing a single image." ) );
	}

	if ( pData == NULL )
	{
		ThrowException( "Write", "Invalid data buffer pointer" );
	}

	// Set number of pixels to write
	// ----------------------------------------------------------
	lNElements = lNAxes[ 0 ] * lNAxes[ 1 ];

	// Write 16-bit data
	// ---------------------------------------------------------------
	if ( dBitsPerPixel == CArcFitsFile::BPP16 )
	{
		fits_write_img( m_fptr.get(),
						TUSHORT,
						dFPixel,
						lNElements,
						( unsigned short * )pData,
						&dFitsStatus );
	}

	// Write 32-bit data
	// ---------------------------------------------------------------
	else if ( dBitsPerPixel == CArcFitsFile::BPP32 )
	{
		fits_write_img( m_fptr.get(),
						TUINT,
						dFPixel,
						lNElements,
						( unsigned int * )pData,
						&dFitsStatus );
	}

	// Invalid bits-per-pixel value
	// ---------------------------------------------------------------
	else
	{
		ThrowException(
			"Write",
			"Invalid dBitsPerPixel, should be 16 ( BPP16 ) or 32 ( BPP32 )." );
	}

	if ( dFitsStatus )
	{
		ThrowException( "Write", dFitsStatus );
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
// +---------------------------------------------------------------------------+
void CArcFitsFile::Write( void* pData, unsigned int uBytesToWrite, int dFPixel )
{
	int  dFitsStatus   = 0;	// Initialize status before calling fitsio routines
	int  dBitsPerPixel = 0;
	int  dNAxis        = 0;
	long lNAxes[ 2 ]   = { 0, 0 };
	long lNElements    = 0;
	bool bMultiWrite  = false;

	//
	// Verify FITS file handle
	// ----------------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "Write",
						"Invalid FITS handle, no file open" );
	}

	//
	// Get the image parameters
	// ----------------------------------------------------------
	fits_get_img_param( m_fptr.get(),
						2,
						&dBitsPerPixel,
						&dNAxis,
						lNAxes,
						&dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "Write", dFitsStatus );
	}

	//
	// Verify parameters
	// ----------------------------------------------------------
	if ( dNAxis != 2 )
	{
		ThrowException( "Write",
				string( "Invalid NAXIS value. " ) +
				string( "This method is only valid for a " ) +
				string( "file containing a single image." ) );
	}

	if ( pData == NULL )
	{
		ThrowException( "Write", "Invalid data buffer pointer" );
	}

	if ( dFPixel >= ( lNAxes[ 0 ] * lNAxes[ 1 ] ) )
	{
		ThrowException( "Write",
				string( "Invalid start position, " ) +
				string( " pixel position outside image size." ) );
	}

	//
	// Set the start pixel ( position ) within the file
	// ---------------------------------------------------------
	if ( dFPixel < 0 && this->m_lFPixel == 0 )
	{
		this->m_lFPixel = 1;
		bMultiWrite  = true;
	}

	else if ( dFPixel == 0 && this->m_lFPixel != 0 )
	{
		this->m_lFPixel = 1;
		bMultiWrite  = true;
	}

	else if ( dFPixel < 0 && this->m_lFPixel != 0 )
	{
		bMultiWrite  = true;
	}

	else
	{
		this->m_lFPixel = dFPixel + 1;
		bMultiWrite  = false;
	}

	//
	// Verify the start position
	// ----------------------------------------------------------
	if ( this->m_lFPixel >= ( lNAxes[ 0 ] * lNAxes[ 1 ] ) )
	{
		ThrowException( "Write",
				string( "Invalid start position, " ) +
				string( " pixel position outside image size." ) );
	}

	//
	// Write 16-bit data
	// ----------------------------------------------------------
	if ( dBitsPerPixel == CArcFitsFile::BPP16 )
	{
		// Number of pixels to write
		lNElements = uBytesToWrite / sizeof( unsigned short );

		fits_write_img( m_fptr.get(),
						TUSHORT,
						this->m_lFPixel,
						lNElements,
						( unsigned short * )pData,
						&dFitsStatus );
	}

	//
	// Write 32-bit data
	// ----------------------------------------------------------
	else if ( dBitsPerPixel == CArcFitsFile::BPP32 )
	{
		// Number of pixels to write
		lNElements = uBytesToWrite / sizeof( unsigned int );

		fits_write_img( m_fptr.get(),
						TUINT,
						this->m_lFPixel,
						lNElements,
						( unsigned int * )pData,
						&dFitsStatus );
	}

	//
	// Invalid bits-per-pixel value
	// ---------------------------------------------------------------
	else
	{
		ThrowException( "Write",
				string( "Invalid dBitsPerPixel, " ) +
				string( " should be 16 ( BPP16 ) or 32 ( BPP32 )." ) );
	}

	if ( dFitsStatus )
	{
		ThrowException( "Write", dFitsStatus );
	}

	if ( bMultiWrite )
	{
		this->m_lFPixel += lNElements;
	}
}

// +---------------------------------------------------------------------------+
// |  WriteSubImage ( Single Image )                                           |
// +---------------------------------------------------------------------------+
// |  Writes a sub-image to a FITS file.                                       |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> data         - The pixel data.                                   |
// |  <IN> -> llrow        - The lower left row of the sub-image.              |
// |  <IN> -> llcol        - The lower left column of the sub-image.           |
// |  <IN> -> urrow        - The upper right row of the sub-image.             |
// |  <IN> -> urcol        - The upper right column of the sub-image.          |
// +---------------------------------------------------------------------------+
void CArcFitsFile::WriteSubImage( void* pData, int llrow, int llcol, int urrow, int urcol )
{
	int  dFitsStatus   = 0;		// Initialize status before calling fitsio routines
	int  dBitsPerPixel = 0;
	int  dNAxis        = 0;
	long lNAxes[ 2 ]   = { 0, 0 };

	// Verify FITS file handle
	// ----------------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "WriteSubImage",
						"Invalid FITS handle, no file open" );
	}

	// Get the image parameters
	// ----------------------------------------------------------
	fits_get_img_param( m_fptr.get(),
						2,
						&dBitsPerPixel,
						&dNAxis,
						lNAxes,
						&dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "WriteSubImage", dFitsStatus );
	}

	// Verify parameters
	// ----------------------------------------------------------
	if ( dNAxis != 2 )
	{
		ThrowException( "WriteSubImage",
				string( "Invalid NAXIS value. " ) +
				string( "This method is only valid for a file " ) +
				string( "containing a single image." ) );
	}

	if ( llrow > urrow )
	{
		ThrowException( "WriteSubImage", "Invalid llrow/urrow parameter!" );
	}

	if ( llcol > urcol )
	{
		ThrowException( "WriteSubImage", "Invalid llcol/urcol parameter!" );
	}

	if ( llrow < 0 || llrow >= lNAxes[ 1 ] )
	{
		ThrowException( "WriteSubImage", "Invalid llrow parameter!" );
	}

	if ( urrow < 0 || urrow >= lNAxes[ 1 ] )
	{
		ThrowException( "WriteSubImage", "Invalid urrow parameter!" );
	}

	if ( llcol < 0 || llcol >= lNAxes[ 0 ] )
	{
		ThrowException( "WriteSubImage", "Invalid llcol parameter!" );
	}

	if ( urcol < 0 || urcol >= lNAxes[ 0 ] )
	{
		ThrowException( "WriteSubImage", "Invalid urcol parameter!" );
	}

	if ( pData == NULL )
	{
		ThrowException( "WriteSubImage", "Invalid data buffer pointer!" );
	}

	// Set the subset start pixels
	// ---------------------------------------------------------------
	long dFPixel[ 2 ] = { llcol + 1, llrow + 1 };
	long lpixel[ 2 ] = { urcol + 1, urrow + 1 };

	// Write 16-bit data
	// ---------------------------------------------------------------
	if ( dBitsPerPixel == CArcFitsFile::BPP16 )
	{
		fits_write_subset( m_fptr.get(),
						   TUSHORT,
						   dFPixel,
						   lpixel,
						   pData,
						   &dFitsStatus );
	}

	// Write 32-bit data
	// ---------------------------------------------------------------
	else if ( dBitsPerPixel == CArcFitsFile::BPP32 )
	{
		fits_write_subset( m_fptr.get(),
						   TUINT,
						   dFPixel,
						   lpixel,
						   pData,
						   &dFitsStatus );
	}

	// Invalid bits-per-pixel value
	// ---------------------------------------------------------------
	else
	{
		ThrowException( "WriteSubImage",
				string( "Invalid dBitsPerPixel " ) +
				string( ", should be 16 ( BPP16 ) or 32 ( BPP32 )." ) );
	}

	if ( dFitsStatus )
	{
		ThrowException( "WriteSubImage", dFitsStatus );
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
// +---------------------------------------------------------------------------+
void *CArcFitsFile::ReadSubImage( int llrow, int llcol, int urrow, int urcol )
{
	int  dFitsStatus   = 0;	// Initialize status before calling fitsio routines
	int  dBitsPerPixel = 0;
	int  dNAxis        = 0;
	int  dAnyNul       = 0;
	long lNAxes[ 2 ]   = { 0, 0 };

	// Verify FITS file handle
	// ----------------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "ReadSubImage",
						"Invalid FITS handle, no file open" );
	}

	// Get the image parameters
	// ----------------------------------------------------------
	fits_get_img_param( m_fptr.get(),
						2,
						&dBitsPerPixel,
						&dNAxis,
						lNAxes,
						&dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "ReadSubImage", dFitsStatus );
	}

	// Verify parameters
	// ----------------------------------------------------------
	if ( dNAxis != 2 )
	{
		ThrowException( "ReadSubImage",
				string( "Invalid NAXIS value. " ) +
				string( "This method is only valid for a file " ) +
				string( "containing a single image." ) );
	}

	if ( llrow > urrow )
	{
		ThrowException( "ReadSubImage", "Invalid llrow/urrow parameter!" );
	}

	if ( llcol > urcol )
	{
		ThrowException( "ReadSubImage", "Invalid llcol/urcol parameter!" );
	}

	if ( llrow < 0 || llrow > lNAxes[ 1 ] )
	{
		ThrowException( "ReadSubImage", "Invalid llrow parameter!" );
	}

	if ( urrow < 0 || urrow > lNAxes[ 1 ] )
	{
		ThrowException( "ReadSubImage", "Invalid urrow parameter!" );
	}

	if ( llcol < 0 || llcol > lNAxes[ 0 ] )
	{
		ThrowException( "ReadSubImage", "Invalid llcol parameter!" );
	}

	if ( urcol < 0 || urcol > lNAxes[ 0 ] )
	{
		ThrowException( "ReadSubImage", "Invalid urcol parameter!" );
	}

	if ( long( urrow ) == lNAxes[ 1 ] ) { urrow = int( lNAxes[ 1 ] - 1 ); }
	if ( long( urcol ) == lNAxes[ 0 ] ) { urcol = int( lNAxes[ 0 ] - 1 ); }

	// Set the subset start pixels
	// ---------------------------------------------------------------
	long lFPixel[ 2 ] = { llcol + 1, llrow + 1 };
	long lpixel[ 2 ]  = { urcol + 1, urrow + 1 };

	// The read routine also has an inc parameter which can be used to
	// read only every inc-th pixel along each dimension of the image.
	// Normally inc[0] = inc[1] = 1 to read every pixel in a 2D image.
	// To read every other pixel in the entire 2D image, set 
	// ---------------------------------------------------------------
	long lInc[ 2 ] = { 1, 1 };

	// Set the data length ( in pixels )
	// ----------------------------------------------------------
	long lDataLength = lNAxes[ 0 ] * lNAxes[ 1 ];

	// Read 16-bit data
	// ---------------------------------------------------------------
	if ( dBitsPerPixel == CArcFitsFile::BPP16 )
	{
		m_pDataBuffer.reset( new unsigned short[ lDataLength ],
							 &CArcFitsFile::ArrayDeleter<unsigned short> );	//ArrayDeleter<unsigned short>() ); //&CArcFitsFile::ArrayDeleter );

		if ( m_pDataBuffer.get() == NULL )
		{
			ThrowException(
					"ReadSubImage",
					"Failed to allocate buffer for image pixel data" );
		}

		fits_read_subset( m_fptr.get(),
						  TUSHORT,
						  lFPixel,
						  lpixel,
						  lInc,
						  0,
						  m_pDataBuffer.get(),
						  &dAnyNul,
						  &dFitsStatus );
	}

	// Read 32-bit data
	// ---------------------------------------------------------------
	else if ( dBitsPerPixel == CArcFitsFile::BPP32 )
	{
		m_pDataBuffer.reset( new unsigned int[ lDataLength ],
							 &CArcFitsFile::ArrayDeleter<unsigned int> ); //&CArcFitsFile::ArrayDeleter );

		if ( m_pDataBuffer.get() == NULL )
		{
			ThrowException(
					"ReadSubImage",
					"Failed to allocate buffer for image pixel data" );
		}

		fits_read_subset( m_fptr.get(),
						  TUINT,
						  lFPixel,
						  lpixel,
						  lInc,
						  0,
						  m_pDataBuffer.get(),
						  &dAnyNul,
						  &dFitsStatus );
	}

	// Invalid bits-per-pixel value
	// ---------------------------------------------------------------
	else
	{
		ThrowException(
			"ReadSubImage",
			"Invalid dBitsPerPixel, should be 16 ( BPP16 ) or 32 ( BPP32 )." );
	}

	if ( dFitsStatus )
	{
		ThrowException( "ReadSubImage", dFitsStatus );
	}

	return m_pDataBuffer.get();
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
// +---------------------------------------------------------------------------+
void *CArcFitsFile::Read()
{
	int  dBitsPerPixel = 0;
	int  dFitsStatus   = 0;
	int  dNAxis        = 0;
	long lNAxes[ 2 ]   = { 0, 0 };

	// Verify FITS file handle
	// --------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "Read",
						"Invalid FITS handle, no file open" );
	}

	// Get the image parameters
	// ----------------------------------------------------------
	fits_get_img_param( m_fptr.get(),
						2,
						&dBitsPerPixel,
						&dNAxis,
						lNAxes,
						&dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "Read", dFitsStatus );
	}

	// Verify NAXIS parameter
	// ----------------------------------------------------------
	if ( dNAxis != 2 )
	{
		ThrowException( "Read",
				string( "Invalid NAXIS value. " ) +
				string( "This method is only valid for a file " ) +
				string( "containing a single image." ) );
	}

	// Set the data length ( in pixels )
	// ----------------------------------------------------------
	long lDataLength = lNAxes[ 0 ] * lNAxes[ 1 ];

	// Get the image data
	// ----------------------------------------------------------
	if ( dBitsPerPixel == CArcFitsFile::BPP16 )
	{
		m_pDataBuffer.reset( new unsigned short[ lDataLength ],
							 &CArcFitsFile::ArrayDeleter<unsigned short> ); //&CArcFitsFile::ArrayDeleter );

		if ( m_pDataBuffer.get() == NULL )
		{
			ThrowException(
				"Read",
				"Failed to allocate buffer for image pixel data" );
		}

		// Read the image data
		// ----------------------------------------------------------
		fits_read_img( m_fptr.get(),
					   TUSHORT,
					   1,
					   lDataLength,
					   NULL,
					   m_pDataBuffer.get(),
					   NULL,
					   &dFitsStatus );

		if ( dFitsStatus )
		{
			ThrowException( "Read", dFitsStatus );
		}
	}
	else
	{
		m_pDataBuffer.reset( new unsigned int[ lDataLength ],
							 &CArcFitsFile::ArrayDeleter<unsigned int> ); //&CArcFitsFile::ArrayDeleter );

		if ( m_pDataBuffer.get() == NULL )
		{
			ThrowException(
				"Read",
				"Failed to allocate buffer for image pixel data" );
		}

		// Read the image data
		// ----------------------------------------------------------
		fits_read_img( m_fptr.get(),
					   TUINT,
					   1,
					   lDataLength,
					   NULL,
					   m_pDataBuffer.get(),
					   NULL,
					   &dFitsStatus );

		if ( dFitsStatus )
		{
			ThrowException( "Read", dFitsStatus );
		}
	}

	return m_pDataBuffer.get();
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
// +---------------------------------------------------------------------------+
void CArcFitsFile::Resize( int dRows, int dCols )
{
	int  dBitsPerPixel = 0;
	int  dFitsStatus   = 0;
	int  dNAxis        = 0;
	long lNAxes[ 2 ]   = { 0, 0 };

	// Verify FITS file handle
	// --------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "Resize",
						"Invalid FITS handle, no file open" );
	}

	// Get the image parameters
	// ----------------------------------------------------------
	fits_get_img_param( m_fptr.get(),
						2,
						&dBitsPerPixel,
						&dNAxis,
						lNAxes,
						&dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "Resize", dFitsStatus );
	}

	// Verify NAXIS parameter
	// ----------------------------------------------------------
	if ( dNAxis != 2 )
	{
		ThrowException( "Resize",
				string( "Invalid NAXIS value. " ) +
				string( "This method is only valid for a " ) +
				string( "file containing a single image." ) );
	}

	// Resize the FITS file
	// ------------------------------------------------------------
	lNAxes[ CArcFitsFile::NAXES_ROW ] = dRows;
	lNAxes[ CArcFitsFile::NAXES_COL ] = dCols;

	fits_resize_img( m_fptr.get(),
					 dBitsPerPixel,
					 dNAxis,
					 lNAxes,
					 &dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "Resize", dFitsStatus );
	}
}

// +---------------------------------------------------------------------------+
// |  Write3D ( Data Cube )                                                    |
// +---------------------------------------------------------------------------+
// |  Add an image to the end of a FITS data cube.                             |
// |                                                                           |
// |  Throws std::runtime_error on error.                                      |
// |                                                                           |
// |  <IN> -> pData - A pointer to the image data.                             |
// +---------------------------------------------------------------------------+
void CArcFitsFile::Write3D( void* pData )
{
	int  dFitsStatus   = 0;	// Initialize status before calling fitsio routines
	int  dBitsPerPixel = 0;
	int  dNAxis        = 0;
	long lNElements    = 0;
	long lNAxes[ 3 ]   = { 1, 1, ++m_lFrame };

	// Verify FITS file handle
	// ----------------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "Write3D",
				"Invalid FITS handle, no file open!" );
	}

	// Get the image parameters
	// ----------------------------------------------------------
	fits_get_img_param( m_fptr.get(),
						3,
						&dBitsPerPixel,
						&dNAxis,
						lNAxes,
						&dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "Write3D", dFitsStatus );
	}

	// Verify parameters
	// ----------------------------------------------------------
	if ( dNAxis != 3 )
	{
		ThrowException( "Write3D",
				string( "Invalid NAXIS value. " ) +
				string( "This method is only valid for a FITS data cube." ) );
	}

	if ( pData == NULL )
	{
		ThrowException( "Write3D", "Invalid data buffer pointer ( Write3D )" );
	}

	// Set number of pixels to write
	// ----------------------------------------------------------
	lNElements = lNAxes[ CArcFitsFile::NAXES_ROW ] * lNAxes[ CArcFitsFile::NAXES_COL ];

	if ( m_lFPixel == 0 )
	{
		m_lFPixel = 1;
	}

	// Write 16-bit data
	// ---------------------------------------------------------------
	if ( dBitsPerPixel == CArcFitsFile::BPP16 )
	{
		fits_write_img( m_fptr.get(),
						TUSHORT,
						m_lFPixel,
						lNElements,
						( unsigned short * )pData,
						&dFitsStatus );
	}

	// Write 32-bit data
	// ---------------------------------------------------------------
	else if ( dBitsPerPixel == CArcFitsFile::BPP32 )
	{
		fits_write_img( m_fptr.get(),
						TUINT,
						m_lFPixel,
						lNElements,
						( unsigned int * )pData,
						&dFitsStatus );
	}

	// Invalid bits-per-pixel value
	// ---------------------------------------------------------------
	else
	{
		ThrowException( "Write3D",
				string( "Invalid dBitsPerPixel, " ) +
				string( "should be 16 ( BPP16 ) or 32 ( BPP32 )." ) );
	}

	if ( dFitsStatus )
	{
		ThrowException( "Write3D", dFitsStatus );
	}

	// Update the start pixel
	// ----------------------------------------------------------
	m_lFPixel += lNElements;

	// Increment the image number and update the key
	// ----------------------------------------------------------
	fits_update_key( m_fptr.get(),
					 TLONG,
					 "NAXIS3",
					 &m_lFrame,
					 NULL,
					 &dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "Write3D", dFitsStatus );
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
// |  <IN> -> data - A pointer to the image data.                              |
// |  <IN> -> dImageNumber - The number of the data cube image to replace.     |
// +---------------------------------------------------------------------------+
void CArcFitsFile::ReWrite3D( void* pData, int dImageNumber )
{
	int  dFitsStatus   = 0;	// Initialize status before calling fitsio routines
	int  dBitsPerPixel = 0;
	int  dNAxis        = 0;
	long lNAxes[ 3 ]   = { 0, 0, 0 };
	long lNElements    = 0;
	long lFPixel       = 0;

	// Verify FITS file handle
	// ----------------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "ReWrite3D",
						"Invalid FITS handle, no file open!" );
	}

	// Get the image parameters
	// ----------------------------------------------------------
	fits_get_img_param( m_fptr.get(),
						3,
						&dBitsPerPixel,
						&dNAxis,
						lNAxes,
						&dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "ReWrite3D", dFitsStatus );
	}

	// Verify parameters
	// ----------------------------------------------------------
	if ( dNAxis != 3 )
	{
		ThrowException( "ReWrite3D",
				string( "Invalid NAXIS value. " ) +
				string( "This method is only valid for a " ) +
				string( "FITS data cube." ) );
	}

	if ( pData == NULL )
	{
		ThrowException( "ReWrite3D", "Invalid data buffer pointer" );
	}

	// Set number of pixels to write; also set the start position
	// ----------------------------------------------------------
	lNElements = lNAxes[ 0 ] * lNAxes[ 1 ];
	lFPixel = lNElements * dImageNumber + 1;

	// Write 16-bit data
	// ---------------------------------------------------------------
	if ( dBitsPerPixel == CArcFitsFile::BPP16 )
	{
		fits_write_img( m_fptr.get(),
						TUSHORT,
						lFPixel,
						lNElements,
						( unsigned short * )pData,
						&dFitsStatus );
	}

	// Write 32-bit data
	// ---------------------------------------------------------------
	else if ( dBitsPerPixel == CArcFitsFile::BPP32 )
	{
		fits_write_img( m_fptr.get(),
						TUINT,
						lFPixel,
						lNElements,
						( unsigned int * )pData,
						&dFitsStatus );
	}

	// Invalid bits-per-pixel value
	// ---------------------------------------------------------------
	else
	{
		ThrowException( "ReWrite3D",
				string( "Invalid dBitsPerPixel, " ) +
				string( "should be 16 ( BPP16 ) or 32 ( BPP32 )." ) );
	}

	if ( dFitsStatus )
	{
		ThrowException( "ReWrite3D", dFitsStatus );
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
// +---------------------------------------------------------------------------+
void *CArcFitsFile::Read3D( int dImageNumber )
{
	int  dFitsStatus   = 0;	// Initialize status before calling fitsio routines
	int  dBitsPerPixel = 0;
	int  dNAxis        = 0;
	long lNAxes[ 3 ]   = { 0, 0, 0 };
	long lNElements    = 0;
	int  dFPixel       = 0;

	// Verify FITS file handle
	// ----------------------------------------------------------
	if ( m_fptr.get() == NULL )
	{
		ThrowException( "Read3D",
						"Invalid FITS handle, no file open!" );
	}

	// Get the image parameters
	// ----------------------------------------------------------
	fits_get_img_param( m_fptr.get(),
						3,
						&dBitsPerPixel,
						&dNAxis,
						lNAxes,
						&dFitsStatus );

	if ( dFitsStatus )
	{
		ThrowException( "Read3D", dFitsStatus );
	}

	// Verify parameters
	// ----------------------------------------------------------
	if ( ( dImageNumber + 1 ) > lNAxes[ 2 ] )
	{
		ostringstream oss;

		oss << "Invalid image number. File contains "
			<< lNAxes[ 2 ] << " images." << ends;

		ThrowException( "Read3D", oss.str() );
	}

	if ( dNAxis != 3 )
	{
		ThrowException( "Read3D",
				string( "Invalid NAXIS value. This " ) +
				string( "method is only valid for a FITS data cube." ) );
	}

	// Set number of pixels to read
	// ----------------------------------------------------------
	lNElements = lNAxes[ 0 ] * lNAxes[ 1 ];
	dFPixel = lNElements * dImageNumber + 1;

	// Read 16-bit data
	// ---------------------------------------------------------------
	if ( dBitsPerPixel == CArcFitsFile::BPP16 )
	{
		m_pDataBuffer.reset( new unsigned short[ lNElements ],
							 &CArcFitsFile::ArrayDeleter<unsigned short> ); //&CArcFitsFile::ArrayDeleter );

		if ( m_pDataBuffer.get() == NULL )
		{
			ThrowException(
				"Read3D",
				"Failed to allocate buffer for image pixel data" );
		}

		// Read the image data
		// ----------------------------------------------------------
		fits_read_img( m_fptr.get(),
					   TUSHORT,
					   dFPixel,
					   lNElements,
					   NULL,
					   m_pDataBuffer.get(),
					   NULL,
					   &dFitsStatus );

		if ( dFitsStatus )
		{
			ThrowException( "Read3D", dFitsStatus );
		}
	}

	// Read 32-bit data
	// ---------------------------------------------------------------
	else if ( dBitsPerPixel == CArcFitsFile::BPP32 )
	{
		m_pDataBuffer.reset( new unsigned int[ lNElements ],
							 &CArcFitsFile::ArrayDeleter<unsigned int> ); //&CArcFitsFile::ArrayDeleter );

		if ( m_pDataBuffer.get() == NULL )
		{
			ThrowException(
				"Read3D",
				"Failed to allocate buffer for image pixel data." );
		}

		// Read the image data
		// ----------------------------------------------------------
		fits_read_img( m_fptr.get(),
					   TUINT,
					   dFPixel,
					   lNElements,
					   NULL,
					   m_pDataBuffer.get(),
					   NULL,
					   &dFitsStatus );

		if ( dFitsStatus )
		{
			ThrowException( "Read3D", dFitsStatus );
		}
	}

	// Invalid bits-per-pixel value
	// ---------------------------------------------------------------
	else
	{
		ThrowException(
			"Read3D",
			"Invalid dBitsPerPixel, should be 16 ( BPP16 ) or 32 ( BPP32 )." );
	}

	return m_pDataBuffer.get();
}

// +---------------------------------------------------------------------------+
// |  GetBaseFile                                                              |
// +---------------------------------------------------------------------------+
// |  Returns the underlying cfitsio file pointer.  INTERNAL USE ONLY.         |
// +---------------------------------------------------------------------------+
fitsfile* CArcFitsFile::GetBaseFile()
{
	return m_fptr.get();
}

// +---------------------------------------------------------------------------+
// |  ThrowException                                                           |
// +---------------------------------------------------------------------------+
// |  Throws a std::runtime_error based on the supplied cfitsio status value.  |
// |                                                                           |
// |  <IN> -> sMethodName : Name of the method where the exception occurred.   |
// |  <IN> -> dFitsStatus : cfitsio error value.                               |
// +---------------------------------------------------------------------------+
void CArcFitsFile::ThrowException( std::string sMethodName, int dFitsStatus )
{
	char szFitsMsg[ 100 ];
	ostringstream oss;

	fits_get_errstatus( dFitsStatus, szFitsMsg );

	oss << "( CArcFitsFile::"
		<< ( sMethodName.empty() ? "???" : sMethodName )
		<< "() ): "
		<< szFitsMsg
		<< ends;

	throw std::runtime_error( ( const std::string )oss.str() );
}

// +---------------------------------------------------------------------------+
// |  ThrowException                                                           |
// +---------------------------------------------------------------------------+
// |  Throws a std::runtime_error based on the supplied cfitsion status value. |
// |                                                                           |
// |  <IN> -> sMethodName : Name of the method where the exception occurred.   |
// |  <IN> -> sMsg        : The exception message.                             |
// +---------------------------------------------------------------------------+
void CArcFitsFile::ThrowException( std::string sMethodName, std::string sMsg )
{
	ostringstream oss;

	oss << "( CArcFitsFile::"
		<< ( sMethodName.empty() ? "???" : sMethodName )
		<< "() ): "
		<< sMsg
		<< ends;

	throw std::runtime_error( ( const std::string )oss.str() );
}



