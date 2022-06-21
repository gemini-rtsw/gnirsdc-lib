#ifndef _CTIFF_FILE_H_
#define _CTIFF_FILE_H_

#ifdef WIN32
#pragma warning( disable: 4251 )
#endif

#include <stdexcept>
#include <memory>
#include <string>
#include "DllMain.h"
#include "tiffio.h"


namespace arc
{
	namespace tiff
	{
		class DLLTIFFFILE_API CArcTiffFile
		{
			public:
				// Read/Write existing file constructor
				// ------------------------------------------------------------------------
				CArcTiffFile( const char* pszFilename, int dMode = WRITEMODE );
				virtual ~CArcTiffFile();

				// Header methods
				// ------------------------------------------------------------------------
				int GetRows();
				int GetCols();
				int GetBpp();

				// Read/Write methods
				// ------------------------------------------------------------------------
				void Write( unsigned short* pU16Buf, int dRows, int dCols, int dBpp = BPP16 );
				void *Read();

				// Constants
				// ------------------------------------------------------------------------
				const static int READMODE  = 0;
				const static int WRITEMODE = 1;

				const static int BPP8  = 8;
				const static int BPP16 = 16;

				const static int RGB_SAMPLES_PER_PIXEL  = 3;
				const static int RGBA_SAMPLES_PER_PIXEL = 4;

			private:
				void WriteU16( unsigned short* pU16Data, int dRows, int dCols );
				void WriteU8( unsigned short* pU16Data, int dRows, int dCols );

				void ReadU16( unsigned short* pU16Buffer );
				void ReadU8( unsigned short* pU16Buffer );

				void ThrowException( std::string sMethodName, std::string sMsg );

				//  Smart pointer array deleter
				// +--------------------------------------------------------------------+
				template<typename T> static void ArrayDeleter( T* p );

				TIFF* m_pTiff;  // Incomplete definition; cannot auto_ptr

				std::shared_ptr<uint16> m_pU16DataBuffer;
		};

	}	// end tiff namespace
}	// end arc namespace

#endif
