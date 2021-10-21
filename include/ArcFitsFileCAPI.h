#ifndef _ARC_FITSFILE_CAPI_H_
#define _ARC_FITSFILE_CAPI_H_

#include <cfitsio/fitsio.h>
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
DLLFITSFILE_API extern const int FITS_READMODE;
DLLFITSFILE_API extern const int FITS_READWRITEMODE;

DLLFITSFILE_API extern const int FITS_BPP16;
DLLFITSFILE_API extern const int FITS_BPP32;

DLLFITSFILE_API extern const int FITS_NAXES_COL;
DLLFITSFILE_API extern const int FITS_NAXES_ROW;
DLLFITSFILE_API extern const int FITS_NAXES_NOF;
DLLFITSFILE_API extern const int FITS_NAXES_SIZE;

DLLFITSFILE_API extern const int FITS_STRING_KEY;
DLLFITSFILE_API extern const int FITS_INTEGER_KEY;
DLLFITSFILE_API extern const int FITS_DOUBLE_KEY;
DLLFITSFILE_API extern const int FITS_LOGICAL_KEY;
DLLFITSFILE_API extern const int FITS_COMMENT_KEY;
DLLFITSFILE_API extern const int FITS_HISTORY_KEY;
DLLFITSFILE_API extern const int FITS_DATE_KEY;


// +----------------------------------------------------------------------------------+
// |  C Interface - Deinterlace Functions                                             |
// +----------------------------------------------------------------------------------+
DLLFITSFILE_API void ArcFitsFile_Create( const char* pszFilename, int dRows, int dCols, int dBpp, int dIs3D, int* pStatus );
DLLFITSFILE_API void ArcFitsFile_Open( const char* pszFilename, int dMode, int* pStatus );
DLLFITSFILE_API	void ArcFitsFile_Close();

DLLFITSFILE_API const char*  ArcFitsFile_GetFilename( int* pStatus );
DLLFITSFILE_API const char** ArcFitsFile_GetHeader( int* pKeyCount, int* pStatus );
DLLFITSFILE_API void         ArcFitsFile_FreeHeader();

DLLFITSFILE_API void ArcFitsFile_WriteKeyword( char* szKey, void* pKeyVal, int dValType, char* szComment, int* pStatus );
DLLFITSFILE_API void ArcFitsFile_UpdateKeyword( char* szKey, void* pKeyVal, int dValType, char* szComment, int* pStatus );
DLLFITSFILE_API void ArcFitsFile_GetParameters( long* pNaxes, int* pNaxis, int* pBitsPerPixel, int* pStatus );
DLLFITSFILE_API long ArcFitsFile_GetNumberOfFrames( int* pStatus );
DLLFITSFILE_API long ArcFitsFile_GetRows( int* pStatus );
DLLFITSFILE_API long ArcFitsFile_GetCols( int* pStatus );
DLLFITSFILE_API int	 ArcFitsFile_GetNAxis( int* pStatus );
DLLFITSFILE_API int	 ArcFitsFile_GetBpp( int* pStatus );
DLLFITSFILE_API void ArcFitsFile_GenerateTestData( int* pStatus );
DLLFITSFILE_API void ArcFitsFile_ReOpen( int* pStatus );

DLLFITSFILE_API void  ArcFitsFile_Resize( int dRows, int dCols, int* pStatus );
DLLFITSFILE_API void  ArcFitsFile_Write( void* pData, int* pStatus );
DLLFITSFILE_API void  ArcFitsFile_WriteToOffset( void* pData, unsigned int bytesToWrite, int fPixl, int* pStatus );
DLLFITSFILE_API void  ArcFitsFile_WriteSubImage( void* pData, int llrow, int llcol, int urrow, int urcol, int* pStatus );
DLLFITSFILE_API void* ArcFitsFile_ReadSubImage( int llrow, int llcol, int urrow, int urcol, int* pStatus );
DLLFITSFILE_API void* ArcFitsFile_Read( int* pStatus );

DLLFITSFILE_API void  ArcFitsFile_Write3D( void* pData, int* pStatus );
DLLFITSFILE_API void  ArcFitsFile_ReWrite3D( void* pData, int dImageNumber, int* pStatus );
DLLFITSFILE_API void* ArcFitsFile_Read3D( int dImageNumber, int* pStatus );

DLLFITSFILE_API fitsfile* ArcFitsFile_GetBaseFile( int* pStatus );

DLLFITSFILE_API const char* ArcFitsFile_GetLastError();

#ifdef __cplusplus
   }
#endif

#endif		// _ARC_FITSFILE_CAPI_H_
