// ArcImageJAPI.cpp : Defines the exported functions for the DLL application.
//

#ifdef __APPLE__
	#include <JavaVM/jni.h>
#else
	#include <jni.h>
#endif

#include "arc_api_image_ArcImageJAPI.h"
#include "CArcImage.h"

#include <exception>
#include <stdexcept>
#include <sstream>
#include <string>
#include <memory>

using namespace std;
using namespace arc::image;


// +-----------------------------------------------------------------------------------------+
// | Private Macros                                                                          |
// +-----------------------------------------------------------------------------------------+
#define GET_INSTANCE()															\
		if ( g_pCArcImg.get() == NULL )											\
		{																		\
			g_pCArcImg.reset( new CArcImage() );								\
		}																		\
																				\
		if ( g_pCArcImg.get() == NULL )											\
		{																		\
			throw runtime_error( "Failed to create CArcImage instance!" );		\
		}


// +-----------------------------------------------------------------------------------------+
// | Private Functions                                                                       |
// +-----------------------------------------------------------------------------------------+
void ThrowJNIException( JNIEnv* pEnv, string sMethod, string sMsg );
void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod );


// +-----------------------------------------------------------------------------------------+
// | Globals                                                                                 |
// +-----------------------------------------------------------------------------------------+
static auto_ptr<CArcImage> g_pCArcImg( 0 );


// +-----------------------------------------------------------------------------------------+
// | API Functions                                                                           |
// +-----------------------------------------------------------------------------------------+

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    Histogram
 * Signature: (JII)[Ljava/awt/geom/Point2D/Float;
 */
JNIEXPORT jobjectArray JNICALL
Java_arc_api_image_ArcImageJAPI_Histogram__JII( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRows, jint jCols )
{
	jobjectArray jHistArr  = NULL;
	int*         pHist     = NULL;
	int          dHistSize = 0;
	int          dIndex    = 0;

	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()

		pHist = g_pCArcImg.get()->Histogram( dHistSize,
											 pU8Buf,
											 int( jRows ),
											 int( jCols ),
											 CArcImage::BPP16 );

		jclass pointClass = pEnv->FindClass( "java/awt/geom/Point2D$Float" );
				
		if ( pointClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class!" );
		}
			
		jmethodID pointConstructor = pEnv->GetMethodID( pointClass, "<init>", "(FF)V" );
				
		if ( pointConstructor == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class constructor!" );
		}
			
		jHistArr = pEnv->NewObjectArray( dHistSize, pointClass, NULL );
				
		if ( jHistArr == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to create new java object array!" );
		}
			
		for ( int i=0; i<dHistSize; i++ )
		{
			jobject pointObj = pEnv->NewObject( pointClass,
												pointConstructor,
												float( i ),
												float( pHist[ i ] ) );

			if ( pointObj == NULL || pEnv->ExceptionOccurred() != NULL )
			{
				throw runtime_error( "Failed to create new java Point2D.Float object!" );
			}
						
			pEnv->SetObjectArrayElement( jHistArr, dIndex, pointObj );

			dIndex++;
		}
	}
	catch ( exception& e )
	{
		ThrowJNIException( pEnv, "Histogram", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Histogram" );
	}

	g_pCArcImg.get()->Free( pHist );
	
	return jHistArr;
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    Histogram
 * Signature: (JIIIIII)[Ljava/awt/geom/Point2D/Float;
 */
JNIEXPORT jobjectArray JNICALL Java_arc_api_image_ArcImageJAPI_Histogram__JIIIIII
( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRow1, jint jRow2, jint jCol1, jint jCol2, jint jRows, jint jCols )
{
	jobjectArray jHistArr  = NULL;
	int*         pHist     = NULL;
	int          dHistSize = 0;
	int          dIndex    = 0;

	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()

		pHist = g_pCArcImg.get()->Histogram( dHistSize,
											 pU8Buf,
											 int( jRow1 ),
											 int( jRow2 ),
											 int( jCol1 ),
											 int( jCol2 ),
											 int( jRows ),
											 int( jCols ),
											 CArcImage::BPP16 );

		int dElemCount = dHistSize;	// Force there to be 65535 elements for X-Y plotting
			
		jclass pointClass = pEnv->FindClass( "java/awt/geom/Point2D$Float" );
				
		if ( pointClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class!" );
		}
			
		jmethodID pointConstructor = pEnv->GetMethodID( pointClass, "<init>", "(FF)V" );
				
		if ( pointConstructor == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class constructor!" );
		}
			
		jHistArr = pEnv->NewObjectArray( dElemCount, pointClass, NULL );
				
		if ( jHistArr == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to create new java object array!" );
		}
			
		for ( int i=0; i<dHistSize; i++ )
		{
			jobject pointObj = pEnv->NewObject( pointClass,
												pointConstructor,
												float( i ),
												float( pHist[ i ] ) );

			if ( pointObj == NULL || pEnv->ExceptionOccurred() != NULL )
			{
				throw runtime_error( "Failed to create new java Point2D.Float object!" );
			}
						
			pEnv->SetObjectArrayElement( jHistArr, dIndex, pointObj );

			dIndex++;
		}
	}
	catch ( exception& e )
	{
		ThrowJNIException( pEnv, "Histogram", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Histogram" );
	}
			
	g_pCArcImg.get()->Free( pHist );
	
	return jHistArr;
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    GetRow
 * Signature: (JIIIII)[Ljava/awt/geom/Point2D/Float;
 */
JNIEXPORT jobjectArray JNICALL Java_arc_api_image_ArcImageJAPI_GetRow
( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRow, jint jCol1, jint jCol2, jint jRows, jint jCols )
{
	jobjectArray    jPointArr   = NULL;
	unsigned short*	pBuf		= NULL;
	int             dElemCount  = 0;
	int             dIndex      = 0;
	CArcImage       cImage;

	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()

		pBuf = ( unsigned short * )g_pCArcImg.get()->GetRow( pU8Buf,
															 int( jRow ),
															 int( jCol1 ),
															 int( jCol2 ),
															 int( jRows ),
															 int( jCols ),
															 dElemCount );
		if ( pBuf == NULL )
		{
			throw runtime_error( "Failed to allocate memory for image row data!" );
		}

		jclass pointClass = pEnv->FindClass( "java/awt/geom/Point2D$Float" );
			
		if ( pointClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class!" );
		}
			
		jmethodID pointConstructor = pEnv->GetMethodID( pointClass, "<init>", "(FF)V" );
			
		if ( pointConstructor == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class constructor!" );
		}
			
		jPointArr = pEnv->NewObjectArray( jsize( dElemCount ), pointClass, NULL );
			
		if ( jPointArr == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to create new java object array!" );
		}
			
		for ( int i=0; i<dElemCount; i++ )
		{
			jobject pointObj = pEnv->NewObject( pointClass,
												pointConstructor,
												float( jCol1 + i ),
												float( pBuf[ i ] ) );
				
			if ( pointObj == NULL || pEnv->ExceptionOccurred() != NULL )
			{
				throw runtime_error( "Failed to create new java Point2D.Float object!" );
			}
				
			pEnv->SetObjectArrayElement( jPointArr, dIndex, pointObj );
			dIndex++;
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetRow", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetRow" );
	}

	g_pCArcImg.get()->Free( reinterpret_cast<unsigned char *>( pBuf ) );

	return jPointArr;
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    GetCol
 * Signature: (JIIIII)[Ljava/awt/geom/Point2D/Float;
 */
JNIEXPORT jobjectArray JNICALL Java_arc_api_image_ArcImageJAPI_GetCol
( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jCol, jint jRow1, jint jRow2, jint jRows, jint jCols )
{
	jobjectArray	jPointArr  = NULL;
	unsigned short*	pBuf		= NULL;
	int				dElemCount = 0;
	int				dIndex     = 0;

	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()

		pBuf = ( unsigned short * )
						g_pCArcImg.get()->GetCol( pU8Buf,
												  int( jCol ),
												  int( jRow1 ),
												  int( jRow2 ),
												  int( jRows ),
												  int( jCols ),
												  dElemCount );
		if ( pBuf == NULL )
		{
			throw runtime_error( "Failed to allocate memory for image column data!" );
		}

		jclass pointClass = pEnv->FindClass( "java/awt/geom/Point2D$Float" );
			
		if ( pointClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class!" );
		}
			
		jmethodID pointConstructor = pEnv->GetMethodID( pointClass, "<init>", "(FF)V" );
			
		if ( pointConstructor == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class constructor!" );
		}
			
		jPointArr = pEnv->NewObjectArray( jsize( dElemCount ), pointClass, NULL );
			
		if ( jPointArr == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to create new java object array!" );
		}
			
		for ( int i=0; i<dElemCount; i++ )
		{
			jobject pointObj = pEnv->NewObject( pointClass,
												pointConstructor,
												float( jRow1 + i ),
												float( pBuf[ i ] ) );
				
			if ( pointObj == NULL || pEnv->ExceptionOccurred() != NULL )
			{
				throw runtime_error( "Failed to create new java Point2D.Float object!" );
			}
				
			pEnv->SetObjectArrayElement( jPointArr, dIndex, pointObj );

			dIndex++;
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCol", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCol" );
	}

	g_pCArcImg.get()->Free( reinterpret_cast<unsigned char *>( pBuf ) );

	return jPointArr;
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    GetRowArea
 * Signature: (JIIIIII)[Ljava/awt/geom/Point2D/Float;
 */
JNIEXPORT jobjectArray JNICALL Java_arc_api_image_ArcImageJAPI_GetRowArea
( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRow1, jint jRow2, jint jCol1, jint jCol2, jint jRows, jint jCols )
{
	jobjectArray    jPointArr   = NULL;
	int             dElemCount  = 0;
	int             dIndex      = 0;
	float*			pBuf		= NULL;

	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()

		pBuf = ( float * )g_pCArcImg.get()->GetRowArea( pU8Buf,
													    int( jRow1 ),
														int( jRow2 ),
														int( jCol1 ),
														int( jCol2 ),
														int( jRows ),
														int( jCols ),
														dElemCount );
		if ( pBuf == NULL )
		{
			throw runtime_error( "Failed to allocate memory for image row data!" );
		}
				
		jclass pointClass = pEnv->FindClass( "java/awt/geom/Point2D$Float" );
				
		if ( pointClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class!" );
		}
				
		jmethodID pointConstructor = pEnv->GetMethodID( pointClass, "<init>", "(FF)V" );
				
		if ( pointConstructor == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class constructor!" );
		}
				
		jPointArr = pEnv->NewObjectArray( jsize( dElemCount ), pointClass, NULL );
				
		if ( jPointArr == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to create new java object array!" );
		}
				
		for ( int i=0; i<dElemCount; i++ )
		{
			jobject pointObj = pEnv->NewObject( pointClass,
												pointConstructor,
												float( jRow1 + i ),
												float( pBuf[ i ] ) );

			if ( pointObj == NULL || pEnv->ExceptionOccurred() != NULL )
			{
				throw runtime_error( "Failed to create new java Point2D.Float object!" );
			}
					
			pEnv->SetObjectArrayElement( jPointArr, dIndex, pointObj );

			dIndex++;
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetRowArea", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetRowArea" );
	}

	g_pCArcImg.get()->Free( pBuf );

	return jPointArr;
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    GetColArea
 * Signature: (JIIIIII)[Ljava/awt/geom/Point2D/Float;
 */
JNIEXPORT jobjectArray JNICALL Java_arc_api_image_ArcImageJAPI_GetColArea
( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRow1, jint jRow2, jint jCol1, jint jCol2, jint jRows, jint jCols )
{
	jobjectArray    jPointArr   = NULL;
	int             dElemCount  = 0;
	int             dIndex      = 0;
	float*			pBuf		= NULL;

	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()

		pBuf = ( float * )g_pCArcImg.get()->GetColArea( pU8Buf,
														int( jRow1 ),
														int( jRow2 ),
														int( jCol1 ),
														int( jCol2 ),
														int( jRows ),
														int( jCols ),
														dElemCount );
		if ( pBuf == NULL )
		{
			throw runtime_error( "Failed to allocate memory for image column data!" );
		}
				
		jclass pointClass = pEnv->FindClass( "java/awt/geom/Point2D$Float" );
				
		if ( pointClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class!" );
		}
				
		jmethodID pointConstructor = pEnv->GetMethodID( pointClass, "<init>", "(FF)V" );
				
		if ( pointConstructor == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to find java Point2D.Float class constructor!" );
		}
				
		jPointArr = pEnv->NewObjectArray( jsize( dElemCount ), pointClass, NULL );
				
		if ( jPointArr == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to create new java object array!" );
		}

		for ( int i=0; i<dElemCount; i++ )
		{
			jobject pointObj = pEnv->NewObject( pointClass,
												pointConstructor,
												float( jCol1 + i ),
												float( pBuf[ i ] ) );
					
			if ( pointObj == NULL || pEnv->ExceptionOccurred() != NULL )
			{
				throw runtime_error( "Failed to create new java Point2D.Float object!" );
			}
					
			pEnv->SetObjectArrayElement( jPointArr, dIndex, pointObj );

			dIndex++;
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetColArea", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetColArea" );
	}

	g_pCArcImg.get()->Free( pBuf );

	return jPointArr;
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    GetStats
 * Signature: (JII)Larc/api/image/ImageStats;
 */
JNIEXPORT jobject JNICALL
Java_arc_api_image_ArcImageJAPI_GetStats__JII( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRows, jint jCols )
{
	jobject jImgStatsObj = NULL;
	CArcImage::CImgStats cImgStats;

	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()

		//  Calculate stats
		// +-----------------------------------------------------+
		cImgStats = g_pCArcImg.get()->GetStats( pU8Buf, int( jRows ), int( jCols ) );

		jclass jImgStatClass = pEnv->FindClass( "arc/api/image/ImageStats" );
			
		if ( jImgStatClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to find java ImageStats class!" );
		}
			
		jmethodID jImgStatInitMthd = pEnv->GetMethodID( jImgStatClass, "<init>", "()V" );
			
		if ( jImgStatInitMthd == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to find java ImageStats class constructor!" );
		}
			
		jImgStatsObj = pEnv->NewObject( jImgStatClass, jImgStatInitMthd );
			
		if ( jImgStatsObj == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to create new java ImageStats object!" );
		}
			
		//  Image statistics
		// +-------------------------------------------------------------------------------------+
		jfieldID jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMin", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gMin );
			
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMax", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gMax );
			
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMean", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gMean );
			
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gVariance", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gVariance );
			
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gStdDev", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gStdDev );
			
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gSaturatedPixCnt", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gSaturatedPixCnt );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetStats", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetStats" );
	}

	return jImgStatsObj;
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    GetStats
 * Signature: (JIIIIII)Larc/api/image/ImageStats;
 */
JNIEXPORT jobject JNICALL Java_arc_api_image_ArcImageJAPI_GetStats__JIIIIII
( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRow1, jint jRow2, jint jCol1, jint jCol2, jint jRows, jint jCols )
{
	jobject jImgStatsObj = NULL;
	CArcImage::CImgStats cImgStats;

	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()

		//  Calculate stats
		// +-----------------------------------------------------+
		cImgStats = g_pCArcImg.get()->GetStats( pU8Buf,
											    int( jRow1 ),
											    int( jRow2 ),
											    int( jCol1 ),
											    int( jCol2 ),
											    int( jRows ),
											    int( jCols ) );

		jclass jImgStatClass = pEnv->FindClass( "arc/api/image/ImageStats" );
			
		if ( jImgStatClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to find java ImageStats class!" );
		}
			
		jmethodID jImgStatInitMthd = pEnv->GetMethodID( jImgStatClass, "<init>", "()V" );
			
		if ( jImgStatInitMthd == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to find java ImageStats class constructor!" );
		}
			
		jImgStatsObj = pEnv->NewObject( jImgStatClass, jImgStatInitMthd );
			
		if ( jImgStatsObj == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to create new java ImageStats object!" );
		}
			
		//  Image statistics
		// +-------------------------------------------------------------------------------------+
		jfieldID jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMin", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gMin );
			
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMax", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gMax );
			
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMean", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gMean );
			
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gVariance", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gVariance );
			
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gStdDev", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gStdDev );
			
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gSaturatedPixCnt", "D" );
		pEnv->SetDoubleField( jImgStatsObj, jImgFieldId, cImgStats.gSaturatedPixCnt );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetStats", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetStats" );
	}

	return jImgStatsObj;
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    GetDifStats
 * Signature: (JJII)Larc/api/image/ImageDifStats;
 */
JNIEXPORT jobject JNICALL Java_arc_api_image_ArcImageJAPI_GetDifStats__JJII
( JNIEnv* pEnv, jclass clazz, jlong jBuffer1VA, jlong jBuffer2VA, jint jRows, jint jCols )
{
	jobject jImgDifStatsObj = NULL;
	CArcImage::CImgDifStats cImgStats;


	try
	{
		unsigned char* pU8Buf1 = reinterpret_cast<unsigned char *>( jBuffer1VA );
		unsigned char* pU8Buf2 = reinterpret_cast<unsigned char *>( jBuffer2VA );
			
		if ( pU8Buf1 == NULL )
		{
			throw runtime_error( "Image buffer #1 is NULL!" );
		}

		if ( pU8Buf2 == NULL )
		{
			throw runtime_error( "Image buffer #2 is NULL!" );
		}

		GET_INSTANCE()
			
		cImgStats = g_pCArcImg.get()->GetDiffStats( pU8Buf1,
													pU8Buf2,
													int( jRows ),
													int( jCols ) );

		//  Get access to Owl's image statistics classes
		// +-------------------------------------------------------------------------------------+
		jclass jImgStatClass = pEnv->FindClass( "arc/api/image/ImageStats" );
		
		if ( jImgStatClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to find java ImageStats class!" );
		}
		
		jclass jImgDifStatClass = pEnv->FindClass( "arc/api/image/ImageDifStats" );
		
		if ( jImgDifStatClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to find java ImageDifStats class!" );
		}
		
		jmethodID jImgStatInitMthd = pEnv->GetMethodID( jImgDifStatClass, "<init>", "()V" );
		
		if ( jImgStatInitMthd == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to find java ImageDifStats class constructor!" );
		}
		
		jImgDifStatsObj = pEnv->NewObject( jImgDifStatClass, jImgStatInitMthd );
		
		if ( jImgDifStatsObj == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to create new java ImageDifStats object!" );
		}
		
		//  Image #1 statistics
		// +-------------------------------------------------------------------------------------+
		jfieldID jImgId = pEnv->GetFieldID( jImgDifStatClass, "img1Stats", "Larc/api/image/ImageStats;" );
		jobject jImgObj = pEnv->GetObjectField( jImgDifStatsObj, jImgId );
		
		jfieldID jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMin", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gMin );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMax", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gMax );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMean", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gMean );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gVariance", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gVariance );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gStdDev", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gStdDev );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gSaturatedPixCnt", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gSaturatedPixCnt );
		
		//  Image #2 statistics
		// +-------------------------------------------------------------------------------------+
		jImgId = pEnv->GetFieldID( jImgDifStatClass, "img2Stats", "Larc/api/image/ImageStats;" );
		jImgObj = pEnv->GetObjectField( jImgDifStatsObj, jImgId );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMin", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gMin );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMax", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gMax );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMean", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gMean );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gVariance", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gVariance );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gStdDev", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gStdDev );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gSaturatedPixCnt", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gSaturatedPixCnt );
		
		//  Image Dif statistics
		// +-------------------------------------------------------------------------------------+
		jImgId = pEnv->GetFieldID( jImgDifStatClass, "imgDifStats", "Larc/api/image/ImageStats;" );
		jImgObj = pEnv->GetObjectField( jImgDifStatsObj, jImgId );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMin", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImgDiffStats.gMin );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMax", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImgDiffStats.gMax );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMean", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImgDiffStats.gMean );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gVariance", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImgDiffStats.gVariance );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gStdDev", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImgDiffStats.gStdDev );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetDifStats", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetDifStats" );
	}
	
	return jImgDifStatsObj;
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    GetDifStats
 * Signature: (JJIIIIII)Larc/api/image/ImageDifStats;
 */
JNIEXPORT jobject JNICALL Java_arc_api_image_ArcImageJAPI_GetDifStats__JJIIIIII
( JNIEnv* pEnv, jclass clazz, jlong jBuffer1VA, jlong jBuffer2VA, jint jRow1,
  jint jRow2, jint jCol1, jint jCol2, jint jRows, jint jCols )
{
	jobject jImgDifStatsObj = NULL;
	CArcImage::CImgDifStats cImgStats;


	try
	{
		unsigned char* pU8Buf1 = reinterpret_cast<unsigned char *>( jBuffer1VA );
		unsigned char* pU8Buf2 = reinterpret_cast<unsigned char *>( jBuffer2VA );
			
		if ( pU8Buf1 == NULL )
		{
			throw runtime_error( "Image buffer #1 is NULL!" );
		}

		if ( pU8Buf2 == NULL )
		{
			throw runtime_error( "Image buffer #2 is NULL!" );
		}

		GET_INSTANCE()
			
		cImgStats = g_pCArcImg.get()->GetDiffStats( pU8Buf1,
													pU8Buf2,
													int( jRow1 ),
													int( jRow2 ),
													int( jCol1 ),
													int( jCol2 ),
													int( jRows ),
													int( jCols ) );

		//  Get access to Owl's image statistics classes
		// +-------------------------------------------------------------------------------------+
		jclass jImgStatClass = pEnv->FindClass( "arc/api/image/ImageStats" );
		
		if ( jImgStatClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to find java ImageStats class!" );
		}
		
		jclass jImgDifStatClass = pEnv->FindClass( "arc/api/image/ImageDifStats" );
		
		if ( jImgDifStatClass == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to find java ImageDifStats class!" );
		}
		
		jmethodID jImgStatInitMthd = pEnv->GetMethodID( jImgDifStatClass, "<init>", "()V" );
		
		if ( jImgStatInitMthd == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to find java ImageDifStats class constructor!" );
		}
		
		jImgDifStatsObj = pEnv->NewObject( jImgDifStatClass, jImgStatInitMthd );
		
		if ( jImgDifStatsObj == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw std::runtime_error( "Failed to create new java ImageDifStats object!" );
		}
		
		//  Image #1 statistics
		// +-------------------------------------------------------------------------------------+
		jfieldID jImgId = pEnv->GetFieldID( jImgDifStatClass, "img1Stats", "Larc/api/image/ImageStats;" );
		jobject jImgObj = pEnv->GetObjectField( jImgDifStatsObj, jImgId );
		
		jfieldID jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMin", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gMin );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMax", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gMax );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMean", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gMean );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gVariance", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gVariance );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gStdDev", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gStdDev );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gSaturatedPixCnt", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg1Stats.gSaturatedPixCnt );
		
		//  Image #2 statistics
		// +-------------------------------------------------------------------------------------+
		jImgId = pEnv->GetFieldID( jImgDifStatClass, "img2Stats", "Larc/api/image/ImageStats;" );
		jImgObj = pEnv->GetObjectField( jImgDifStatsObj, jImgId );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMin", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gMin );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMax", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gMax );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMean", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gMean );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gVariance", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gVariance );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gStdDev", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gStdDev );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gSaturatedPixCnt", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImg2Stats.gSaturatedPixCnt );
		
		//  Image Dif statistics
		// +-------------------------------------------------------------------------------------+
		jImgId = pEnv->GetFieldID( jImgDifStatClass, "imgDifStats", "Larc/api/image/ImageStats;" );
		jImgObj = pEnv->GetObjectField( jImgDifStatsObj, jImgId );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMin", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImgDiffStats.gMin );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMax", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImgDiffStats.gMax );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gMean", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImgDiffStats.gMean );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gVariance", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImgDiffStats.gVariance );
		
		jImgFieldId = pEnv->GetFieldID( jImgStatClass, "gStdDev", "D" );
		pEnv->SetDoubleField( jImgObj, jImgFieldId, cImgStats.cImgDiffStats.gStdDev );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetDifStats", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetDifStats" );
	}
	
	return jImgDifStatsObj;
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    Add
 * Signature: (JJJII)V
 */
JNIEXPORT void JNICALL Java_arc_api_image_ArcImageJAPI_Add
( JNIEnv* pEnv, jclass clazz, jlong jU32Buffer, jlong jBuffer1VA, jlong jBuffer2VA, jint jRows, jint jCols )
{
	try
	{
		unsigned int*   pU32Buf  = reinterpret_cast<unsigned int *>( jU32Buffer );
		unsigned short* pU16Buf1 = reinterpret_cast<unsigned short *>( jBuffer1VA );
		unsigned short* pU16Buf2 = reinterpret_cast<unsigned short *>( jBuffer2VA );

		if ( pU32Buf == NULL )
		{
			throw runtime_error( "32-bit image buffer is NULL!" );
		}

		if ( pU16Buf1 == NULL )
		{
			throw runtime_error( "Image buffer #1 is NULL!" );
		}

		if ( pU16Buf2 == NULL )
		{
			throw runtime_error( "Image buffer #2 is NULL!" );
		}

		GET_INSTANCE()

		g_pCArcImg.get()->Add( pU32Buf,
							   pU16Buf1,
							   pU16Buf2,
							   int( jRows ),
							   int( jCols ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Add", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Add" );
	}
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    Subtract
 * Signature: (JJII)V
 */
JNIEXPORT void JNICALL Java_arc_api_image_ArcImageJAPI_Subtract
( JNIEnv* pEnv, jclass clazz, jlong jBuffer1VA, jlong jBuffer2VA, jint jRows, jint jCols )
{
	try
	{
		unsigned char* pU8Buf1 = reinterpret_cast<unsigned char *>( jBuffer1VA );
		unsigned char* pU8Buf2 = reinterpret_cast<unsigned char *>( jBuffer2VA );
			
		if ( pU8Buf1 == NULL )
		{
			throw runtime_error( "Image buffer #1 is NULL!" );
		}

		if ( pU8Buf2 == NULL )
		{
			throw runtime_error( "Image buffer #2 is NULL!" );
		}

		GET_INSTANCE()
			
		g_pCArcImg.get()->Subtract( pU8Buf1, pU8Buf2, int( jRows ), int( jCols ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Subtract", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Subtract" );
	}
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    SubtractHalves
 * Signature: (JII)V
 */
JNIEXPORT void JNICALL
Java_arc_api_image_ArcImageJAPI_SubtractHalves( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRows, jint jCols )
{
	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()
			
		g_pCArcImg.get()->SubtractHalves( pU8Buf, int( jRows ), int( jCols ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SubtractHalves", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SubtractHalves" );
	}
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    Divide
 * Signature: (JJII)V
 */
JNIEXPORT void JNICALL
Java_arc_api_image_ArcImageJAPI_Divide( JNIEnv* pEnv, jclass clazz, jlong jBuffer1VA, jlong jBuffer2VA, jint jRows, jint jCols )
{
	try
	{
		unsigned char* pU8Buf1 = reinterpret_cast<unsigned char *>( jBuffer1VA );
		unsigned char* pU8Buf2 = reinterpret_cast<unsigned char *>( jBuffer2VA );
			
		if ( pU8Buf1 == NULL )
		{
			throw runtime_error( "Image buffer #1 is NULL!" );
		}

		if ( pU8Buf2 == NULL )
		{
			throw runtime_error( "Image buffer #2 is NULL!" );
		}

		GET_INSTANCE()
			
		g_pCArcImg.get()->Divide( pU8Buf1, pU8Buf2, int( jRows ), int( jCols ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Divide", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Divide" );
	}
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    Copy
 * Signature: (JIIJII)V
 */
JNIEXPORT void JNICALL Java_arc_api_image_ArcImageJAPI_Copy__JIIJII
( JNIEnv* pEnv, jclass clazz, jlong jDstBufVA, jint jDstRows, jint jDstCols, jlong jSrcBufVA, jint jSrcRows, jint jSrcCols )
{
	try
	{
		unsigned char* pU8DstBuf = reinterpret_cast<unsigned char *>( jDstBufVA );
		unsigned char* pU8SrcBuf = reinterpret_cast<unsigned char *>( jSrcBufVA );
			
		if ( pU8DstBuf == NULL )
		{
			throw runtime_error( "Destination buffer is NULL!" );
		}
		
		if ( pU8SrcBuf == NULL )
		{
			throw runtime_error( "Source buffer is NULL!" );
		}

		GET_INSTANCE()

		g_pCArcImg.get()->Copy( pU8DstBuf,
								int( jDstRows ),
								int( jDstCols ),
								pU8SrcBuf,
								int( jSrcRows ),
								int( jSrcCols ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Copy", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Copy" );
	}
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    Copy
 * Signature: (JIJI)V
 */
JNIEXPORT void JNICALL Java_arc_api_image_ArcImageJAPI_Copy__JIJI
( JNIEnv* pEnv, jclass clazz, jlong jDstBufVA, jint jDstSize, jlong jSrcBufVA, jint jSrcSize )
{
	try
	{
		unsigned char* pU8DstBuf = reinterpret_cast<unsigned char *>( jDstBufVA );
		unsigned char* pU8SrcBuf = reinterpret_cast<unsigned char *>( jSrcBufVA );
			
		if ( pU8DstBuf == NULL )
		{
			throw runtime_error( "Destination buffer is NULL!" );
		}
		
		if ( pU8SrcBuf == NULL )
		{
			throw runtime_error( "Source buffer is NULL!" );
		}

		GET_INSTANCE()

		g_pCArcImg.get()->Copy( pU8DstBuf,
								int( jDstSize ),
								pU8SrcBuf,
								int( jSrcSize ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Copy", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Copy" );
	}
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    Fill
 * Signature: (JIII)V
 */
JNIEXPORT void JNICALL
Java_arc_api_image_ArcImageJAPI_Fill( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRows, jint jCols, jint jValue )
{
	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()
			
		g_pCArcImg.get()->Fill( pU8Buf, int( jRows ), int( jCols ), int( jValue ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Fill", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Fill" );
	}
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    GradientFill
 * Signature: (JII)V
 */
JNIEXPORT void JNICALL
Java_arc_api_image_ArcImageJAPI_GradientFill( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRows, jint jCols )
{
	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()
			
		g_pCArcImg.get()->GradientFill( pU8Buf, int( jRows ), int( jCols ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GradientFill", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GradientFill" );
	}
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    SmileyFill
 * Signature: (JII)V
 */
JNIEXPORT void JNICALL
Java_arc_api_image_ArcImageJAPI_SmileyFill( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRows, jint jCols )
{
	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()
			
		g_pCArcImg.get()->SmileyFill( pU8Buf, int( jRows ), int( jCols ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SmileyFill", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SmileyFill" );
	}
}

/*
 * Class:     arc_api_image_ArcImageJAPI
 * Method:    VerifyAsSynthetic
 * Signature: (JII)V
 */
JNIEXPORT void JNICALL
Java_arc_api_image_ArcImageJAPI_VerifyAsSynthetic( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRows, jint jCols )
{
	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		GET_INSTANCE()
			
		g_pCArcImg.get()->VerifyAsSynthetic( pU8Buf, int( jRows ), int( jCols ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "VerifyAsSynthetic", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "VerifyAsSynthetic" );
	}
}

// +-----------------------------------------------------------------------------------------+
// | PRIVATE INTERNAL FUNCTIONS ONLY                                                         |
// +-----------------------------------------------------------------------------------------+
void ThrowJNIException( JNIEnv* pEnv, string sMethod, string sMsg )
{
	jclass newExcCls = pEnv->FindClass( "java/lang/Exception" );
	
	if ( newExcCls != NULL )
	{
		ostringstream oss;

		oss << "( ArcImageJAPI::" << sMethod << " ): " << sMsg << ends;

		pEnv->ThrowNew( newExcCls, oss.str().c_str() );
	}
}

void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod )
{
	jclass newExcCls = pEnv->FindClass( "java/lang/Exception" );
	
	if ( newExcCls != NULL )
	{
		pEnv->ThrowNew( newExcCls,
					   ( "( ArcImageJAPI::" + sMethod +
					   " ): An unknown error caused an exception!" ).c_str() );
	}
}
