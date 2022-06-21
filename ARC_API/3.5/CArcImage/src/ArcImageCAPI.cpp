// ArcImageCAPI.cpp : Defines the exported functions for the DLL application.
//
#include <stdexcept>
#include <cstdio>
#include "DllMain.h"
#include "ArcImageCAPI.h"
#include "CArcImage.h"

using namespace std;
using namespace arc::image;



// +------------------------------------------------------------------------------------+
// | Deinterlace constants
// +------------------------------------------------------------------------------------+
const int IMAGE_BPP16	= CArcImage::BPP16;
const int IMAGE_BPP32	= CArcImage::BPP32;


// +------------------------------------------------------------------------------------+
// | Globals
// +------------------------------------------------------------------------------------+
static auto_ptr<CArcImage> g_pCImage( 0 );
static char g_szErrMsg[ ARC_ERROR_MSG_SIZE ];


// +------------------------------------------------------------------------------------+
// | Define system dependent macros
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
// | Verify class pointer macro
// +------------------------------------------------------------------------------------+
#define VERIFY_CLASS_PTR( fnc, ptr )											\
						if ( ptr.get() == NULL )								\
						{														\
							ptr.reset( new CArcImage() );						\
																				\
							if ( ptr.get() == NULL )							\
							{													\
								throw runtime_error( string( "( " ) + fnc +		\
								string( " ): Invalid class pointer!" ) );		\
							}													\
						}


// +----------------------------------------------------------------------------
// |  FreeU8
// +----------------------------------------------------------------------------
// |  Frees data, such as a row or column, that was passed to the caller. This
// |  method MUST be called by the user to free the returned data buffer.
// |
// |  <IN>  -> ptr     : Pointer to free
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_FreeU8( unsigned char* ptr, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_FreeU8", g_pCImage )

		g_pCImage.get()->Free( ptr );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  FreeFlt
// +----------------------------------------------------------------------------
// |  Frees data, such as a row or column, that was passed to the caller. This
// |  method MUST be called by the user to free the returned data buffer.
// |
// |  <IN>  -> ptr     : Pointer to free
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_FreeFlt( float* ptr, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_FreeFlt", g_pCImage )

		g_pCImage.get()->Free( ptr );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  FreeS32
// +----------------------------------------------------------------------------
// |  Frees data, such as a row or column, that was passed to the caller. This
// |  method MUST be called by the user to free the returned data buffer.
// |
// |  <IN>  -> ptr     : Pointer to free
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_FreeS32( int* ptr, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_FreeS32", g_pCImage )

		g_pCImage.get()->Free( ptr );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  GetRow
// +----------------------------------------------------------------------------
// |  Returns a row of pixel data from the specified image buffer. All or part
// |  of the row can be returned.
// |
// |  <IN> -> pBuf     : Pointer to image buffer
// |  <IN> -> dRow     : The row to return
// |  <IN> -> dCol1    : Start column 
// |  <IN> -> dCol2    : End column
// |  <IN> -> dRows    : Number of rows in full image
// |  <IN> -> dCols    : Number of columns in full image
// |  <IN> -> rdCount  : The number of elements in the return buffer
// |  <IN> -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// |
// |  Returns: Pointer to the row data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CArcImage::Free().
// +----------------------------------------------------------------------------
DLLARCIMAGE_API unsigned char* ArcImage_GetRow( void* pBuf, int dRow, int dCol1,
									   int dCol2, int dRows, int dCols, int* pCount,
									   int dBpp, int* pStatus )
{
	unsigned char* pU8Row = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_GetRow", g_pCImage )

		pU8Row = g_pCImage.get()->GetRow( pBuf,
										  dRow,
										  dCol1,
										  dCol2,
										  dRows,
										  dCols,
										  *pCount,
										  dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pU8Row;
}


// +----------------------------------------------------------------------------
// |  GetCol
// +----------------------------------------------------------------------------
// |  Returns a column of pixel data from the specified image buffer. All or
// |  part of the column can be returned.
// |
// |  <IN> -> pBuf     : Pointer to image buffer
// |  <IN> -> dCol     : The column to return
// |  <IN> -> dRow1    : Start row 
// |  <IN> -> dRow2    : End row
// |  <IN> -> dRows    : Number of rows in full image
// |  <IN> -> dCols    : Number of columns in full image
// |  <IN> -> rdCount  : The number of elements in the return buffer
// |  <IN> -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// |
// |  Returns: Pointer to the column data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CArcImage::Free().
// +----------------------------------------------------------------------------
DLLARCIMAGE_API unsigned char* ArcImage_GetCol( void* pBuf, int dCol, int dRow1,
									   int dRow2, int dRows, int dCols, int* pCount,
									   int dBpp, int* pStatus )
{
	unsigned char* pU8Col = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_GetCol", g_pCImage )

		pU8Col = g_pCImage.get()->GetCol( pBuf,
										  dCol,
										  dRow1,
										  dRow2,
										  dRows,
										  dCols,
										  *pCount,
										  dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pU8Col;
}


// +----------------------------------------------------------------------------
// |  GetRowArea
// +----------------------------------------------------------------------------
// |  Returns a row of pixel data where each value is the average over the
// |  specified range of columns.
// |
// |  <IN> -> pBuf     : Pointer to image buffer
// |  <IN> -> dRow1    : Start row 
// |  <IN> -> dRow2    : End row
// |  <IN> -> dCol1    : Start column 
// |  <IN> -> dCol2    : End column
// |  <IN> -> dRows    : Number of rows in full image
// |  <IN> -> dCols    : Number of columns in full image
// |  <IN> -> rdCount  : The number of elements in the return buffer
// |  <IN> -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// |
// |  Returns: Pointer to the row data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CArcImage::Free().
// +----------------------------------------------------------------------------
DLLARCIMAGE_API float* ArcImage_GetRowArea( void* pBuf, int dRow1, int dRow2, int dCol1,
										    int dCol2, int dRows, int dCols, int* pCount,
											int dBpp, int* pStatus )
{
	float* pRowArea = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_GetRowArea", g_pCImage )

		pRowArea = g_pCImage.get()->GetRowArea( pBuf,
												dRow1,
												dRow2,
												dCol1,
												dCol2,
												dRows,
												dCols,
												*pCount,
												dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pRowArea;
}


// +----------------------------------------------------------------------------
// |  GetColArea
// +----------------------------------------------------------------------------
// |  Returns a column of pixel data where each value is the average over the
// |  specified range of rows.
// |
// |  <IN> -> pBuf     : Pointer to image memory
// |  <IN> -> dRow1    : Start row 
// |  <IN> -> dRow2    : End row
// |  <IN> -> dCol1    : Start column 
// |  <IN> -> dCol2    : End column
// |  <IN> -> dRows	   : Number of rows in full image
// |  <IN> -> dCols    : Number of columns in full image
// |  <IN> -> rdCount  : The number of elements in the return buffer
// |  <IN> -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// |
// |  Returns: Pointer to the column data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CArcImage::Free().
// +----------------------------------------------------------------------------
DLLARCIMAGE_API float* ArcImage_GetColArea( void* pBuf, int dRow1, int dRow2, int dCol1,
										    int dCol2, int dRows, int dCols, int* pCount,
											int dBpp, int* pStatus )
{
	float* pColArea = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_GetColArea", g_pCImage )

		pColArea = g_pCImage.get()->GetColArea( pBuf,
												dRow1,
												dRow2,
												dCol1,
												dCol2,
												dRows,
												dCols,
												*pCount,
												dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pColArea;
}


// +----------------------------------------------------------------------------
// |  GetDiffStats
// +----------------------------------------------------------------------------
// |  Calculates the min, max, mean, variance, standard deviation for each image
// |  as well as the difference mean, variance and standard deviation over the
// |  specified image memory rows and cols. This is used for PTC. The two images
// |  MUST be the same size or the methods behavior is undefined as this cannot
// |  be verified using the given parameters.
// |
// |  <IN>  -> pBuf1   : 1st image buffer pointer
// |  <IN>  -> pBuf2   : 2nd image buffer pointer
// |  <IN>  -> dRow1   : Start row
// |  <IN>  -> dRow2   : End row
// |  <IN>  -> dCol1   : Start column
// |  <IN>  -> dCol2   : End column
// |  <IN>  -> dRows   : Number of rows in full image
// |  <IN>  -> dCols   : Number of columns in each full image
// |  <IN>  -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// |
// |  Returns a CArcImage::CArcImageDiffStats structure with all the statistice
// |  filled in. Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
DLLARCIMAGE_API struct CImgDifStats ArcImage_GetDiffStats( void* pBuf1, void* pBuf2, int dRow1,
														   int dRow2, int dCol1, int dCol2,
														   int dRows, int dCols, int dBpp,
														   int* pStatus )
{
	struct CImgDifStats sDifStats;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_GetDiffStats", g_pCImage )

		CArcImage::CImgDifStats cDifStats =
									g_pCImage.get()->GetDiffStats( pBuf1,
																   pBuf2,
																   dRow1,
																   dRow2,
																   dCol1,
																   dCol2,
																   dRows,
																   dCols,
																   dBpp );

		sDifStats.cImg1Stats.gMax					= cDifStats.cImg1Stats.gMax;
		sDifStats.cImg1Stats.gMean					= cDifStats.cImg1Stats.gMean;
		sDifStats.cImg1Stats.gMin					= cDifStats.cImg1Stats.gMin;
		sDifStats.cImg1Stats.gSaturatedPixCnt		= cDifStats.cImg1Stats.gSaturatedPixCnt;
		sDifStats.cImg1Stats.gStdDev				= cDifStats.cImg1Stats.gStdDev;
		sDifStats.cImg1Stats.gTotalPixels			= cDifStats.cImg1Stats.gTotalPixels;
		sDifStats.cImg1Stats.gVariance				= cDifStats.cImg1Stats.gVariance;

		sDifStats.cImg2Stats.gMax					= cDifStats.cImg2Stats.gMax;
		sDifStats.cImg2Stats.gMean					= cDifStats.cImg2Stats.gMean;
		sDifStats.cImg2Stats.gMin					= cDifStats.cImg2Stats.gMin;
		sDifStats.cImg2Stats.gSaturatedPixCnt		= cDifStats.cImg2Stats.gSaturatedPixCnt;
		sDifStats.cImg2Stats.gStdDev				= cDifStats.cImg2Stats.gStdDev;
		sDifStats.cImg2Stats.gTotalPixels			= cDifStats.cImg2Stats.gTotalPixels;
		sDifStats.cImg2Stats.gVariance				= cDifStats.cImg2Stats.gVariance;

		sDifStats.cImgDiffStats.gMax				= cDifStats.cImgDiffStats.gMax;
		sDifStats.cImgDiffStats.gMean				= cDifStats.cImgDiffStats.gMean;
		sDifStats.cImgDiffStats.gMin				= cDifStats.cImgDiffStats.gMin;
		sDifStats.cImgDiffStats.gSaturatedPixCnt	= cDifStats.cImgDiffStats.gSaturatedPixCnt;
		sDifStats.cImgDiffStats.gStdDev				= cDifStats.cImgDiffStats.gStdDev;
		sDifStats.cImgDiffStats.gTotalPixels		= cDifStats.cImgDiffStats.gTotalPixels;
		sDifStats.cImgDiffStats.gVariance			= cDifStats.cImgDiffStats.gVariance;
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return sDifStats;
}


// +----------------------------------------------------------------------------
// |  GetImageDiffStats
// +----------------------------------------------------------------------------
// |  Calculates the image min, max, mean, variance, and standard deviation over
// |  the entire image.
// |
// |  <IN>  -> pBuf1   : 1st pointer to image buffer
// |  <IN>  -> pBuf2   : 2nd pointer to image buffer
// |  <IN>  -> dRows   : Number of rows in full image
// |  <IN>  -> dCols   : Number of columns in full image
// |  <IN>  -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// |
// |  Returns a CArcImage::CArcImageStats structure with all the statistice
// |  filled in. Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
DLLARCIMAGE_API struct CImgDifStats ArcImage_GetImageDiffStats( void* pBuf1, void* pBuf2,
															    int dRows, int dCols, int dBpp,
															    int* pStatus )
{
	struct CImgDifStats sDifStats;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_GetDiffStats", g_pCImage )

		CArcImage::CImgDifStats cDifStats =
									g_pCImage.get()->GetDiffStats( pBuf1,
																   pBuf2,
																   0,
																   dRows,
																   0,
																   dCols,
																   dRows,
																   dCols,
																   dBpp );

		sDifStats.cImg1Stats.gMax					= cDifStats.cImg1Stats.gMax;
		sDifStats.cImg1Stats.gMean					= cDifStats.cImg1Stats.gMean;
		sDifStats.cImg1Stats.gMin					= cDifStats.cImg1Stats.gMin;
		sDifStats.cImg1Stats.gSaturatedPixCnt		= cDifStats.cImg1Stats.gSaturatedPixCnt;
		sDifStats.cImg1Stats.gStdDev				= cDifStats.cImg1Stats.gStdDev;
		sDifStats.cImg1Stats.gTotalPixels			= cDifStats.cImg1Stats.gTotalPixels;
		sDifStats.cImg1Stats.gVariance				= cDifStats.cImg1Stats.gVariance;

		sDifStats.cImg2Stats.gMax					= cDifStats.cImg2Stats.gMax;
		sDifStats.cImg2Stats.gMean					= cDifStats.cImg2Stats.gMean;
		sDifStats.cImg2Stats.gMin					= cDifStats.cImg2Stats.gMin;
		sDifStats.cImg2Stats.gSaturatedPixCnt		= cDifStats.cImg2Stats.gSaturatedPixCnt;
		sDifStats.cImg2Stats.gStdDev				= cDifStats.cImg2Stats.gStdDev;
		sDifStats.cImg2Stats.gTotalPixels			= cDifStats.cImg2Stats.gTotalPixels;
		sDifStats.cImg2Stats.gVariance				= cDifStats.cImg2Stats.gVariance;

		sDifStats.cImgDiffStats.gMax				= cDifStats.cImgDiffStats.gMax;
		sDifStats.cImgDiffStats.gMean				= cDifStats.cImgDiffStats.gMean;
		sDifStats.cImgDiffStats.gMin				= cDifStats.cImgDiffStats.gMin;
		sDifStats.cImgDiffStats.gSaturatedPixCnt	= cDifStats.cImgDiffStats.gSaturatedPixCnt;
		sDifStats.cImgDiffStats.gStdDev				= cDifStats.cImgDiffStats.gStdDev;
		sDifStats.cImgDiffStats.gTotalPixels		= cDifStats.cImgDiffStats.gTotalPixels;
		sDifStats.cImgDiffStats.gVariance			= cDifStats.cImgDiffStats.gVariance;
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return sDifStats;
}


// +----------------------------------------------------------------------------
// |  GetStats
// +----------------------------------------------------------------------------
// |  Calculates the image min, max, mean, variance, and standard deviation over
// |  the specified image memory rows and cols.
// |
// |  <IN>  -> pBuf	   : Pointer to image buffer
// |  <IN>  -> dRow1   : Start row
// |  <IN>  -> dRow2   : End row
// |  <IN>  -> dCol1   : Start column
// |  <IN>  -> dCol2   : End column
// |  <IN>  -> dRows   : Number of rows in full image
// |  <IN>  -> dCols   : Number of columns in full image
// |  <IN>  -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// |
// |  Returns a CArcImage::CArcImageStats structure with all the statistice
// |  filled in.  Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
DLLARCIMAGE_API struct CImgStats ArcImage_GetStats( void* pBuf, int dRow1, int dRow2,
												    int dCol1, int dCol2, int dRows,
													int dCols, int dBpp, int* pStatus )
{
	struct CImgStats sStats;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_GetStats", g_pCImage )

		CArcImage::CImgStats cStats =
								g_pCImage.get()->GetStats( pBuf,
														   dRow1,
														   dRow2,
														   dCol1,
														   dCol2,
														   dRows,
														   dCols,
														   dBpp );

		sStats.gMax				= cStats.gMax;
		sStats.gMean			= cStats.gMean;
		sStats.gMin				= cStats.gMin;
		sStats.gSaturatedPixCnt	= cStats.gSaturatedPixCnt;
		sStats.gStdDev			= cStats.gStdDev;
		sStats.gTotalPixels		= cStats.gTotalPixels;
		sStats.gVariance		= cStats.gVariance;
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return sStats;
}


// +----------------------------------------------------------------------------
// |  GetStats
// +----------------------------------------------------------------------------
// |  Calculates the image min, max, mean, variance, and standard deviation over
// |  the entire image.
// |
// |  <IN>  -> pBuf    : Pointer to image buffer
// |  <IN>  -> dRows   : Number of rows in full image
// |  <IN>  -> dCols   : Number of columns in full image
// |  <IN>  -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// |
// |  Returns a CArcImage::CArcImageStats structure with all the statistice
// |  filled in.  Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
DLLARCIMAGE_API struct CImgStats ArcImage_GetImageStats( void* pBuf, int dRows,
														 int dCols, int dBpp,
														 int* pStatus )
{
	struct CImgStats sStats;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_GetStats", g_pCImage )

		CArcImage::CImgStats cStats =
								g_pCImage.get()->GetStats( pBuf,
														   dRows,
														   dCols,
														   dBpp );

		sStats.gMax				= cStats.gMax;
		sStats.gMean			= cStats.gMean;
		sStats.gMin				= cStats.gMin;
		sStats.gSaturatedPixCnt	= cStats.gSaturatedPixCnt;
		sStats.gStdDev			= cStats.gStdDev;
		sStats.gTotalPixels		= cStats.gTotalPixels;
		sStats.gVariance		= cStats.gVariance;
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return sStats;
}


// +----------------------------------------------------------------------------
// |  Histogram
// +----------------------------------------------------------------------------
// |  Calculates the histogram over the specified image memory rows and cols.
// |
// |  <OUT> -> rdCount : Pointer to image buffer
// |  <IN>  -> pBuf    : Pointer to image memory
// |  <IN>  -> dRow1   : Start row
// |  <IN>  -> dRow2   : End row
// |  <IN>  -> dCol1   : Start column
// |  <IN>  -> dCol2   : End column
// |  <IN>  -> dRows   : Number of rows in full image
// |  <IN>  -> dCols   : Number of columns in full image
// |  <IN>  -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// |
// |  Returns a pointer to an array of elements that represent the histogram.
// |  This data SHOULD NOT be freed by the calling application.
// +----------------------------------------------------------------------------
DLLARCIMAGE_API int* ArcImage_Histogram( int* pCount, void* pBuf, int dRow1, int dRow2,
										 int dCol1, int dCol2, int dRows, int dCols,
										 int dBpp, int* pStatus )
{
	int* pHist = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_Histogram", g_pCImage )

		pHist =	g_pCImage.get()->Histogram( *pCount,
											pBuf,
											dRow1,
											dRow2,
											dCol1,
											dCol2,
											dRows,
											dCols,
											dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pHist;
}


// +----------------------------------------------------------------------------
// |  Histogram
// +----------------------------------------------------------------------------
// |  Calculates the histogram over the entire image memory.
// |
// |  <OUT> -> rdCount : Number of elements in returned array
// |  <IN>  -> pBuf    : Pointer to image buffer
// |  <IN>  -> dRows   : Number of rows in full image
// |  <IN>  -> dCols   : Number of columns in full image
// |  <IN>  -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// |
// |  Returns a pointer to an array of elements that represent the histogram.
// |  This data SHOULD NOT be freed by the calling application.
// +----------------------------------------------------------------------------
DLLARCIMAGE_API int* ArcImage_ImageHistogram( int* pCount, void* pBuf, int dRows,
											  int dCols, int dBpp, int* pStatus )
{
	int* pHist = NULL;

	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_Histogram", g_pCImage )

		pHist =	g_pCImage.get()->Histogram( *pCount,
											pBuf,
											dRows,
											dCols,
											dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}

	return pHist;
}


// +----------------------------------------------------------------------------
// |  Add
// +----------------------------------------------------------------------------
// |  Adds buffer 1 to buffer2 and outputs the result in a new 32-bit buffer.
// |
// |  <OUT> -> pU32Buf  : Pointer to 32-bit image buffer
// |  <IN>  -> pU16Buf1 : Pointer to 16-bit image buffer 1
// |  <IN>  -> pU16Buf2 : Pointer to 16-bit image buffer 2
// |  <IN>  -> dRows    : Number of rows in full image
// |  <IN>  -> dCols    : Number of columns in full image
// |  <OUT> -> pStatus  : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_Add( unsigned int* pU32Buf, unsigned short* pU16Buf1,
								   unsigned short* pU16Buf2, int dRows, int dCols,
								   int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_Add", g_pCImage )

		g_pCImage.get()->Add( pU32Buf, pU16Buf1, pU16Buf2, dRows, dCols );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  Subtract
// +----------------------------------------------------------------------------
// |  Subtracts buffer 2 from buffer 1. Buffer 1 is replaced with the new data.
// |
// |  <IN>  -> pBuf1    : Pointer to 1st image buffer
// |  <IN>  -> pBuf2    : Pointer to 2nd image buffer
// |  <IN>  -> dRows    : Number of rows in full image
// |  <IN>  -> dCols    : Number of columns in full image
// |  <IN>  -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus  : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_Subtract( void* pBuf1, void* pBuf2, int dRows,
									    int dCols, int dBpp, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_Subtract", g_pCImage )

		g_pCImage.get()->Subtract( pBuf1,
								   pBuf2,
								   dRows,
								   dCols,
								   dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  SubtractHalves
// +----------------------------------------------------------------------------
// |  Subtracts the first half of an image from the second half.  NOTE: The
// |  first half of the image buffer is replaced with the new image.
// |
// |  Throws std::runtime_error on if the number of rows is not even or
// |  if any of the image buffer pointers is null.
// |
// |  <IN>  -> pBuf     : Pointer to image buffer
// |  <IN>  -> dRows    : The number of rows in entire image ( includes both halves )
// |  <IN>  -> dCols    : The number of cols in the image
// |  <IN>  -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus  : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_SubtractHalves( void* pBuf, int dRows, int dCols,
											  int dBpp, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_SubtractHalves", g_pCImage )

		g_pCImage.get()->SubtractHalves( pBuf,
										 dRows,
										 dCols,
										 dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  Divide
// +----------------------------------------------------------------------------
// |  Divides buffer 1 by buffer 2. Buffer 1 is replaced with the new data.
// |
// |  <IN>  -> pBuf1    : Pointer to 1st image buffer
// |  <IN>  -> pBuf2    : Pointer to 2nd image buffer
// |  <IN>  -> dRows    : Number of rows in full image
// |  <IN>  -> dCols    : Number of columns in full image
// |  <IN>  -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus  : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_Divide( void* pBuf1, void* pBuf2, int dRows,
									  int dCols, int dBpp, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_Divide", g_pCImage )

		g_pCImage.get()->Divide( pBuf1,
							     pBuf2,
							     dRows,
							     dCols,
							     dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  Copy
// +----------------------------------------------------------------------------
// |  Copies the source buffer to the destination buffer.  The source buffer
// |  must be less than or equal in dimension to the destination buffer. These
// |  values are checked.
// |
// |  <IN>  -> pDstBuf  : Pointer to destination buffer
// |  <IN>  -> dDstRows : Number of rows in destination buffer
// |  <IN>  -> dDstCols : Number of columns in destination buffer
// |  <IN>  -> pSrcBuf  : Pointer to source buffer
// |  <IN>  -> dSrcRows : Number of rows in source buffer
// |  <IN>  -> dSrcCols : Number of columns in source buffer
// |  <IN>  -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus  : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_Copy( void* pDstBuf, int dDstRows, int dDstCols,
									void* pSrcBuf, int dSrcRows, int dSrcCols,
									int dBpp, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_Copy", g_pCImage )

		g_pCImage.get()->Copy( pDstBuf,
							   dDstRows,
							   dDstCols,
							   pSrcBuf,
							   dSrcRows,
							   dSrcCols,
							   dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  CopyBySize
// +----------------------------------------------------------------------------
// |  Copies the source buffer to the destination buffer.  The source buffer
// |  must be less than or equal in dimension to the destination buffer. These
// |  values are checked.
// |
// |  <IN>  -> pDstBuf  : Pointer to destination buffer
// |  <IN>  -> dDstSize : Size ( in bytes ) of the destination buffer
// |  <IN>  -> pSrcBuf  : Pointer to source buffer
// |  <IN>  -> dSrcSize : Size ( in bytes ) of the source buffer
// |  <OUT> -> pStatus  : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_CopyBySize( void* pDstBuf, int dDstSize,
										  void* pSrcBuf, int dSrcSize,
										  int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_CopyBySize", g_pCImage )

		g_pCImage.get()->Copy( pDstBuf,
							   dDstSize,
							   pSrcBuf,
							   dSrcSize );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  Fill
// +----------------------------------------------------------------------------
// |  Fills the image memory with the specified value.
// |
// |  <IN>  -> pBuf     : Pointer to image buffer
// |  <IN>  -> dRows    : Number of rows in full image
// |  <IN>  -> dCols    : Number of columns in full image
// |  <IN>  -> dValue   : The with which to fill the buffer
// |  <IN>  -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus  : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_Fill( void* pBuf, int dRows, int dCols, int dValue,
									int dBpp, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_Fill", g_pCImage )

		g_pCImage.get()->Fill( pBuf,
							   dRows,
							   dCols,
							   dValue,
							   dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  GradientFill
// +----------------------------------------------------------------------------
// |  Fills the image memory with a gradient pattern.
// |
// |  <IN>  -> pBuf     : Pointer to image buffer
// |  <IN>  -> dRows    : Number of rows in full image
// |  <IN>  -> dCols    : Number of columns in full image
// |  <IN>  -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus  : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_GradientFill( void* pBuf, int dRows, int dCols,
										    int dBpp, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_GradientFill", g_pCImage )

		g_pCImage.get()->GradientFill( pBuf,
									   dRows,
									   dCols,
									   dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  SmileyFill
// +----------------------------------------------------------------------------
// |  Fills the image memory with zeroes and puts a smiley face at the center.
// |
// |  <IN>  -> pBuf     : Pointer to image buffer
// |  <IN>  -> dRows    : Number of rows in full image
// |  <IN>  -> dCols    : Number of columns in full image
// |  <IN>  -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus  : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_SmileyFill( void* pBuf, int dRows, int dCols,
										  int dBpp, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_SmileyFill", g_pCImage )

		g_pCImage.get()->SmileyFill( pBuf,
									 dRows,
									 dCols,
									 dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// |  VerifyAsSynthetic
// +----------------------------------------------------------------------------
// |  Verifies that the kernel buffer contains a valid synthetic image.
// |
// |  NOTE: The PCI DSP firmware has an issue where img[ 0 ] = 1 up to 65535,
// |  then, img[ 65536 ] = 0, where it should be 1. So this method takes this
// |  into account by reading and setting compareValue to the first pixel for
// |  each new 65535 pixel segment.
// |
// |  Throws: std::runtime_error on error
// |
// |  <IN>  -> pBuf     : Pointer to image buffer
// |  <IN>  -> dRows    : The number of rows in the synthetic image.
// |  <IN>  -> dCols    : The number of columns in the synthetic image.
// |  <IN>  -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// |  <OUT> -> pStatus  : Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +----------------------------------------------------------------------------
DLLARCIMAGE_API void ArcImage_VerifyAsSynthetic( void* pBuf, int dRows, int dCols,
												 int dBpp, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcImage_VerifyAsSynthetic", g_pCImage )

		g_pCImage.get()->VerifyAsSynthetic( pBuf,
											dRows,
											dCols,
											dBpp );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +----------------------------------------------------------------------------
// | GetLastError
// +----------------------------------------------------------------------------
// | Returns the last error message reported.
// +----------------------------------------------------------------------------
DLLARCIMAGE_API const char* ArcImage_GetLastError()
{
	return const_cast<const char *>( g_szErrMsg );
}
