// +-------------------------------------------------------------------------------+
// |  CArcFitsFile.h                                                               |
// +-------------------------------------------------------------------------------+
// |  Defines the exported functions for the CArcFitsFile DLL.  Wraps the cfitsio  |
// |  library for convenience and for use with Owl.                                |
// |                                                                               |
// |  Scott Streit                                                                 |
// |  Astronomical Research Cameras, Inc.                                          |
// +-------------------------------------------------------------------------------+
#ifndef _ARC_CFITS_FILE_H_
#define _ARC_CFITS_FILE_H_

#ifdef WIN32
#pragma warning( disable: 4251 )
#endif

#include <stdexcept>
#include <memory>
#include <string>
#include <cfitsio/fitsio.h>
#include "DllMain.h"


namespace arc
{
	namespace fits
	{
		#define T_SIZE( t )	pow( ( double )2, ( double )( sizeof( t ) * 8 ) )


//		class DLLFITSFILE_API CArcFitsFile
		class CArcFitsFile
		{
			public:
				//  Copy constructor & destructor
				// +------------------------------------------------------------------------+
				CArcFitsFile( CArcFitsFile& rAnotherFitsFile, const char* pszNewFileName = "Image.fit" );

				virtual ~CArcFitsFile();

				//  Read/Write existing file constructor
				// +------------------------------------------------------------------------+
				CArcFitsFile( const char* pszFilename, int dMode = READMODE );

				//  Create new file constructor
				// +------------------------------------------------------------------------+
				CArcFitsFile( const char* pszFilename, int dRows, int dCols, int dBitsPerPixel = BPP16, bool bIs3D = false );

				//  General methods
				// +------------------------------------------------------------------------+
				const std::string	GetFilename();
				std::string*		GetHeader( int* pKeyCount );
				void				WriteKeyword( char* pszKey, void* pKeyVal, int dValType, char* pszComment );
				void				UpdateKeyword( char* pszKey, void* pKeyVal, int dValType, char* pszComment );
				void				GetParameters( long* pNaxes, int* pNaxis = NULL, int* pBitsPerPixel = NULL );
				long				GetNumberOfFrames();
				long				GetRows();
				long				GetCols();
				int					GetNAxis();
				int					GetBpp();
				void				GenerateTestData();
				void				ReOpen();

				//  Single image methods
				// +------------------------------------------------------------------------+
				bool  Compare( CArcFitsFile& anotherCFitsFile );
				void  Resize( int dRows, int dCols );
				void  Write( void* pData );
				void  Write( void* pData, unsigned int uBytesToWrite, int fPixel = -1 );
				void  WriteSubImage( void* pData, int llrow, int llcol, int urrow, int urcol );
				void* ReadSubImage( int llrow, int llcol, int urrow, int urcol );
				void* Read();

				//  Data cube methods
				// +------------------------------------------------------------------------+
				void  Write3D( void* pData );
				void  ReWrite3D( void* pData, int dImageNumber );
				void* Read3D( int dImageNumber );

				//  cfitsio file access
				// +------------------------------------------------------------------------+
				fitsfile* GetBaseFile();

				//  Constants
				// +------------------------------------------------------------------------+
				const static int READMODE       = READONLY;
				const static int READWRITEMODE  = READWRITE;

				const static int BPP16          = 16;
				const static int BPP32          = 32;

				const static int NAXES_COL		= 0;
				const static int NAXES_ROW		= 1;
				const static int NAXES_NOF		= 2;
				const static int NAXES_SIZE		= 3;

				const static int FITS_STRING_KEY	= 0;
				const static int FITS_INTEGER_KEY	= 1;
				const static int FITS_DOUBLE_KEY	= 2;
				const static int FITS_LOGICAL_KEY	= 3;
				const static int FITS_COMMENT_KEY	= 4;
				const static int FITS_HISTORY_KEY	= 5;
				const static int FITS_DATE_KEY		= 6;

			private:
				void ThrowException( std::string sMethodName, int dFitsStatus );
				void ThrowException( std::string sMethodName, std::string sMsg );

				//  Smart pointer array deleter
				// +--------------------------------------------------------------------+
				template<typename T> static void ArrayDeleter( T* p );

				std::shared_ptr<std::string>	m_pDataHeader;
				std::shared_ptr<void>			m_pDataBuffer;
				std::auto_ptr<fitsfile>			m_fptr;
				long							m_lFPixel;
				long							m_lFrame;
		};

	}	// end fits namespace
}	// end arc namespace

#endif
