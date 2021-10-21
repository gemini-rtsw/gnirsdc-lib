#ifndef _ARC_IMAGE_CAPI_H_
#define _ARC_IMAGE_CAPI_H_

#include "DllMain.h"


// +------------------------------------------------------------------------------------+
// | Status/Error constants                                                             |
// +------------------------------------------------------------------------------------+
#ifndef ARC_STATUS
#define ARC_STATUS

	#define ARC_STATUS_OK			0
	#define ARC_STATUS_ERROR		1
	#define ARC_ERROR_MSG_SIZE		128

#endif


// +------------------------------------------------------------------------------------+
// | Image statistics structs                                                           |
// +------------------------------------------------------------------------------------+
#ifndef ARC_IMG_STATS
#define ARC_IMG_STATS

	struct CImgStats
	{
		double gTotalPixels;
		double gMin;
		double gMax;
		double gMean;
		double gVariance;
		double gStdDev;
		double gSaturatedPixCnt;
	};

	struct CImgDifStats
	{
		CImgStats cImg1Stats;
		CImgStats cImg2Stats;
		CImgStats cImgDiffStats;
	};

#endif

#ifdef __cplusplus
   extern "C" {		// Using a C++ compiler
#endif

// +------------------------------------------------------------------------------------+
// |  C Interface - Image bits-per-pixel constants                                      |
// +------------------------------------------------------------------------------------+
DLLARCIMAGE_API extern const int IMAGE_BPP16;
DLLARCIMAGE_API extern const int IMAGE_BPP32;


// +------------------------------------------------------------------------------------+
// |  Used to free ANY and ALL pointers returned by methods                             |
// +------------------------------------------------------------------------------------+
DLLARCIMAGE_API void ArcImage_FreeU8( unsigned char* ptr, int* pStatus );
DLLARCIMAGE_API void ArcImage_FreeFlt( float* ptr, int* pStatus );
DLLARCIMAGE_API void ArcImage_FreeS32( int* ptr, int* pStatus );


// +------------------------------------------------------------------------------------+
// |  Image Data                                                                        |
// +------------------------------------------------------------------------------------+
DLLARCIMAGE_API unsigned char* ArcImage_GetRow( void* pBuf, int dRow, int dCol1, int dCol2, int dRows, int dCols, int* pCount, int dBpp, int* pStatus );
DLLARCIMAGE_API unsigned char* ArcImage_GetCol( void* pBuf, int dCol, int dRow1, int dRow2, int dRows, int dCols, int* pCount, int dBpp, int* pStatus );

DLLARCIMAGE_API float* ArcImage_GetRowArea( void* pBuf, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int* pCount, int dBpp, int* pStatus );
DLLARCIMAGE_API float* ArcImage_GetColArea( void* pBuf, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int* pCount, int dBpp, int* pStatus );


// +------------------------------------------------------------------------------------+
// |  Image Statistics                                                                  |
// +------------------------------------------------------------------------------------+
DLLARCIMAGE_API CImgDifStats ArcImage_GetDiffStats( void* pBuf1, void* pBuf2, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int dBpp, int* pStatus );
DLLARCIMAGE_API CImgDifStats ArcImage_GetImageDiffStats( void* pBuf1, void* pBuf2, int dRows, int dCols, int dBpp, int* pStatus );

DLLARCIMAGE_API CImgStats ArcImage_GetStats( void* pBuf, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int dBpp, int* pStatus );
DLLARCIMAGE_API CImgStats ArcImage_GetImageStats( void* pBuf, int dRows, int dCols, int dBpp, int* pStatus );


// +------------------------------------------------------------------------------------+
// |  Histogram                                                                         |
// +------------------------------------------------------------------------------------+
DLLARCIMAGE_API int* ArcImage_Histogram( int* pCount, void* pBuf, int dRow1, int dRow2, int dCol1, int dCol2, int dRows, int dCols, int dBpp, int* pStatus );
DLLARCIMAGE_API int* ArcImage_ImageHistogram( int* pCount, void* pBuf, int dRows, int dCols, int dBpp, int* pStatus );


// +------------------------------------------------------------------------------------+
// |  Image Buffer Manipulation                                                         |
// +------------------------------------------------------------------------------------+
DLLARCIMAGE_API void ArcImage_Add( unsigned int* pU32Buf, unsigned short* pU16Buf1, unsigned short* pU16Buf2, int dRows, int dCols, int* pStatus );
DLLARCIMAGE_API void ArcImage_Subtract( void* pBuf1, void* pBuf2, int dRows, int dCols, int dBpp, int* pStatus );
DLLARCIMAGE_API void ArcImage_SubtractHalves( void* pBuf, int dRows, int dCols, int dBpp, int* pStatus );
DLLARCIMAGE_API void ArcImage_Divide( void* pBuf1, void* pBuf2, int dRows, int dCols, int dBpp, int* pStatus );


// +------------------------------------------------------------------------------------+
// |  Image Buffer Copy                                                                 |
// +------------------------------------------------------------------------------------+
DLLARCIMAGE_API void ArcImage_Copy( void* pDstBuf, int dDstRows, int dDstCols, void* pSrcBuf, int dSrcRows, int dSrcCols, int dBpp, int* pStatus );
DLLARCIMAGE_API void ArcImage_CopyBySize( void* pDstBuf, int dDstSize, void* pSrcBuf, int dSrcSize, int* pStatus );


// +------------------------------------------------------------------------------------+
// |  Image Buffer Fill                                     |
// +------------------------------------------------------------------------------------+
DLLARCIMAGE_API void ArcImage_Fill( void* pBuf, int dRows, int dCols, int dValue, int dBpp, int* pStatus );
DLLARCIMAGE_API void ArcImage_GradientFill( void* pBuf, int dRows, int dCols, int dBpp, int* pStatus );
DLLARCIMAGE_API void ArcImage_SmileyFill( void* pBuf, int dRows, int dCols, int dBpp, int* pStatus );


// +------------------------------------------------------------------------------------+
// |  VerifyAsSynthetic                                                                 |
// +------------------------------------------------------------------------------------+
DLLARCIMAGE_API void ArcImage_VerifyAsSynthetic( void* pBuf, int dRows, int dCols, int dBpp, int* pStatus );

DLLARCIMAGE_API const char* ArcImage_GetLastError();

#ifdef __cplusplus
   }
#endif

#endif		// _ARC_IMAGE_CAPI_H_
