#ifndef _ARC_TIFFFILE_CAPI_H_
#define _ARC_TIFFFILE_CAPI_H_

#include "tiffio.h"
#include "DllMain.h"


// +------------------------------------------------------------------------------------+
// | Status/Error constants
// +------------------------------------------------------------------------------------+
#ifndef ARC_STATUS
#define ARC_STATUS

	#define ARC_STATUS_OK			0
	#define ARC_STATUS_ERROR		1
	#define ARC_ERROR_MSG_SIZE		128

#endif


#ifdef __cplusplus
   extern "C" {		// Using a C++ compiler
#endif

// +----------------------------------------------------------------------------------+
// |  C Interface - CArcFitsFile Constants                                            |
// +----------------------------------------------------------------------------------+
DLLTIFFFILE_API extern const int TIFF_READMODE;
DLLTIFFFILE_API extern const int TIFF_WRITEMODE;

DLLTIFFFILE_API extern const int TIFF_BPP8;
DLLTIFFFILE_API extern const int TIFF_BPP16;

DLLTIFFFILE_API extern const int TIFF_RGB_SAMPLES_PER_PIXEL;
DLLTIFFFILE_API extern const int TIFF_RGBA_SAMPLES_PER_PIXEL;


// +----------------------------------------------------------------------------------+
// |  C Interface - Constructor/Destructor                                            |
// +----------------------------------------------------------------------------------+
DLLTIFFFILE_API void ArcTiffFile_Create( const char* pszFilename, int* pStatus );
DLLTIFFFILE_API void ArcTiffFile_Open( const char* pszFilename, int dMode, int* pStatus );
DLLTIFFFILE_API void ArcTiffFile_Close();


// +----------------------------------------------------------------------------------+
// |  C Interface - Header methods                                                    |
// +----------------------------------------------------------------------------------+
DLLTIFFFILE_API int ArcTiffFile_GetRows( int* pStatus );
DLLTIFFFILE_API int ArcTiffFile_GetCols( int* pStatus );
DLLTIFFFILE_API int ArcTiffFile_GetBpp( int* pStatus );


// +----------------------------------------------------------------------------------+
// |  C Interface - Read/Write methods                                                |
// +----------------------------------------------------------------------------------+
DLLTIFFFILE_API void  ArcTiffFile_Write( unsigned short* pU16Buf, int dRows, int dCols, int dBpp, int* pStatus );
DLLTIFFFILE_API void* ArcTiffFile_Read( int* pStatus );

DLLTIFFFILE_API const char* ArcTiffFile_GetLastError();

#ifdef __cplusplus
   }
#endif

#endif		// _ARC_TIFFFILE_CAPI_H_
