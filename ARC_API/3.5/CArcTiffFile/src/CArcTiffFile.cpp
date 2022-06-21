// +-----------------------------------------------------------------------------+
// | CArcTiffFile.cpp : Defines the exported functions for the DLL application   |
// +-----------------------------------------------------------------------------+
// |                                                                             |
// | Notes:                                                                      |
// |                                                                             |
// | Memory Management                                                           |
// | -------------------                                                         |
// | libtiff uses a machine-specific set of routines for managing dynamically    |
// | allocated memory. _TIFFmalloc, _TIFFrealloc, and _TIFFfree mimic the normal |
// | ANSI C routines. Any dynamically allocated memory that is to be passed into |
// | the library should be allocated using these interfaces in order to insure   |
// | pointer compatibility on machines with a segmented architecture. (On 32-bit |
// | UNIX systems these routines just call the normal malloc, realloc, and free  |
// | routines in the C library.)                                                 |
// |                                                                             |
// | To deal with segmented pointer issues libtiff also provides _TIFFmemcpy,    |
// | _TIFFmemset, and _TIFFmemmove routines that mimic the equivalent ANSI C     |
// | routines, but that are intended for use with memory allocated through       |
// | _TIFFmalloc and _TIFFrealloc.                                               |
// +-----------------------------------------------------------------------------+
#include "CArcTiffFile.h"
#include <sstream>

using namespace std;
using namespace arc::tiff;


// +---------------------------------------------------------------------------+
// | ArrayDeleter                                                              |
// +---------------------------------------------------------------------------+
// | Called by std::shared_ptr to delete the temporary image buffer.      |
// | This method should NEVER be called directly by the user.                  |
// +---------------------------------------------------------------------------+
template<typename T> void CArcTiffFile::ArrayDeleter( T* p )
{
	if ( p != NULL )
	{
		delete [] p;
	}
}

// +---------------------------------------------------------------------------+
// |  Constructor for READING/WRITING a TIFF file.                             |
// +---------------------------------------------------------------------------+
// |  Opens an existing or creates a new TIFF file depending on the mode       |
// |  parameter. The default mode is to create ( WRITEMODE ).                  |
// |                                                                           |
// |  Throws std::runtime_error on parameter or libtiff library error.         |
// |                                                                           |
// |  <IN> -> pszFilename - The file to open, including path.                  |
// |  <IN> -> dMode       - Read/Write mode, default = CArcTiffFile::WRITEMODE |
// +---------------------------------------------------------------------------+
CArcTiffFile::CArcTiffFile( const char* pszFilename, int dMode )
{
	string sFilename( pszFilename );
	string sMode( "w" );

	// Verify filename and make sure the kernel image
	// buffer has been initialized.
	// --------------------------------------------------
	if ( sFilename.empty() )
	{
		ThrowException( "CArcTiffFile", "Invalid file name : " + sFilename );
	}

	m_pTiff = NULL;

	if ( dMode == CArcTiffFile::READMODE )
	{
		sMode = "r";
	}

	// Open the TIFF file
	// --------------------------------------------------
	m_pTiff = TIFFOpen( sFilename.c_str(), sMode.c_str() );

	if ( m_pTiff == NULL )
	{
		ThrowException( "CArcTiffFile", "Failed to create " + sFilename );
	}

	m_pU16DataBuffer.reset();
}

// +---------------------------------------------------------------------------+
// |  Class destructor                                                         |
// +---------------------------------------------------------------------------+
// |  Destroys the class. Deallocates any data buffers. Closes any open TIFF   |
// |  pointers.                                                                |
// +---------------------------------------------------------------------------+
CArcTiffFile::~CArcTiffFile()
{
	if ( m_pTiff != NULL )
	{
		TIFFClose( m_pTiff );
	}

	m_pTiff = NULL;
}

// +---------------------------------------------------------------------------+
// |  GetRows                                                                  |
// +---------------------------------------------------------------------------+
// |  Returns the row dimension of the TIFF file currently open.               |
// |                                                                           |
// |  <RET> -> Number of rows in TIFF image.                                   |
// +---------------------------------------------------------------------------+
int CArcTiffFile::GetRows()
{
	int dRows = 0;

	if ( m_pTiff == NULL )
	{
		ThrowException( "GetRows", "Invalid TIFF handle, no file open!" );
	}

	TIFFGetField( m_pTiff, TIFFTAG_IMAGEWIDTH, &dRows );

	return dRows;
}

// +---------------------------------------------------------------------------+
// |  GetCols                                                                  |
// +---------------------------------------------------------------------------+
// |  Returns the column dimension of the TIFF file currently open.            |
// |                                                                           |
// |  <RET> -> Number of columns in TIFF image.                                |
// +---------------------------------------------------------------------------+
int CArcTiffFile::GetCols()
{
	int dCols = 0;

	if ( m_pTiff == NULL )
	{
		ThrowException( "GetCols", "Invalid TIFF handle, no file open!" );
	}

	TIFFGetField( m_pTiff, TIFFTAG_IMAGELENGTH, &dCols );

	return dCols;
}

// +---------------------------------------------------------------------------+
// |  GetBpp                                                                   |
// +---------------------------------------------------------------------------+
// |  Returns the bits-per-pixel ( bits-per-sample ) of the TIFF file          |
// |  currently open.                                                          |
// |                                                                           |
// |  <RET> -> bits-per-pixel ( bits-per-sample ) in TIFF image.               |
// +---------------------------------------------------------------------------+
int CArcTiffFile::GetBpp()
{
	uint32 dBpp = 0;

	if ( m_pTiff == NULL )
	{
		ThrowException( "GetBpp", "Invalid TIFF handle, no file open!" );
	}

	TIFFGetField( m_pTiff, TIFFTAG_BITSPERSAMPLE, &dBpp );

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
// |  <IN> -> pU16Buf - Pointer to 16-bit image data.                          |
// |  <IN> -> dRows   - Number of rows in image data.                          |
// |  <IN> -> dCols   - Number of cols in image data.                          |
// |  <IN> -> dBpp    - The bits-per-pixel(/sample), default = 16.             |
// +---------------------------------------------------------------------------+
void CArcTiffFile::Write( unsigned short* pU16Buf, int dRows, int dCols, int dBpp )
{
	if ( dBpp == CArcTiffFile::BPP16 )
	{
		WriteU16( pU16Buf, dRows, dCols );
	}

	else if ( dBpp == CArcTiffFile::BPP8 )
	{
		WriteU8( pU16Buf, dRows, dCols );
	}

	else
	{
		ThrowException( "Write", "Invalid bits-per-pixel, must be 8 or 16!" );
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
// +---------------------------------------------------------------------------+
void *CArcTiffFile::Read()
{
	//  Verify TIFF file is open
	// --------------------------------------------------------------------
	if ( m_pTiff == NULL )
	{
		ThrowException( "Read", "Invalid TIFF handle, no file open!" );
	}

	int dRows = GetRows();
	int dCols = GetCols();
	int dBpp  = GetBpp();

	//  Verify the image dimensions
	// --------------------------------------------------------------------
	if ( dRows <= 0 || dCols <= 0 )
	{
		ostringstream oss;

		oss << "Invalid TIFF image dimensions, rows: "
			<< dRows << " cols: " << dCols;

		ThrowException( "Read", oss.str() );
	}

	//  Verify the bits-per-pixel ( bits-per-sample ), must be 8 0r 16
	// --------------------------------------------------------------------
	if ( dBpp != CArcTiffFile::BPP16 && dBpp != CArcTiffFile::BPP8 )
	{
		ostringstream oss;

		oss << "Invalid TIFF bits-per-pixel: " << dBpp
			<< "! Must be 8 or 16!" << ends;

		ThrowException( "Read", oss.str() );
	}

	//  Verify the samples-per-pixel is three, only RGB is supported
	// --------------------------------------------------------------------
	uint32 dSamplesPerPixel = 0;

	TIFFGetField( m_pTiff, TIFFTAG_SAMPLESPERPIXEL, &dSamplesPerPixel );

	if ( int( dSamplesPerPixel ) != CArcTiffFile::RGB_SAMPLES_PER_PIXEL )
	{
		ostringstream oss;

		oss << "Invalid TIFF samples-per-pixel: " << dSamplesPerPixel
			<< "! Only RGB samples-per-pixel of three is supported!"
			<< ends;

		ThrowException( "Read", oss.str() );
	}

	//  Allocate 16-bit buffer
	// --------------------------------------------------------------------
	m_pU16DataBuffer.reset( new uint16[ dRows * dCols ], &CArcTiffFile::ArrayDeleter<uint16> );

	if ( m_pU16DataBuffer.get() == NULL )
	{
		ThrowException( "Read", "Failed to allocate TIFF data buffer!" );
	}

	if ( dBpp == CArcTiffFile::BPP16 )
	{
		ReadU16( m_pU16DataBuffer.get() );
	}
	else
	{
		ReadU8( m_pU16DataBuffer.get() );
	}

	return reinterpret_cast<void *>( m_pU16DataBuffer.get() );
}

// +---------------------------------------------------------------------------+
// |  WriteU16                                                                 |
// +---------------------------------------------------------------------------+
// |  Writes 16-bit grayscale image to a 16-bit TIFF file.                     |
// |                                                                           |
// |  Throws std::runtime_error on parameter error or libtiff library error.   |
// |                                                                           |
// |  <IN> -> pU16Data - Pointer to 16-bit image data.                         |
// |  <IN> -> dRows    - Number of rows in image data.                         |
// |  <IN> -> dCols    - Number of cols in image data.                         |
// +---------------------------------------------------------------------------+
void CArcTiffFile::WriteU16( unsigned short* pU16Data, int dRows, int dCols )
{
	if ( m_pTiff == NULL )
	{
		ThrowException( "WriteU16", "Invalid TIFF handle, no file open!" );
	}

	if ( pU16Data == NULL )
	{
		ThrowException( "WriteU16", "Invalid pixel data, NULL pointer!" );
	}

	tsize_t tTotalDataSize =
			( dCols * dRows * CArcTiffFile::RGB_SAMPLES_PER_PIXEL * sizeof( uint16 ) );


	//  Write TIFF header tags
	// --------------------------------------------------
	TIFFSetField( m_pTiff, TIFFTAG_IMAGEWIDTH,      dCols );
	TIFFSetField( m_pTiff, TIFFTAG_IMAGELENGTH,     dRows );
	TIFFSetField( m_pTiff, TIFFTAG_BITSPERSAMPLE,   CArcTiffFile::BPP16 );
	TIFFSetField( m_pTiff, TIFFTAG_SAMPLESPERPIXEL, CArcTiffFile::RGB_SAMPLES_PER_PIXEL );
	TIFFSetField( m_pTiff, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG );
	TIFFSetField( m_pTiff, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_RGB );
	TIFFSetField( m_pTiff, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT );    // set the origin of the image

	//  We set the strip size of the file to be size of one row of pixels
	// ------------------------------------------------------------------------------------
	TIFFSetField(
			m_pTiff,
			TIFFTAG_ROWSPERSTRIP,
			TIFFDefaultStripSize( m_pTiff,
								  dCols * CArcTiffFile::RGB_SAMPLES_PER_PIXEL ) );

	//  Copy the image data to libtiff buffer. Must be this way or it won't work!
	// ---------------------------------------------------------------------------
	uint16* pU16TiffBuf = ( uint16 * )_TIFFmalloc( tTotalDataSize );

	for ( int i=0, pos=0; i<(dRows * dCols); i++, pos+=CArcTiffFile::RGB_SAMPLES_PER_PIXEL )
	{
		pU16TiffBuf[ pos + 0 ] = ( ( uint16 * )pU16Data )[ i ];
		pU16TiffBuf[ pos + 1 ] = ( ( uint16 * )pU16Data )[ i ];
		pU16TiffBuf[ pos + 2 ] = ( ( uint16 * )pU16Data )[ i ];
	}

	// Write the image to the file
	// --------------------------------------------------
	TIFFWriteEncodedStrip( m_pTiff, 0, ( uint8 * )pU16TiffBuf, tTotalDataSize );

	//  The following would work too ....
	// --------------------------------------------------
	//for ( uint32 row = 0; row<uint32( dRows ); row++ )
	//{
	//	if ( TIFFWriteScanline( m_pTiff, &pU16TiffBuf[ row * dCols ], row, 0 ) < 0 )
	//	{
			//ostringstream oss; 
			//oss << "Failed to write row #" << y;

			//ThrowException( "Write", oss.str() );
	//	}
	//}

	_TIFFfree( pU16TiffBuf );
}

// +---------------------------------------------------------------------------+
// |  WriteU8                                                                  |
// +---------------------------------------------------------------------------+
// |  Writes 16-bit grayscale image to an 8-bit TIFF file.                     |
// |                                                                           |
// |  Throws std::runtime_error on parameter error or libtiff library error.   |
// |                                                                           |
// |  <IN> -> pU16Data - Pointer to 16-bit image data.                         |
// |  <IN> -> dRows    - Number of rows in image data.                         |
// |  <IN> -> dCols    - Number of cols in image data.                         |
// +---------------------------------------------------------------------------+
void CArcTiffFile::WriteU8( unsigned short* pU16Data, int dRows, int dCols )
{
	if ( m_pTiff == NULL )
	{
		ThrowException( "WriteU8", "Invalid TIFF handle, no file open!" );
	}

	if ( pU16Data == NULL )
	{
		ThrowException( "WriteU8", "Invalid pixel data, NULL pointer!" );
	}

	tsize_t tTotalDataSize = ( dCols * dRows * CArcTiffFile::RGB_SAMPLES_PER_PIXEL );


	//  Write TIFF header tags
	// --------------------------------------------------
	TIFFSetField( m_pTiff, TIFFTAG_IMAGEWIDTH,      dCols );
	TIFFSetField( m_pTiff, TIFFTAG_IMAGELENGTH,     dRows );
	TIFFSetField( m_pTiff, TIFFTAG_BITSPERSAMPLE,   CArcTiffFile::BPP8 );
	TIFFSetField( m_pTiff, TIFFTAG_SAMPLESPERPIXEL, CArcTiffFile::RGB_SAMPLES_PER_PIXEL );
	TIFFSetField( m_pTiff, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG );
	TIFFSetField( m_pTiff, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_RGB );
	TIFFSetField( m_pTiff, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT );    // set the origin of the image

	//  We set the strip size of the file to be size of one row of pixels
	// ------------------------------------------------------------------------------------
	TIFFSetField(
			m_pTiff,
			TIFFTAG_ROWSPERSTRIP,
			TIFFDefaultStripSize( m_pTiff,
								  dCols * CArcTiffFile::RGB_SAMPLES_PER_PIXEL ) );

	//  Copy the image data to libtiff buffer. Must be this way or it won't work!
	// ---------------------------------------------------------------------------
	uint8* pU8TiffBuf = ( uint8 * )_TIFFmalloc( tTotalDataSize );
	uint8  u8TiffData = 0;

	for ( int i=0, pos=0; i<(dRows * dCols); i++, pos+=CArcTiffFile::RGB_SAMPLES_PER_PIXEL )
	{
		u8TiffData = ( ( ( uint16 * )pU16Data )[ i ] / 256 );

		pU8TiffBuf[ pos + 0 ] = u8TiffData;		// red
		pU8TiffBuf[ pos + 1 ] = u8TiffData;		// green
		pU8TiffBuf[ pos + 2 ] = u8TiffData;		// blue
	}

	// Write the image to the file
	// --------------------------------------------------
	TIFFWriteEncodedStrip( m_pTiff, 0, ( uint8 * )pU8TiffBuf, tTotalDataSize );


	//  The following would work too ....
	// --------------------------------------------------
	//for ( uint32 row = 0; row<uint32( dRows ); row++ )
	//{
	//	if ( TIFFWriteScanline( m_pTiff, &pU8TiffBuf[ row * dCols ], row, 0 ) < 0 )
	//	{
	//		ostringstream oss; 
	//		oss << "Failed to write row #" << row << ends;

	//		ThrowException( "Write", oss.str() );
	//	}
	//}

	_TIFFfree( pU8TiffBuf );
}

// +---------------------------------------------------------------------------+
// |  ReadU16                                                                  |
// +---------------------------------------------------------------------------+
// |  Reads 16-bit grayscale image from a 16-bit TIFF file.                    |
// |                                                                           |
// |  Throws std::runtime_error on parameter error or libtiff library error.   |
// |                                                                           |
// |  <IN> -> pU16Buffer - Pointer to 16-bit image data buffer.                |
// +---------------------------------------------------------------------------+
void CArcTiffFile::ReadU16( unsigned short* pU16Buffer )
{
	if ( m_pTiff == NULL )
	{
		ThrowException( "ReadU16", "Invalid TIFF handle, no file open!" );
	}

	//  Verify 16-bit output buffer
	// --------------------------------------------------------------------
	if ( pU16Buffer == NULL )
	{
		ThrowException( "ReadU16", "Failed to allocate TIFF data buffer!" );
	}

	//  Calculate size of 16-bit RGB TIFF buffer
	// --------------------------------------------------------------------
	tsize_t tTiffDataSize = ( GetRows() * GetCols() * GetBpp() * sizeof( uint16 ) );

	//  Allocate 16-bit TIFF buffer
	// --------------------------------------------------------------------
	uint16* pU16TiffData = ( uint16 * )_TIFFmalloc( tTiffDataSize );

	//  Read the image data from the TIFF
	// --------------------------------------------------------------------
	TIFFReadEncodedStrip( m_pTiff, 0, ( uint8 * )pU16TiffData, tTiffDataSize );

	//  Convert the 16-bit RGB TIFF data to single 16-bit buffer
	// --------------------------------------------------------------------
	for ( int i=0, j=0; i<( GetRows() * GetCols() ); i++, j+=CArcTiffFile::RGB_SAMPLES_PER_PIXEL )
	{
		( ( uint16 * )pU16Buffer )[ i ] = pU16TiffData[ j ];
	}

	//  Free the 16-bit RGB TIFF buffer
	// --------------------------------------------------------------------
	_TIFFfree( pU16TiffData );
}

// +---------------------------------------------------------------------------+
// |  ReadU8                                                                   |
// +---------------------------------------------------------------------------+
// |  Reads 16-bit grayscale image from a 8-bit TIFF file. Each pixel from     |
// |  TIFF file is multiplied by 256 to convert to 16-bit image data.          |
// |                                                                           |
// |  Throws std::runtime_error on parameter error or libtiff library error.   |
// |                                                                           |
// |  <IN> -> pU16Buffer - Pointer to 16-bit image data buffer.                |
// +---------------------------------------------------------------------------+
void CArcTiffFile::ReadU8( unsigned short* pU16Buffer )
{
	if ( m_pTiff == NULL )
	{
		ThrowException( "ReadU8", "Invalid TIFF handle, no file open!" );
	}

	//  Verify 16-bit output buffer
	// --------------------------------------------------------------------
	if ( pU16Buffer == NULL )
	{
		ThrowException( "ReadU8", "Failed to allocate TIFF data buffer!" );
	}

	//  Calculate size of 8-bit RGB TIFF buffer
	// --------------------------------------------------------------------
	tsize_t tTiffDataSize = ( GetRows() * GetCols() * GetBpp() );

	//  Allocate 8-bit TIFF buffer
	// --------------------------------------------------------------------
	uint8* pU8TiffData = ( uint8 * )_TIFFmalloc( tTiffDataSize );

	//  Read the image data from the TIFF
	// --------------------------------------------------------------------
	TIFFReadEncodedStrip( m_pTiff, 0, ( uint8 * )pU8TiffData, tTiffDataSize );

	//  Convert the 8-bit RGB TIFF data to single 16-bit buffer
	// --------------------------------------------------------------------
	for ( int i=0, j=0; i<( GetRows() * GetCols() ); i++, j+=CArcTiffFile::RGB_SAMPLES_PER_PIXEL )
	{
		( ( uint16 * )pU16Buffer )[ i ] = pU8TiffData[ j ] * 256;
	}

	//  Free the 16-bit RGB TIFF buffer
	// --------------------------------------------------------------------
	_TIFFfree( pU8TiffData );
}

// +----------------------------------------------------------------------------+
// |  ThrowException                                                            |
// +----------------------------------------------------------------------------+
// |  Method to throw a "const char *" exception.                               |
// |                                                                            |
// |  <IN> -> sMethodName : The method where the exception occurred.            |
// |  <IN> -> sMsg        : A std::string to use for the exception message.     |
// +----------------------------------------------------------------------------+
void CArcTiffFile::ThrowException( std::string sMethodName, std::string sMsg )
{
	ostringstream oss;

	oss << "( CArcTiffFile::"
		<< ( sMethodName.empty() ? "???" : sMethodName )
		<< "() ): "
		<< sMsg
		<< ends;

	throw std::runtime_error( ( const std::string )oss.str() );
}

