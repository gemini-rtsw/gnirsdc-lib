// CArcImage.cpp : Defines the exported functions for the DLL application.
//
#ifdef WIN32
	#include <windows.h>
#endif

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cstring>
#include "CArcImage.h"

using namespace std;
using namespace arc::image;

//#include <iostream>
//#include <fstream>
//ofstream dbgStream( "CArcImage_Debug.txt" );


// +----------------------------------------------------------------------------
// |  MACRO - VERIFY_ROW
// +----------------------------------------------------------------------------
// |  Throws a runtime_error exception if the specified row is less than zero
// |  or greater than the specified image row length.
// |
// |  <IN> -> sMethodName : Method where the error occurred
// |  <IN> -> dRow        : The row to check
// |  <IN> -> dRows       : The total row length ( i.e. image row count )
// +----------------------------------------------------------------------------
#define VERIFY_ROW( sMethodName, dRow, dRows )						\
					if ( dRow == dRows )							\
					{												\
						dRow = ( dRows - 1 );						\
					}												\
																	\
					if ( ( dRow < 0 ) || ( dRow > dRows ) ) 		\
					{												\
						ostringstream oss;							\
																	\
						oss << "Invalid row [ " << dRow				\
							<< " ]! Must be between 0 and "			\
							<< dRows << "!" << ends;				\
																	\
						ThrowException( sMethodName, oss.str() );	\
					}


// +----------------------------------------------------------------------------
// |  MACRO - VERIFY_COL
// +----------------------------------------------------------------------------
// |  Throws a runtime_error exception if the specified column is less than
// |  zero or greater than the specified image column length.
// |
// |  <IN> -> sMethodName : Method where the error occurred
// |  <IN> -> dCol        : The column to check
// |  <IN> -> dCols       : The total column length ( i.e. image column count )
// +----------------------------------------------------------------------------
#define VERIFY_COL( sMethodName, dCol, dCols )						\
					if ( dCol == dCols )							\
					{												\
						dCol = ( dCols - 1 );						\
					}												\
																	\
					if ( ( dCol < 0 ) || ( dCol >= dCols ) )		\
					{												\
						ostringstream oss;							\
																	\
						oss << "Invalid column [ " << dCol			\
							<< " ]! Must be between 0 and "			\
							<< dCols << "!" << ends;				\
																	\
						ThrowException( sMethodName, oss.str() );	\
					}


// +----------------------------------------------------------------------------
// |  MACRO - VERIFY_RANGE_ORDER
// +----------------------------------------------------------------------------
// |  Throws a runtime_error exception if value 2 is less than value 1.  This
// |  is used to ensure the coordinate range is valid.
// |
// |  <IN> -> sMethodName : Method where the error occurred
// |  <IN> -> dValue1     : The first ( lesser ) range value
// |  <IN> -> dValue2     : The second ( higher ) range value
// +----------------------------------------------------------------------------
#define VERIFY_RANGE_ORDER( sMethodName, dValue1, dValue2 )			\
					if ( dValue2 < dValue1 )						\
					{												\
						ostringstream oss;							\
																	\
						oss << "Invalid range order [ " << dValue2	\
							<< " < " << dValue1						\
							<< " ]! Values must be reversed!"		\
							<< ends;								\
																	\
						ThrowException( sMethodName, oss.str() );	\
					}

// +----------------------------------------------------------------------------
// |  MACRO - VERIFY_IMG_DIMEN
// +----------------------------------------------------------------------------
// |  Throws a runtime_error exception if the specified image dimension is less
// |  than or equal to zero or greater than 65K. The specified dimension can be
// |  either the image rows or columns.
// |
// |  <IN> -> sMethodName : Method where the error occurred
// |  <IN> -> dDimen      : The dimension ( rows or columns to check )
// +----------------------------------------------------------------------------
#define VERIFY_IMG_DIMEN( sMethodName, dDimen )						\
					if ( ( dDimen <= 0 ) || ( dDimen > 65535 ) )	\
					{												\
						ostringstream oss;							\
																	\
						oss << "Invalid image dimension: "			\
							<< dDimen << "!" << ends;				\
																	\
						ThrowException( sMethodName, oss.str() );	\
					}

// +----------------------------------------------------------------------------
// |  MACRO - VERIFY_BPP
// +----------------------------------------------------------------------------
// |  Throws a runtime_error exception if the specified bits-per-pixel value
// |  is not one of the allowed values ( CArcImage::BPP16 or CArcImage::BPP32 ).
// |
// |  <IN> -> sMethodName : Method where the error occurred
// |  <IN> -> dDimen      : The dimension ( rows or columns to check )
// +----------------------------------------------------------------------------
#define VERIFY_BPP( sMethodName, dBpp )											\
					if ( dBpp != CArcImage::BPP16 && dBpp != CArcImage::BPP32 )	\
					{															\
						ostringstream oss;										\
																				\
						oss << "Invalid bits-per-pixel parameter: "	<< dBpp		\
							<< "! Must be CArcImage::BPP16 or CArcImage::BPP32" \
							<< ends;											\
																				\
						ThrowException( sMethodName, oss.str() );				\
					}

// +----------------------------------------------------------------------------
// |  MACRO - VERIFY_BUFFER
// +----------------------------------------------------------------------------
// |  Throws a runtime_error exception if the specified image buffer is NULL.
// |
// |  <IN> -> sMethodName : Method where the error occurred
// |  <IN> -> dDimen      : The dimension ( rows or columns to check )
// +----------------------------------------------------------------------------
#define VERIFY_BUFFER( sMethodName, pBuf )										\
					if ( pBuf == NULL )											\
					{															\
						ostringstream oss;										\
																				\
						oss << "Invalid image buffer parameter: NULL!"			\
							<< ends;											\
																				\
						ThrowException( sMethodName, oss.str() );				\
					}

// +----------------------------------------------------------------------------
// |  MACRO - TYPE_SIZE_OF
// +----------------------------------------------------------------------------
// |  Returns the correct byte size associated with the specified bits-per-pixel
// |  and count parameters.  Used to allocate byte arrays of the correct size
// |  for the specified bits-per-pixel.
// |
// |  <IN> -> dBpp   : The bits-per-pixel value
// |  <IN> -> dCount : The value to multiply the dBpp size by
// +----------------------------------------------------------------------------
#define TYPE_SIZE_OF( dBpp )													\
						( ( dBpp == BPP16 ) ?									\
							size_t( sizeof( tU16 ) ) :							\
							size_t( sizeof( tU32 ) ) )


// +----------------------------------------------------------------------------
// |  MACRO - MAX_TYPE_VALUE
// +----------------------------------------------------------------------------
// |  Returns the maximum data value for the specified bits-per-pixel. This
// |  basically calculates ( 2^BPP ).
// |
// |  <IN> -> dBpp   : The bits-per-pixel value
// +----------------------------------------------------------------------------
#define MAX_TYPE_VALUE( dBpp ) ( ( dBpp == BPP16 ) ?							\
							     ( T_SIZE( tU16 ) ) :							\
							     ( T_SIZE( tU32 ) ) )


// +----------------------------------------------------------------------------
// |  MACRO - STORE_U8_POINTER
// +----------------------------------------------------------------------------
// |  Creates a new unsigned int shared pointer to the specified pointer and
// |  stores the result in the appropriate pointer map data member.
// |
// |  <IN> -> ptr   : The int pointer to store
// +----------------------------------------------------------------------------
#define STORE_U8_POINTER( ptr )													\
					if ( ptr != NULL )											\
					{															\
						tSharedU8Ptr pNew( ptr,                                 \
										   &CArcImage::ArrayDeleter<tU8> );     \
						m_u8PtrMap.insert( pair<unsigned long,tSharedU8Ptr>(	\
						reinterpret_cast<unsigned long>( pNew.get() ), pNew ) );\
					}

// +----------------------------------------------------------------------------
// |  MACRO - STORE_FLT_POINTER
// +----------------------------------------------------------------------------
// |  Creates a new float shared pointer to the specified pointer and stores
// |  the result in the appropriate pointer map data member.
// |
// |  <IN> -> ptr   : The float pointer to store
// +----------------------------------------------------------------------------
#define STORE_FLT_POINTER( ptr )												\
					if ( ptr != NULL )											\
					{															\
						tSharedFltPtr pNew( ptr,                                \
											&CArcImage::ArrayDeleter<float> );	\
						m_fltPtrMap.insert( pair<unsigned long,tSharedFltPtr>(	\
						reinterpret_cast<unsigned long>( pNew.get() ), pNew ) );\
					}

// +----------------------------------------------------------------------------
// |  MACRO - STORE_S32_POINTER
// +----------------------------------------------------------------------------
// |  Creates a new signed int shared pointer to the specified pointer and
// |  stores the result in the appropriate pointer map data member.
// |
// |  <IN> -> ptr   : The signed int pointer to store
// +----------------------------------------------------------------------------
#define STORE_S32_POINTER( ptr )												\
					if ( ptr != NULL )											\
					{															\
						tSharedS32Ptr pNew( ptr,                                \
											&CArcImage::ArrayDeleter<int> );	\
						m_s32PtrMap.insert( pair<unsigned long,tSharedS32Ptr>(	\
						reinterpret_cast<unsigned long>( pNew.get() ), pNew ) );\
					}


// +----------------------------------------------------------------------------
// |  CONSTANTS and TYPEDEFS
// +----------------------------------------------------------------------------
#define DEG2RAD		( 3.14159 / 180 )


typedef	unsigned short	tU16;
typedef unsigned int	tU32;
typedef	unsigned char	tU8;


#ifdef WIN32
	#define Arc_ZeroMemory( pDest, dSize )			ZeroMemory( pDest, dSize )
	#define Arc_CopyMemory( pDest, pSrc, dSize )	CopyMemory( pDest, pSrc, dSize )
#else
	#define Arc_ZeroMemory( pDest, dSize )			memset( pDest, 0, dSize )
	#define Arc_CopyMemory( pDest, pSrc, dSize )	memcpy( pDest, pSrc, dSize )
#endif


// +----------------------------------------------------------------------------
// |  Static member initialization
// +----------------------------------------------------------------------------
std::unordered_map<unsigned long,tSharedU8Ptr>  CArcImage::m_u8PtrMap;
std::unordered_map<unsigned long,tSharedFltPtr> CArcImage::m_fltPtrMap;
std::unordered_map<unsigned long,tSharedS32Ptr> CArcImage::m_s32PtrMap;


// +----------------------------------------------------------------------------
// | ArrayDeleter
// +----------------------------------------------------------------------------
// | Called by std::shared_ptr to delete a temporary buffer passed to the user.
// +----------------------------------------------------------------------------
template<typename T> void CArcImage::ArrayDeleter( T* p )
{
	if ( p != NULL )
	{
		delete [] p;

		p = NULL;
	}
}


// +----------------------------------------------------------------------------
// |  Default constructor
// +----------------------------------------------------------------------------
CArcImage::CArcImage()
{
}


// +----------------------------------------------------------------------------
// |  Default destructor
// +----------------------------------------------------------------------------
CArcImage::~CArcImage()
{
	if ( !m_u8PtrMap.empty() )
	{
		m_u8PtrMap.erase( m_u8PtrMap.begin(), m_u8PtrMap.end() );
	}

	if ( !m_fltPtrMap.empty() )
	{
		m_fltPtrMap.erase( m_fltPtrMap.begin(), m_fltPtrMap.end() );
	}

	if ( !m_s32PtrMap.empty() )
	{
		m_s32PtrMap.erase( m_s32PtrMap.begin(), m_s32PtrMap.end() );
	}
}


// +----------------------------------------------------------------------------
// |  Free
// +----------------------------------------------------------------------------
// |  Frees data, such as a row or column, that was passed to the caller. This
// |  method MUST be called by the user to free the returned data buffer.
// |
// |  <IN>  -> ptr : Pointer to free
// +----------------------------------------------------------------------------
void CArcImage::Free( unsigned char* ptr )
{
	if ( ptr != NULL )
	{
		//  This will automatically call the CArcImage::ArrayDeleter
		//  method, which will delete the array.
		// +---------------------------------------------------------+
		m_u8PtrMap.erase( reinterpret_cast<unsigned long>( ptr ) );
	}
}


// +----------------------------------------------------------------------------
// |  Free
// +----------------------------------------------------------------------------
// |  Frees data, such as a row or column, that was passed to the caller. This
// |  method MUST be called by the user to free the returned data buffer.
// |
// |  <IN>  -> ptr : Pointer to free
// +----------------------------------------------------------------------------
void CArcImage::Free( float* ptr )
{
	if ( ptr != NULL )
	{
		//  This will automatically call the CArcImage::ArrayDeleter
		//  method, which will delete the array.
		// +---------------------------------------------------------+
		m_fltPtrMap.erase( reinterpret_cast<unsigned long>( ptr ) );
	}
}


// +----------------------------------------------------------------------------
// |  Free
// +----------------------------------------------------------------------------
// |  Frees data, such as a row or column, that was passed to the caller. This
// |  method MUST be called by the user to free the returned data buffer.
// |
// |  <IN>  -> ptr : Pointer to free
// +----------------------------------------------------------------------------
void CArcImage::Free( int* ptr )
{
	if ( ptr != NULL )
	{
		//  This will automatically call the CArcImage::ArrayDeleter
		//  method, which will delete the array.
		// +---------------------------------------------------------+
		m_s32PtrMap.erase( reinterpret_cast<unsigned long>( ptr ) );
	}
}


// +----------------------------------------------------------------------------
// |  GetRow
// +----------------------------------------------------------------------------
// |  Returns a row of pixel data from the specified image buffer. All or part
// |  of the row can be returned.
// |
// |  <IN> -> pBuf    : Pointer to image buffer
// |  <IN> -> dRow    : The row to return
// |  <IN> -> dCol1   : Start column 
// |  <IN> -> dCol2   : End column
// |  <IN> -> dRows   : Number of rows in full image
// |  <IN> -> dCols   : Number of columns in full image
// |  <IN> -> rdCount : The number of elements in the return buffer
// |  <IN> -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// |
// |  Returns: Pointer to the row data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CArcImage::Free().
// +----------------------------------------------------------------------------
unsigned char* CArcImage::GetRow( void* pBuf, int dRow, int dCol1, int dCol2,
								  int dRows, int dCols, int& rdCount, int dBpp )
{
	tU8* pU8Row = NULL;

	VERIFY_IMG_DIMEN( "GetRow", dRows )
	VERIFY_IMG_DIMEN( "GetRow", dCols )

	VERIFY_ROW( "GetRow", dRow,  dRows )
	VERIFY_COL( "GetRow", dCol1, dCols )
	VERIFY_COL( "GetRow", dCol2, dCols )

	VERIFY_RANGE_ORDER( "GetRow", dCol1, dCol2 )

	VERIFY_BUFFER( "GetRow", pBuf )
	VERIFY_BPP( "GetRow", dBpp )

	rdCount = ( ( dCol2 - dCol1 ) == 0 ? 1 : ( dCol2 - dCol1 ) );

	try
	{
		pU8Row = new tU8[ rdCount * TYPE_SIZE_OF( dBpp ) ];
	}
	catch ( bad_alloc& ba )
	{
		ThrowException( "GetRow",
						"Failed to allocate row data buffer! " +
						 string( ba.what() ) );
	}

	if ( dBpp == CArcImage::BPP16 )
	{
		tU16* pSrc = reinterpret_cast<tU16*>( pBuf );

		Arc_CopyMemory( pU8Row,
						&pSrc[ dCol1 + dRow * dCols ],
						( rdCount * TYPE_SIZE_OF( dBpp ) ) );
	}

	else
	{
		tU32* pSrc = reinterpret_cast<tU32*>( pBuf );

		Arc_CopyMemory( pU8Row,
						&pSrc[ dCol1 + dRow * dCols ],
						( rdCount * TYPE_SIZE_OF( dBpp ) ) );
	}

	STORE_U8_POINTER( pU8Row )

	return pU8Row;
}


// +----------------------------------------------------------------------------
// |  GetCol
// +----------------------------------------------------------------------------
// |  Returns a column of pixel data from the specified image buffer. All or
// |  part of the column can be returned.
// |
// |  <IN> -> pBuf    : Pointer to image buffer
// |  <IN> -> dCol    : The column to return
// |  <IN> -> dRow1   : Start row 
// |  <IN> -> dRow2   : End row
// |  <IN> -> dRows   : Number of rows in full image
// |  <IN> -> dCols   : Number of columns in full image
// |  <IN> -> rdCount : The number of elements in the return buffer
// |  <IN> -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// |
// |  Returns: Pointer to the column data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CArcImage::Free().
// +----------------------------------------------------------------------------
unsigned char* CArcImage::GetCol( void* pBuf, int dCol, int dRow1, int dRow2,
								  int dRows, int dCols, int& rdCount, int dBpp )
{
	tU8* pU8Col = NULL;

	VERIFY_IMG_DIMEN( "GetCol", dRows )
	VERIFY_IMG_DIMEN( "GetCol", dCols )

	VERIFY_COL( "GetCol", dCol,  dCols )
	VERIFY_ROW( "GetCol", dRow1, dRows )
	VERIFY_ROW( "GetCol", dRow2, dRows )

	VERIFY_RANGE_ORDER( "GetCol", dRow1, dRow2 )

	VERIFY_BUFFER( "GetCol", pBuf )
	VERIFY_BPP( "GetCol", dBpp )

	rdCount = ( ( dRow2 - dRow1 ) == 0 ? 1 : ( dRow2 - dRow1 ) );

	try
	{
		pU8Col = new tU8[ rdCount * TYPE_SIZE_OF( dBpp ) ];
	}
	catch ( bad_alloc& ba )
	{
		ThrowException(	"GetCol",
						"Failed to allocate column data buffer! " +
						 string( ba.what() ) );
	}

	if ( dBpp == CArcImage::BPP16 )
	{
		tU16* pSrc = reinterpret_cast<tU16*>( pBuf );
		tU16* pDst = reinterpret_cast<tU16*>( pU8Col );

		for ( int dRow=dRow1, i=0; dRow<dRow2; dRow++, i++ )
		{
			pDst[ i ] = pSrc[ dCol + dRow * dCols ];
		}
	}

	else
	{
		tU32* pSrc = reinterpret_cast<tU32*>( pBuf );
		tU32* pDst = reinterpret_cast<tU32*>( pU8Col );

		for ( int dRow=dRow1, i=0; dRow<dRow2; dRow++, i++ )
		{
			pDst[ i ] = pSrc[ dCol + dRow * dCols ];
		}
	}

	STORE_U8_POINTER( pU8Col )

	return pU8Col;
}


// +----------------------------------------------------------------------------
// |  GetRowArea
// +----------------------------------------------------------------------------
// |  Returns a row of pixel data where each value is the average over the
// |  specified range of columns.
// |
// |  <IN> -> pBuf    : Pointer to image buffer
// |  <IN> -> dRow1   : Start row 
// |  <IN> -> dRow2   : End row
// |  <IN> -> dCol1   : Start column 
// |  <IN> -> dCol2   : End column
// |  <IN> -> dRows   : Number of rows in full image
// |  <IN> -> dCols   : Number of columns in full image
// |  <IN> -> rdCount : The number of elements in the return buffer
// |  <IN> -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// |
// |  Returns: Pointer to the row data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CArcImage::Free().
// +----------------------------------------------------------------------------
float* CArcImage::GetRowArea( void* pBuf, int dRow1, int dRow2, int dCol1, int dCol2,
							  int dRows, int dCols, int& rdCount, int dBpp )
{
	float* pfAreaBuf = NULL;
	float  fRowSum   = 0;

	VERIFY_IMG_DIMEN( "GetRowArea", dRows )
	VERIFY_IMG_DIMEN( "GetRowArea", dCols )

	VERIFY_ROW( "GetRowArea", dRow1, dRows )
	VERIFY_ROW( "GetRowArea", dRow2, dRows )
	VERIFY_COL( "GetRowArea", dCol1, dCols )
	VERIFY_COL( "GetRowArea", dCol2, dCols )

	VERIFY_RANGE_ORDER( "GetRowArea", dCol1, dCol2 )
	VERIFY_RANGE_ORDER( "GetRowArea", dRow1, dRow2 )

	VERIFY_BUFFER( "GetRowArea", pBuf )
	VERIFY_BPP( "GetRowArea", dBpp )

	rdCount = ( ( dRow2 - dRow1 ) == 0 ? 1 : ( dRow2 - dRow1 ) );

	try
	{
		pfAreaBuf = new float[ size_t( rdCount ) ];
	}
	catch ( bad_alloc& ba )
	{
		ThrowException( "GetRowArea",
						"Failed to allocate memory for row data! " +
						 string( ba.what() ) );
	}

	for ( int dRow=dRow1, i=0; dRow<dRow2; dRow++, i++ )
	{
		fRowSum = 0;

		for ( int dCol=dCol1; dCol<dCol2; dCol++ )
		{
			fRowSum += ( ( dBpp == CArcImage::BPP16 ) ?
					   ( reinterpret_cast<tU16*>( pBuf )[ dCol + dRow * dCols ] ) :
					   ( reinterpret_cast<tU32*>( pBuf )[ dCol + dRow * dCols ] ) );
		}

		pfAreaBuf[ i ] = fRowSum / ( static_cast<float>( dCol2 - dCol1 ) );
	}

	STORE_FLT_POINTER( pfAreaBuf )

	return pfAreaBuf;
}


// +----------------------------------------------------------------------------
// |  GetColArea
// +----------------------------------------------------------------------------
// |  Returns a column of pixel data where each value is the average over the
// |  specified range of rows.
// |
// |  <IN> -> pBuf    : Pointer to image memory
// |  <IN> -> dRow1   : Start row 
// |  <IN> -> dRow2   : End row
// |  <IN> -> dCol1   : Start column 
// |  <IN> -> dCol2   : End column
// |  <IN> -> dRows	  : Number of rows in full image
// |  <IN> -> dCols   : Number of columns in full image
// |  <IN> -> rdCount : The number of elements in the return buffer
// |  <IN> -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// |
// |  Returns: Pointer to the column data. IMPORTANT: This buffer MUST be freed
// |           by the user calling CArcImage::Free().
// +----------------------------------------------------------------------------
float* CArcImage::GetColArea( void* pBuf, int dRow1, int dRow2, int dCol1, int dCol2,
							  int dRows, int dCols, int& rdCount, int dBpp )
{
	float* pfAreaBuf = NULL;
	float  fColSum   = 0;

	VERIFY_IMG_DIMEN( "GetColArea", dRows )
	VERIFY_IMG_DIMEN( "GetColArea", dCols )

	VERIFY_ROW( "GetColArea", dRow1, dRows )
	VERIFY_ROW( "GetColArea", dRow2, dRows )
	VERIFY_COL( "GetColArea", dCol1, dCols )
	VERIFY_COL( "GetColArea", dCol2, dCols )

	VERIFY_RANGE_ORDER( "GetColArea", dCol1, dCol2 )
	VERIFY_RANGE_ORDER( "GetColArea", dRow1, dRow2 )

	VERIFY_BUFFER( "GetColArea", pBuf )
	VERIFY_BPP( "GetColArea", dBpp )

	rdCount = ( ( dCol2 - dCol1 ) == 0 ? 1 : ( dCol2 - dCol1 ) );

	try
	{
		pfAreaBuf = new float[ size_t( rdCount ) ];
	}
	catch( bad_alloc& ba )
	{
		ThrowException( "GetColArea",
						"Failed to allocate memory for column data! " +
						 string( ba.what() ) );
	}

	for ( int dCol=dCol1, i=0; dCol<dCol2; dCol++, i++ )
	{
		fColSum = 0;

		for ( int dRow=dRow1; dRow<dRow2; dRow++ )
		{
			fColSum += ( ( dBpp == CArcImage::BPP16 ) ?
					   ( reinterpret_cast<tU16*>( pBuf )[ dCol + dRow * dCols ] ) :
					   ( reinterpret_cast<tU32*>( pBuf )[ dCol + dRow * dCols ] ) );
		}

		pfAreaBuf[ i ] = fColSum / ( static_cast<float>( dRow2 - dRow1 ) );
	}

	STORE_FLT_POINTER( pfAreaBuf )

	return pfAreaBuf;
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
// |  <IN> -> pBuf1 : 1st image buffer pointer
// |  <IN> -> pBuf2 : 2nd image buffer pointer
// |  <IN> -> dRow1 : Start row
// |  <IN> -> dRow2 : End row
// |  <IN> -> dCol1 : Start column
// |  <IN> -> dCol2 : End column
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in each full image
// |  <IN> -> dBpp  : The image bits-per-pixel. Default: CArcImage::BPP16
// |
// |  Returns a CArcImage::CArcImageDiffStats structure with all the statistice
// |  filled in. Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
CArcImage::CImgDifStats CArcImage::GetDiffStats( void* pBuf1, void* pBuf2, int dRow1,
												 int dRow2, int dCol1, int dCol2,
												 int dRows, int dCols, int dBpp )
{
	double gSum		= 0.0;
	double gDifSum	= 0.0;
	double gVal1	= 0.0;
	double gVal2	= 0.0;

	VERIFY_IMG_DIMEN( "GetDiffStats", dRows )
	VERIFY_IMG_DIMEN( "GetDiffStats", dCols )

	VERIFY_ROW( "GetDiffStats", dRow1, dRows )
	VERIFY_ROW( "GetDiffStats", dRow2, dRows )
	VERIFY_COL( "GetDiffStats", dCol1, dCols )
	VERIFY_COL( "GetDiffStats", dCol2, dCols )

	VERIFY_RANGE_ORDER( "GetDiffStats", dCol1, dCol2 )
	VERIFY_RANGE_ORDER( "GetDiffStats", dRow1, dRow2 )

	VERIFY_BUFFER( "GetDiffStats", pBuf1 )
	VERIFY_BUFFER( "GetDiffStats", pBuf2 )

	VERIFY_BPP( "GetDiffStats", dBpp )

	if ( dRow1 == dRow2 ) { dRow2++; }
	if ( dCol1 == dCol2 ) { dCol2++; }

	double gTotalPixelCount = ( ( dRow2 - dRow1 ) * ( dCol2 - dCol1 ) );

	CImgDifStats cImgDifStats;

	cImgDifStats.cImg1Stats = GetStats( pBuf1,
										dRow1,
										dRow2,
										dCol1,
										dCol2,
										dRows,
										dCols );

	cImgDifStats.cImg2Stats = GetStats( pBuf2,
										dRow1,
										dRow2,
										dCol1,
										dCol2,
										dRows,
										dCols );

	for ( int i=dRow1; i<dRow2; i++ )
	{
		for ( int j=dCol1; j<dCol2; j++ )
		{
			gVal1 = static_cast<double>(
							( ( dBpp == CArcImage::BPP16 ) ?
							( reinterpret_cast<tU16*>( pBuf1 )[ j + i * dCols ] ) :
							( reinterpret_cast<tU32*>( pBuf1 )[ j + i * dCols ] ) ) );

			gVal2 = static_cast<double>(
							( ( dBpp == CArcImage::BPP16 ) ?
							( reinterpret_cast<tU16*>( pBuf2 )[ j + i * dCols ] ) :
							( reinterpret_cast<tU32*>( pBuf2 )[ j + i * dCols ] ) ) );

			gSum += ( gVal1 - gVal2 );

			gDifSum += ( pow( ( cImgDifStats.cImg2Stats.gMean - gVal2 ) -
					   ( cImgDifStats.cImg1Stats.gMean - gVal1 ), 2 ) );
		}
	}

	cImgDifStats.cImgDiffStats.gMean     = fabs( gSum / gTotalPixelCount );
	cImgDifStats.cImgDiffStats.gVariance = gDifSum / gTotalPixelCount;
	cImgDifStats.cImgDiffStats.gStdDev   = sqrt( cImgDifStats.cImgDiffStats.gVariance );

	return cImgDifStats;
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
// |  <IN> -> pBuf1 : 1st pointer to image buffer
// |  <IN> -> pBuf2 : 2nd pointer to image buffer
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp  : The image bits-per-pixel. Default: CArcImage::BPP16
// |
// |  Returns a CArcImage::CArcImageStats structure with all the statistice
// |  filled in.  Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
CArcImage::CImgDifStats CArcImage::GetDiffStats( void* pBuf1, void* pBuf2, int dRows,
												 int dCols, int dBpp )
{
	return GetDiffStats( pBuf1, pBuf2, 0, dRows, 0, dCols, dRows, dCols, dBpp );
}


// +----------------------------------------------------------------------------
// |  GetStats
// +----------------------------------------------------------------------------
// |  Calculates the image min, max, mean, variance, and standard deviation over
// |  the specified image memory rows and cols.
// |
// |  <IN> -> pBuf	: Pointer to image buffer
// |  <IN> -> dRow1 : Start row
// |  <IN> -> dRow2 : End row
// |  <IN> -> dCol1 : Start column
// |  <IN> -> dCol2 : End column
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp  : The image bits-per-pixel. Default: CArcImage::BPP16
// |
// |  Returns a CArcImage::CArcImageStats structure with all the statistice
// |  filled in.  Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
CArcImage::CImgStats CArcImage::GetStats( void* pBuf, int dRow1, int dRow2, int dCol1,
										  int dCol2, int dRows, int dCols, int dBpp )
{
	double gVal			= 0.0;
	double gSum			= 0.0;
	double gDevSqrdSum	= 0.0;

	VERIFY_IMG_DIMEN( "GetStats", dRows )
	VERIFY_IMG_DIMEN( "GetStats", dCols )

	VERIFY_ROW( "GetStats", dRow1, dRows )
	VERIFY_ROW( "GetStats", dRow2, dRows )
	VERIFY_COL( "GetStats", dCol1, dCols )
	VERIFY_COL( "GetStats", dCol2, dCols )

	VERIFY_RANGE_ORDER( "GetStats", dCol1, dCol2 )
	VERIFY_RANGE_ORDER( "GetStats", dRow1, dRow2 )

	VERIFY_BUFFER( "GetStats", pBuf )
	VERIFY_BPP( "GetStats", dBpp )

	double gMaxBpp = MAX_TYPE_VALUE( dBpp );

	CImgStats cImgStats;

	cImgStats.gMin = gMaxBpp;

	if ( dRow1 == dRow2 ) { dRow2++; }
	if ( dCol1 == dCol2 ) { dCol2++; }

	double gTotalPixelCount = ( ( dRow2 - dRow1 ) *
								( dCol2 - dCol1 ) );

	cImgStats.gTotalPixels = gTotalPixelCount;

	for ( int i=dRow1; i<dRow2; i++ )
	{
		for ( int j=dCol1; j<dCol2; j++ )
		{
			gVal = static_cast<double>(
						( ( dBpp == CArcImage::BPP16 ) ?
						( reinterpret_cast<tU16*>( pBuf )[ j + i * dCols ] ) :
						( reinterpret_cast<tU32*>( pBuf )[ j + i * dCols ] ) ) );

			//
			// Determine min/max values
			//
			if ( gVal < cImgStats.gMin )
			{
				cImgStats.gMin = gVal;
			}

			else if ( gVal > cImgStats.gMax )
			{
				cImgStats.gMax = gVal;
			}

			//
			// Monitor for saturated pixels
			//
			if ( gVal >= MAX_TYPE_VALUE( dBpp ) )
			{
				cImgStats.gSaturatedPixCnt++;
			}

			gSum += gVal;
		}
	}

	// Calculate mean
	cImgStats.gMean = gSum / gTotalPixelCount;

	for ( int i=dRow1; i<dRow2; i++ )
	{
		for ( int j=dCol1; j<dCol2; j++ )
		{
			double gPixVal =
					static_cast<double>( 
							( ( dBpp == CArcImage::BPP16 ) ?
							( reinterpret_cast<tU16*>( pBuf )[ j + i * dCols ] ) :
							( reinterpret_cast<tU32*>( pBuf )[ j + i * dCols ] ) ) );

			gDevSqrdSum += pow( ( gPixVal - cImgStats.gMean ), 2 );
		}
	}

	cImgStats.gVariance = gDevSqrdSum / gTotalPixelCount;
	cImgStats.gStdDev = sqrt( cImgStats.gVariance );

	return cImgStats;
}


// +----------------------------------------------------------------------------
// |  GetStats
// +----------------------------------------------------------------------------
// |  Calculates the image min, max, mean, variance, and standard deviation over
// |  the entire image.
// |
// |  <IN> -> pBuf  : Pointer to image buffer
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp  : The image bits-per-pixel. Default: CArcImage::BPP16
// |
// |  Returns a CArcImage::CArcImageStats structure with all the statistice
// |  filled in.  Throws a std::runtume_error on error.
// +----------------------------------------------------------------------------
CArcImage::CImgStats CArcImage::GetStats( void* pBuf, int dRows, int dCols, int dBpp )
{
	return GetStats( pBuf, 0, dRows, 0, dCols, dRows, dCols, dBpp );
}


// +----------------------------------------------------------------------------
// |  Histogram
// +----------------------------------------------------------------------------
// |  Calculates the histogram over the specified image memory rows and cols.
// |
// |  <OUT> -> rdCount : Number of values in histogram
// |  <IN>  -> pBuf    : Pointer to image memory
// |  <IN>  -> dRow1   : Start row
// |  <IN>  -> dRow2   : End row
// |  <IN>  -> dCol1   : Start column
// |  <IN>  -> dCol2   : End column
// |  <IN>  -> dRows   : Number of rows in full image
// |  <IN>  -> dCols   : Number of columns in full image
// |  <IN>  -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// |
// |  Returns a pointer to an array of elements that represent the histogram.
// |  This data SHOULD NOT be freed by the calling application.
// +----------------------------------------------------------------------------
int* CArcImage::Histogram( int& rdCount, void* pBuf, int dRow1, int dRow2, int dCol1,
						   int dCol2, int dRows, int dCols, int dBpp )
{
	VERIFY_IMG_DIMEN( "Histogram", dRows )
	VERIFY_IMG_DIMEN( "Histogram", dCols )

	VERIFY_ROW( "Histogram", dRow1, dRows )
	VERIFY_ROW( "Histogram", dRow2, dRows )
	VERIFY_COL( "Histogram", dCol1, dCols )
	VERIFY_COL( "Histogram", dCol2, dCols )

	VERIFY_RANGE_ORDER( "Histogram", dCol1, dCol2 )
	VERIFY_RANGE_ORDER( "Histogram", dRow1, dRow2 )

	VERIFY_BUFFER( "Histogram", pBuf )
	VERIFY_BPP( "Histogram", dBpp )

	int  tSize = static_cast<int>( MAX_TYPE_VALUE( dBpp ) );
	int* pHist = NULL;

	try
	{
		pHist = new int[ tSize ];
	}
	catch ( bad_alloc& ba )
	{
		ThrowException( "Histogram",
						"Failed to allocate histogram array! " +
						 string( ba.what() ) );
	}

	rdCount = tSize;

	Arc_ZeroMemory( pHist, size_t( tSize ) * sizeof( int ) );

	if ( dRow1 == dRow2 ) { dRow2++; }
	if ( dCol1 == dCol2 ) { dCol2++; }

	for ( int i=dRow1; i<dRow2; i++ )
	{
		for ( int j=dCol1; j<dCol2; j++ )
		{
			pHist[ ( ( dBpp == CArcImage::BPP16 ) ?
				   ( reinterpret_cast<tU16*>( pBuf )[ j + i * dCols ] ) :
				   ( reinterpret_cast<tU32*>( pBuf )[ j + i * dCols ] ) ) ]++;
		}
	}

	STORE_S32_POINTER( pHist )

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
// |
// |  Returns a pointer to an array of elements that represent the histogram.
// |  This data SHOULD NOT be freed by the calling application.
// +----------------------------------------------------------------------------
int* CArcImage::Histogram( int& rdCount, void* pBuf, int dRows, int dCols, int dBpp )
{
	return Histogram( rdCount, pBuf, 0, dRows, 0, dCols, dRows, dCols, dBpp );
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
// +----------------------------------------------------------------------------
void CArcImage::Add( tU32* pU32Buf, tU16* pU16Buf1, tU16* pU16Buf2, int dRows, int dCols )
{
	VERIFY_IMG_DIMEN( "Add", dRows )
	VERIFY_IMG_DIMEN( "Add", dCols )

	VERIFY_BUFFER( "Add", pU32Buf  )
	VERIFY_BUFFER( "Add", pU16Buf1 )
	VERIFY_BUFFER( "Add", pU16Buf2 )

	for ( int r=0; r<dRows; r++ )
	{
		for ( int c=0; c<dCols; c++ )
		{
			pU32Buf[ c + r * dCols ] =
						pU16Buf1[ c + r * dCols ] + pU16Buf2[ c + r * dCols ];
		}
	}
}


// +----------------------------------------------------------------------------
// |  Subtract
// +----------------------------------------------------------------------------
// |  Subtracts buffer 2 from buffer 1. Buffer 1 is replaced with the new data.
// |
// |  <IN> -> pBuf1 : Pointer to 1st image buffer
// |  <IN> -> pBuf2 : Pointer to 2nd image buffer
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp  : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::Subtract( void* pBuf1, void* pBuf2, int dRows, int dCols, int dBpp )
{
	VERIFY_IMG_DIMEN( "Subtract", dRows )
	VERIFY_IMG_DIMEN( "Subtract", dCols )

	VERIFY_BUFFER( "Subtract", pBuf1 )
	VERIFY_BUFFER( "Subtract", pBuf2 )

	VERIFY_BPP( "Subtract", dBpp )

	//
	// Subtract the images. Subtract pU16Buffer1 from pU16Buffer2.
	//
	if ( dBpp == BPP16 )
	{
		tU16* pU16Buf1 = reinterpret_cast<tU16*>( pBuf1 );
		tU16* pU16Buf2 = reinterpret_cast<tU16*>( pBuf2 );

		for ( int i=0; i<( dRows * dCols ); i++ )
		{
			pU16Buf1[ i ] = pU16Buf2[ i ] - pU16Buf1[ i ];
		}
	}

	else
	{
		tU32* pU32Buf1 = reinterpret_cast<tU32*>( pBuf1 );
		tU32* pU32Buf2 = reinterpret_cast<tU32*>( pBuf2 );

		for ( int i=0; i<( dRows * dCols ); i++ )
		{
			pU32Buf1[ i ] = pU32Buf2[ i ] - pU32Buf1[ i ];
		}
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
// |  <IN> -> pBuf  : Pointer to image buffer
// |  <IN> -> dRows : The number of rows in entire image ( includes both halves )
// |  <IN> -> dCols : The number of cols in the image
// |  <IN> -> dBpp  : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::SubtractHalves( void* pBuf, int dRows, int dCols, int dBpp )
{
	VERIFY_IMG_DIMEN( "SubtractHalves", dRows )
	VERIFY_IMG_DIMEN( "SubtractHalves", dCols )

	VERIFY_BUFFER( "SubtractHalves", pBuf )
	VERIFY_BPP( "SubtractHalves", dBpp )

	if ( ( dRows % 2 ) != 0 )
	{
		ostringstream oss;

		oss << "Image must have an even number of rows [ "
			<< dRows << " ]" << ends;

		ThrowException( "SubtractHalves", oss.str() );
	}

	tU8 *pU8Buf1 = reinterpret_cast<tU8*>( pBuf );

	tU8 *pU8Buf2 = ( reinterpret_cast<tU8*>( pBuf ) +
					TYPE_SIZE_OF( dBpp ) * ( ( dRows * dCols ) / 2 ) );

	Subtract( pU8Buf1, pU8Buf2, ( dRows / 2 ), dCols );
}


// +----------------------------------------------------------------------------
// |  Divide
// +----------------------------------------------------------------------------
// |  Divides buffer 1 by buffer 2. Buffer 1 is replaced with the new data.
// |
// |  <IN> -> pBuf1 : Pointer to 1st image buffer
// |  <IN> -> pBuf2 : Pointer to 2nd image buffer
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp  : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::Divide( void* pBuf1, void* pBuf2, int dRows, int dCols, int dBpp )
{
	VERIFY_IMG_DIMEN( "Divide", dRows )
	VERIFY_IMG_DIMEN( "Divide", dCols )

	VERIFY_BUFFER( "Divide", pBuf1 )
	VERIFY_BUFFER( "Divide", pBuf2 )

	VERIFY_BPP( "Divide", dBpp )

	if ( dBpp == BPP16 )
	{
		tU16* pU16Buf1 = reinterpret_cast<tU16*>( pBuf1 );
		tU16* pU16Buf2 = reinterpret_cast<tU16*>( pBuf2 );

		for ( int r=0; r<dRows; r++ )
		{
			for ( int c=0; c<dCols; c++ )
			{
				if ( pU16Buf2[ c + r * dCols ] != 0 )
				{
					pU16Buf1[ c + r * dCols ] = pU16Buf1[ c + r * dCols ] /
												pU16Buf2[ c + r * dCols ];
				}
				else
				{
					pU16Buf1[ c + r * dCols ] = 0;
				}
			}
		}
	}

	else
	{
		tU32* pU32Buf1 = reinterpret_cast<tU32*>( pBuf1 );
		tU32* pU32Buf2 = reinterpret_cast<tU32*>( pBuf2 );

		for ( int r=0; r<dRows; r++ )
		{
			for ( int c=0; c<dCols; c++ )
			{
				if ( pU32Buf2[ c + r * dCols ] != 0 )
				{
					pU32Buf1[ c + r * dCols ] = pU32Buf1[ c + r * dCols ] /
												pU32Buf2[ c + r * dCols ];
				}
				else
				{
					pU32Buf1[ c + r * dCols ] = 0;
				}
			}
		}
	}
}


// +----------------------------------------------------------------------------
// |  Copy
// +----------------------------------------------------------------------------
// |  Copies the source image buffer to the destination image buffer. The source
// |  buffer must be less than or equal in dimensions to the destination buffer.
// |  These values are checked.
// |
// |  <IN> -> pDstBuf  : Pointer to destination buffer
// |  <IN> -> dDstRows : Number of rows in destination buffer
// |  <IN> -> dDstCols : Number of columns in destination buffer
// |  <IN> -> pSrcBuf  : Pointer to source buffer
// |  <IN> -> dSrcRows : Number of rows in source buffer
// |  <IN> -> dSrcCols : Number of columns in source buffer
// |  <IN> -> dBpp     : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::Copy( void* pDstBuf, int dDstRows, int dDstCols, void* pSrcBuf, int dSrcRows, int dSrcCols, int dBpp )
{
	VERIFY_IMG_DIMEN( "Copy", dDstRows )
	VERIFY_IMG_DIMEN( "Copy", dDstCols )

	VERIFY_IMG_DIMEN( "Copy", dSrcRows )
	VERIFY_IMG_DIMEN( "Copy", dSrcCols )

	if ( dBpp == BPP16 )
	{
		Copy( pDstBuf, ( dDstRows * dDstCols * sizeof( tU16 ) ),
			  pSrcBuf, ( dSrcRows * dSrcCols * sizeof( tU16 ) ) );
	}
	else
	{
		Copy( pDstBuf, ( dDstRows * dDstCols * sizeof( tU32 ) ),
			  pSrcBuf, ( dSrcRows * dSrcCols * sizeof( tU32 ) ) );
	}
}


// +----------------------------------------------------------------------------
// |  Copy
// +----------------------------------------------------------------------------
// |  Copies the source image buffer to the destination image buffer. The source
// |  buffer must be less than or equal in dimension to the destination buffer.
// |  These values are checked. Copy is done via bytes, so BPP doesn't matter.
// |
// |  <IN> -> pDstBuf  : Pointer to destination buffer
// |  <IN> -> dDstSize : Size ( in bytes ) of the destination buffer
// |  <IN> -> pSrcBuf  : Pointer to source buffer
// |  <IN> -> dSrcRows : Size ( in bytes ) of the source buffer
// +----------------------------------------------------------------------------
void CArcImage::Copy( void* pDstBuf, int dDstSize, void* pSrcBuf, int dSrcSize )
{
	VERIFY_BUFFER( "Copy", pDstBuf )
	VERIFY_BUFFER( "Copy", pSrcBuf )

	//  Verify image dimensions
	// +--------------------------------------------+
	if ( dSrcSize > dDstSize )
	{
		ostringstream oss;

		oss << "Source buffer must be less than or equal to destination buffer size!"
			<< endl << "Source size: " << dSrcSize
			<< endl << "Destination size: " << dDstSize
			<< ends;

		ThrowException( "Copy", oss.str() );
	}

	Arc_CopyMemory( pDstBuf, pSrcBuf, dSrcSize );
}


// +----------------------------------------------------------------------------
// |  Fill
// +----------------------------------------------------------------------------
// |  Fills the image memory with the specified value.
// |
// |  <IN> -> pBuf   : Pointer to image buffer
// |  <IN> -> dRows  : Number of rows in full image
// |  <IN> -> dCols  : Number of columns in full image
// |  <IN> -> dValue : The with which to fill the buffer
// |  <IN> -> dBpp   : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::Fill( void* pBuf, int dRows, int dCols, int dValue, int dBpp )
{
	VERIFY_IMG_DIMEN( "Fill", dRows )
	VERIFY_IMG_DIMEN( "Fill", dCols )

	VERIFY_BUFFER( "Fill", pBuf )
	VERIFY_BPP( "Fill", dBpp )

	if ( dValue >= MAX_TYPE_VALUE( dBpp ) )
	{
		ostringstream oss;

		oss << "Invalid value parameter! Value must be less than "
			<< MAX_TYPE_VALUE( dBpp ) << ends;

		ThrowException( "Fill", oss.str() );
	}

	if ( dBpp == BPP16 )
	{
		tU16* pU16Buf = reinterpret_cast<tU16*>( pBuf );

		for ( int r=0; r<dRows; r++ )
		{
			for ( int c=0; c<dCols; c++ )
			{
				pU16Buf[ c + r * dCols ] = static_cast<tU16>( dValue );
			}
		}
	}

	else
	{
		tU32* pU32Buf = reinterpret_cast<tU32*>( pBuf );

		for ( int r=0; r<dRows; r++ )
		{
			for ( int c=0; c<dCols; c++ )
			{
				pU32Buf[ c + r * dCols ] = static_cast<tU32>( dValue );
			}
		}
	}
}


// +----------------------------------------------------------------------------
// |  GradientFill
// +----------------------------------------------------------------------------
// |  Fills the image memory with a gradient pattern.
// |
// |  <IN> -> pBuf  : Pointer to image buffer
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp   : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::GradientFill( void *pBuf, int dRows, int dCols, int dBpp )
{
	int dValue = 0;

	VERIFY_IMG_DIMEN( "GradientFill", dRows )
	VERIFY_IMG_DIMEN( "GradientFill", dCols )

	VERIFY_BUFFER( "GradientFill", pBuf )
	VERIFY_BPP( "GradientFill", dBpp )

	Arc_ZeroMemory( pBuf, ( TYPE_SIZE_OF( dBpp ) * ( dRows * dCols ) ) );

	if ( dBpp == BPP16 )
	{
		tU16* pU16Buf = reinterpret_cast<tU16*>( pBuf );

		for ( int r=0; r<dRows; r++ )
		{
			for ( int c=0; c<dCols; c++ )
			{
				pU16Buf[ c + r * dCols ] = static_cast<tU16>( dValue );
			}

			dValue += static_cast<tU16>( T_SIZE( tU16 ) / dRows );
		}
	}

	else
	{
		tU32* pU32Buf = reinterpret_cast<tU32*>( pBuf );

		for ( int r=0; r<dRows; r++ )
		{
			for ( int c=0; c<dCols; c++ )
			{
				pU32Buf[ c + r * dCols ] = static_cast<tU32>( dValue );
			}

			dValue += static_cast<tU32>( T_SIZE( tU32 ) / dRows );
		}
	}
}


// +----------------------------------------------------------------------------
// |  SmileyFill
// +----------------------------------------------------------------------------
// |  Fills the image memory with zeroes and puts a smiley face at the center.
// |
// |  <IN> -> pBuf  : Pointer to image buffer
// |  <IN> -> dRows : Number of rows in full image
// |  <IN> -> dCols : Number of columns in full image
// |  <IN> -> dBpp  : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::SmileyFill( void* pBuf, int dRows, int dCols, int dBpp )
{
	VERIFY_IMG_DIMEN( "SmileyFill", dRows )
	VERIFY_IMG_DIMEN( "SmileyFill", dCols )

	VERIFY_BUFFER( "SmileyFill", pBuf )
	VERIFY_BPP( "SmileyFill", dBpp )

	Arc_ZeroMemory( pBuf, ( TYPE_SIZE_OF( dBpp ) * ( dRows * dCols ) ) );

	int radius = min( ( dRows / 2 ), ( dCols / 2 ) ) - 10;

	//  Draw Head
	// +---------------------------------------------------------------------------- +
	DrawGradientFillCircle( ( dCols / 2 ),
							( dRows / 2 ),
							 radius,
							 dRows,
							 dCols,
							 pBuf );

	//  Draw Left Eye
	// +---------------------------------------------------------------------------- +
	int rowFactor = int( radius / 2.5 );

	DrawFillCircle( ( dCols / 2 ) - rowFactor,
					( dRows / 2 ) + rowFactor,
					( radius / 5 ),
					 dCols,
					 pBuf );

	//  Draw Right Eye
	// +---------------------------------------------------------------------------- +
	DrawFillCircle( ( dCols / 2 ) + rowFactor,
					( dRows / 2 ) + rowFactor,
					( radius / 5 ),
					 dCols,
					 pBuf );

	//  Draw Mouth
	// +---------------------------------------------------------------------------- +
	for ( int i=0; i<( radius / 2 ); i++ )
	{
		DrawSemiCircle( ( dCols / 2 ),
						( dRows / 2 ) - ( rowFactor / 2 ),
						 i,
						 180,
						 360,
						 dCols,
						 pBuf );
	}
}

//void CArcImage::LogoFill( void* pBuf, int dRows, int dCols, int dBpp )
//{
//	VERIFY_IMG_DIMEN( "LogoFill", dRows )
//	VERIFY_IMG_DIMEN( "LogoFill", dCols )
//
//	VERIFY_BUFFER( "LogoFill", pBuf )
//	VERIFY_BPP( "LogoFill", dBpp )
//
//	Fill( pBuf, dRows, dCols, 65000 );
//
//	int radius = min( ( dRows / 2 ), ( dCols / 2 ) ) - 10;
//
//	int colBase = ( ( dCols / 2 ) - ( 500 / 2 ) );
//
//	// +-----------------------------------------------------------------------------+
//	// |  Draw the rainbow
//	// +-----------------------------------------------------------------------------+
//	for ( int i=300; i<350; i++ )
//	{
//		DrawSemiCircle( ( dCols / 2 ),
//						( dRows - ( dRows / 4 ) ),
//						 i,
//						 0,
//						 180,
//						 dCols,
//						 pBuf );
//	}
//
//	// +-----------------------------------------------------------------------------+
//	// |  Draw the 'A'
//	// +-----------------------------------------------------------------------------+
//	int dRowStart = 500;
//	int dColStart = colBase;
//
//	for ( int j=0; j<50; j++ )
//	{
//		for ( int i=( dRowStart + j ), rl=dColStart, rr=dColStart; i>( dRowStart - 150 + j ); i--, rl--, rr++ )
//		{
//			static_cast<tU16*>( pBuf )[ rl + i * dCols ] = 0;
//			static_cast<tU16*>( pBuf )[ rr + i * dCols ] = 0;
//		}
//	}
//
//	for ( int j=0; j<50; j++ )
//	{
//		for ( int i=( dRowStart - 100 ); i>( dRowStart - 250 ); i-- )
//		{
//			static_cast<tU16*>( pBuf )[ ( dColStart + 149 - j ) + i * dCols ] = 0;	//350
//			static_cast<tU16*>( pBuf )[ ( dColStart - 149 + j ) + i * dCols ] = 0;	//50
//		}
//	}
//
//	for ( int j=0; j<50; j++ )
//	{
//		for ( int i=( dColStart - 149 + j ); i<( dColStart + 150 - j ); i++ )
//		{
//			static_cast<tU16*>( pBuf )[ i + ( dRowStart - 100 + j ) * dCols ] = 0;
//		}
//	}
//
//	// +-----------------------------------------------------------------------------+
//	// |  Draw the 'R'
//	// +-----------------------------------------------------------------------------+
//	dRowStart = 550;
//	dColStart = colBase + 200;
//
//	for ( int j=0; j<50; j++ )
//	{
//		for ( int i=dRowStart; i>( dRowStart - 300 ); i-- )
//		{
//			static_cast<tU16*>( pBuf )[ ( dColStart + j ) + i * dCols ] = 0;
//		}
//	}
//
//	for ( int i=( radius / 9 ); i<( radius / 5 ); i++ )
//	{
//		DrawSemiCircle( dColStart + 50,
//						dRowStart - ( radius / 5 ),
//						 i,
//						 0,
//						 90,
//						 dCols,
//						 pBuf );
//	}
//
//	for ( int i=( radius / 9 ); i<( radius / 5 ); i++ )
//	{
//		DrawSemiCircle( dColStart + 50,
//						dRowStart - ( radius / 5 ),
//						 i,
//						 270,
//						 360,
//						 dCols,
//						 pBuf );
//	}
//
//	dRowStart = dRowStart - 2 * ( radius / 5 );
//
//	for ( int j=0; j<50; j++ )
//	{
//		for ( int i=( dRowStart + j ), rr=( dColStart + 50 ); i>( dRowStart - 100 ); i--, rr++ )
//		{
//			static_cast<tU16*>( pBuf )[ rr + i * dCols ] = 0;
//		}
//	}
//
//	// +-----------------------------------------------------------------------------+
//	// |  Draw the 'C'
//	// +-----------------------------------------------------------------------------+
//	dRowStart = 500;
//	dColStart = colBase + 550;
//
//	for ( int i=( radius / 4 ); i<( radius / 3 ); i++ )
//	{
//		DrawSemiCircle( dColStart + 50,
//						dRowStart - ( radius / 5 ),
//						 i,
//						 90,
//						 270,
//						 dCols,
//						 pBuf );
//	}
//}


// +----------------------------------------------------------------------------
// |  VerifyAsSynthetic
// +----------------------------------------------------------------------------
// |  Verifies that the kernel buffer contains a valid ramp synthetic image
// |  ( as generated by the controller ).
// |
// |  NOTE: The PCI DSP firmware has an issue where img[ 0 ] = 1 up to 65535,
// |  then, img[ 65536 ] = 0, where it should be 1. So this method takes this
// |  into account by reading and setting compareValue to the first pixel for
// |  each new 65535 pixel segment.
// |
// |  Throws: std::runtime_error on error
// |
// |  <IN> -> pBuf  : Pointer to image buffer
// |  <IN> -> dRows : The number of rows in the synthetic image.
// |  <IN> -> dCols : The number of columns in the synthetic image.
// |  <IN> -> dBpp  : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::VerifyAsSynthetic( void* pBuf, int dRows, int dCols, int dBpp )
{
	VERIFY_IMG_DIMEN( "VerifyAsSynthetic", dRows )
	VERIFY_IMG_DIMEN( "VerifyAsSynthetic", dCols )

	VERIFY_BUFFER( "VerifyAsSynthetic", pBuf )
	VERIFY_BPP( "VerifyAsSynthetic", dBpp )

	tU32 tU32CompareValue = 
					static_cast<tU32>(
							( ( dBpp == CArcImage::BPP16 ) ?
							( reinterpret_cast<tU16*>( pBuf )[ 0 ] ) :
							( reinterpret_cast<tU32*>( pBuf )[ 0 ] ) ) );

	for ( int row=0; row<dRows; row++ )
	{
		for ( int col=0; col<dCols; col++ )
		{
			tU32 tU32BufVal =
						static_cast<tU32>(
								( ( dBpp == CArcImage::BPP16 ) ?
								( reinterpret_cast<tU16*>( pBuf )[ col + row * dCols ] ) :
								( reinterpret_cast<tU32*>( pBuf )[ col + row * dCols ] ) ) );

			if ( tU32BufVal != tU32CompareValue )
			{
				ostringstream oss;

				oss << "Synthetic image check failed at: "
					<< endl << "row: " << ( row + 1 )
					<< "  col: " << ( col + 1 ) << " ( mem[ "
					<< ( col + row * dCols ) << " ] )" << endl
					<< "Found value: " << tU32BufVal
					<< " Expected value: " << tU32CompareValue
					<< ends;

				ThrowException( "VerifyAsSynthetic", oss.str() );
			}

			tU32CompareValue++;

			if ( tU32CompareValue >= MAX_TYPE_VALUE( dBpp ) )
			{
				tU32CompareValue =
						static_cast<tU32>(
								( ( dBpp == CArcImage::BPP16 ) ?
								( reinterpret_cast<tU16*>( pBuf )[ col + row * dCols + 1 ] ) :
								( reinterpret_cast<tU32*>( pBuf )[ col + row * dCols + 1 ] ) ) );
			}
		}
	}
}


// +----------------------------------------------------------------------------
// |  DrawCircle
// +----------------------------------------------------------------------------
// |  Draws a circle on the specified image buffer.
// |
// |  Throws: std::runtime_error on error
// |
// |  <IN> -> xCenter : x position of circle center point
// |  <IN> -> yCenter : y position of circle center point
// |  <IN> -> radius  : The radius of the circle
// |  <IN> -> dCols   : Number of columns in full image
// |  <IN> -> pBuf    : Pointer to image buffer
// |  <IN> -> dColor  : Color to draw circle ( default = 0 )
// |  <IN> -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::DrawCircle( int xCenter, int yCenter, int radius, int dCols,
							void* pBuf, int dColor, int dBpp )
{
	VERIFY_BUFFER( "DrawCircle", pBuf )

	VERIFY_BPP( "DrawCircle", dBpp )

	if ( dBpp == CArcImage::BPP16 )
	{
		for ( double angle=0; angle<360; angle+=0.1 )
		{
			int x = int( radius * cos( angle * DEG2RAD ) + xCenter );
			int y = int( radius * sin( angle * DEG2RAD ) + yCenter );

			reinterpret_cast<tU16*>
			( pBuf )[ x + y * dCols ] = static_cast<tU16>( dColor );
		}
	}

	else
	{
		for ( double angle=0; angle<360; angle+=0.1 )
		{
			int x = int( radius * cos( angle * DEG2RAD ) + xCenter );
			int y = int( radius * sin( angle * DEG2RAD ) + yCenter );

			reinterpret_cast<tU32*>
			( pBuf )[ x + y * dCols ] = static_cast<tU32>( dColor );
		}
	}
}

// +----------------------------------------------------------------------------
// |  DrawFillCircle
// +----------------------------------------------------------------------------
// |  Draws a filled circle on the specified memory buffer.
// |
// |  Throws: std::runtime_error on error
// |
// |  <IN> -> xCenter : x position of circle center point
// |  <IN> -> yCenter : y position of circle center point
// |  <IN> -> radius  : The radius of the circle
// |  <IN> -> dCols   : Number of columns in full image
// |  <IN> -> pBuf    : Pointer to image buffer
// |  <IN> -> dColor  : Color to draw circle ( default = 0 )
// |  <IN> -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::DrawFillCircle( int xCenter, int yCenter, int radius, int dCols,
								void* pBuf, int dColor, int dBpp )
{
	VERIFY_BUFFER( "DrawFillCircle", pBuf )

	VERIFY_BPP( "DrawFillCircle", dBpp )

	if ( dBpp == CArcImage::BPP16 )
	{
		for ( int r=0; r<radius; r++ )
		{
			for ( double angle=0; angle<360; angle+=0.1 )
			{
				int x = int( r * cos( angle * DEG2RAD ) + xCenter );
				int y = int( r * sin( angle * DEG2RAD ) + yCenter );

				reinterpret_cast<tU16*>
				( pBuf )[ x + y * dCols ] = static_cast<tU16>( dColor );
			}
		}
	}

	else
	{
		for ( int r=0; r<radius; r++ )
		{
			for ( double angle=0; angle<360; angle+=0.1 )
			{
				int x = int( r * cos( angle * DEG2RAD ) + xCenter );
				int y = int( r * sin( angle * DEG2RAD ) + yCenter );

				reinterpret_cast<tU32*>
				( pBuf )[ x + y * dCols ] = static_cast<tU32>( dColor );
			}
		}
	}
}


// +----------------------------------------------------------------------------
// |  DrawGradientFillCircle
// +----------------------------------------------------------------------------
// |  Draws a gradient filled circle on the specified memory buffer.
// |
// |  Throws: std::runtime_error on error
// |
// |  <IN> -> xCenter : x position of circle center point
// |  <IN> -> yCenter : y position of circle center point
// |  <IN> -> radius  : The radius of the circle
// |  <IN> -> dCols   : Number of columns in full image
// |  <IN> -> pBuf    : Pointer to image buffer
// |  <IN> -> dBpp    : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::DrawGradientFillCircle( int xCenter, int yCenter, int radius,
										int dRows, int dCols, void* pBuf, int dBpp )
{
	VERIFY_BUFFER( "DrawGradientFillCircle", pBuf )

	VERIFY_BPP( "DrawGradientFillCircle", dBpp )

	for ( int r=0; r<radius; r++ )
	{
		DrawCircle( ( dCols / 2 ),
					( dRows / 2 ),
					( radius - r ),
					 dCols,
					 pBuf,
					 static_cast<int>
						( r + ( ( MAX_TYPE_VALUE( dBpp ) - 1 ) / radius ) ) );
	}
}


// +----------------------------------------------------------------------------
// |  DrawSemiCircle
// +----------------------------------------------------------------------------
// |  Draws a semi-circle on the specified memory buffer.
// |
// |  Throws: std::runtime_error on error
// |
// |  <IN> -> xCenter     : x position of semi-circle center point
// |  <IN> -> yCenter     : y position of semi-circle center point
// |  <IN> -> radius      : The radius of the semi-circle
// |  <IN> -> gStartAngle : The start angle of the semi-circle
// |  <IN> -> gEndAngle   : The end angle of the semi-circle
// |  <IN> -> dCols       : Number of columns in full image
// |  <IN> -> pBuf        : Pointer to image buffer
// |  <IN> -> dColor      : Color to draw circle ( default = 0 )
// |  <IN> -> dBpp        : The image bits-per-pixel. Default: CArcImage::BPP16
// +----------------------------------------------------------------------------
void CArcImage::DrawSemiCircle( int xCenter, int yCenter, int radius,
							    double gStartAngle, double gEndAngle, int dCols,
								void* pBuf, int dColor, int dBpp )
{
	VERIFY_BUFFER( "DrawSemiCircle", pBuf )

	VERIFY_BPP( "DrawSemiCircle", dBpp )

	if ( dBpp == CArcImage::BPP16 )
	{
//		for ( double angle=180; angle<360; angle+=0.1 )
		for ( double angle=gStartAngle; angle<gEndAngle; angle+=0.1 )
		{
			int x = int( radius * cos( angle * DEG2RAD ) + xCenter );
			int y = int( radius * sin( angle * DEG2RAD ) + yCenter );

			reinterpret_cast<tU16*>
			( pBuf )[ x + y * dCols ] = static_cast<tU16>( dColor );
		}
	}

	else
	{
//		for ( double angle=180; angle<360; angle+=0.1 )
		for ( double angle=gStartAngle; angle<gEndAngle; angle+=0.1 )
		{
			int x = int( radius * cos( angle * DEG2RAD ) + xCenter );
			int y = int( radius * sin( angle * DEG2RAD ) + yCenter );

			reinterpret_cast<tU32*>
			( pBuf )[ x + y * dCols ] = static_cast<tU32>( dColor );
		}
	}
}


// +----------------------------------------------------------------------------
// |  ThrowException
// +----------------------------------------------------------------------------
// |  Method to throw a "const char *" exception.
// |
// |  <IN> -> sMethodName : The method where the exception occurred.
// |  <IN> -> sMsg        : A std::string to use for the exception message.
// +----------------------------------------------------------------------------
void CArcImage::ThrowException( std::string sMethodName, std::string sMsg )
{
	ostringstream oss;

	oss << "( CArcImage::"
		<< ( sMethodName.empty() ? "???" : sMethodName )
		<< "() ): "
		<< sMsg
		<< ends;

	throw std::runtime_error( ( const string )oss.str() );
}
