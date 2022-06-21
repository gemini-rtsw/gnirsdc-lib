// ArcDeinterlaceJAPI.cpp : Defines the exported functions for the DLL application.
//

#ifdef __APPLE__
	#include <JavaVM/jni.h>
#else
	#include <jni.h>
#endif

#include "arc_api_deinterlace_ArcDeinterlaceJAPI.h"
#include "CArcDeinterlace.h"

#include <exception>
#include <sstream>
#include <string>
#include <cstdio>

using namespace std;
using namespace arc::deinterlace;


// +-----------------------------------------------------------------------------------------+
// | Private Macros                                                                          |
// +-----------------------------------------------------------------------------------------+
#define GET_INSTANCE()																\
		if ( g_pCDLace.get() == NULL )												\
		{																			\
			g_pCDLace.reset( new CArcDeinterlace() );								\
		}																			\
																					\
		if ( g_pCDLace.get() == NULL )												\
		{																			\
			throw runtime_error( "Failed to create CArcDeinterlace instance!" );	\
		}

// +-----------------------------------------------------------------------------------------+
// | Private Functions                                                                       |
// +-----------------------------------------------------------------------------------------+
void ThrowJNIException( JNIEnv* pEnv, string sMethod, string sMsg );
void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod );


// +-----------------------------------------------------------------------------------------+
// | Globals                                                                                 |
// +-----------------------------------------------------------------------------------------+
static auto_ptr<CArcDeinterlace> g_pCDLace( 0 );


// +-----------------------------------------------------------------------------------------+
// | API Functions                                                                           |
// +-----------------------------------------------------------------------------------------+

/*
 * Class:     arc_api_deinterlace_ArcDeinterlaceJAPI
 * Method:    GetAPIConstants
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_arc_api_deinterlace_ArcDeinterlaceJAPI_GetAPIConstants
( JNIEnv* pEnv, jclass clazz )
{
	jfieldID fid;

	try
	{
		fid = pEnv->GetStaticFieldID( clazz, "DEINTERLACE_NONE", "I" );
		if ( fid == 0 ) { throw runtime_error( "DEINTERLACE_NONE field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcDeinterlace::DEINTERLACE_NONE ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "DEINTERLACE_PARALLEL", "I" );
		if ( fid == 0 ) { throw runtime_error( "DEINTERLACE_PARALLEL field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcDeinterlace::DEINTERLACE_PARALLEL ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "DEINTERLACE_SERIAL", "I" );
		if ( fid == 0 ) { throw runtime_error( "DEINTERLACE_SERIAL field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcDeinterlace::DEINTERLACE_SERIAL ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "DEINTERLACE_CCD_QUAD", "I" );
		if ( fid == 0 ) { throw runtime_error( "DEINTERLACE_CCD_QUAD field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcDeinterlace::DEINTERLACE_CCD_QUAD ); }

		fid = pEnv->GetStaticFieldID( clazz, "DEINTERLACE_IR_QUAD", "I" );
		if ( fid == 0 ) { throw runtime_error( "DEINTERLACE_IR_QUAD field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcDeinterlace::DEINTERLACE_IR_QUAD ); }

		fid = pEnv->GetStaticFieldID( clazz, "DEINTERLACE_CDS_IR_QUAD", "I" );
		if ( fid == 0 ) { throw runtime_error( "DEINTERLACE_CDS_IR_QUAD field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcDeinterlace::DEINTERLACE_CDS_IR_QUAD ); }

		fid = pEnv->GetStaticFieldID( clazz, "DEINTERLACE_HAWAII_RG", "I" );
		if ( fid == 0 ) { throw runtime_error( "DEINTERLACE_HAWAII_RG field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcDeinterlace::DEINTERLACE_HAWAII_RG ); }

		fid = pEnv->GetStaticFieldID( clazz, "DEINTERLACE_STA1600", "I" );
		if ( fid == 0 ) { throw runtime_error( "DEINTERLACE_STA1600 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcDeinterlace::DEINTERLACE_STA1600 ); }

		fid = pEnv->GetStaticFieldID( clazz, "DEINTERLACE_CUSTOM", "I" );
		if ( fid == 0 ) { throw runtime_error( "DEINTERLACE_CUSTOM field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcDeinterlace::DEINTERLACE_CUSTOM ); }
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetAPIConstants", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetAPIConstants" );
	}
}

/*
 * Class:     arc_api_deinterlace_ArcDeinterlaceJAPI
 * Method:    RunAlg
 * Signature: (JIII)V
 */
JNIEXPORT void JNICALL Java_arc_api_deinterlace_ArcDeinterlaceJAPI_RunAlg__JIII
( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRows, jint jCols, jint jAlgorithm )
{
	try
	{
		GET_INSTANCE()

		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );

		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image data buffer is NULL!" );
		}

		g_pCDLace.get()->RunAlg( pU8Buf,
								 int( jRows ),
								 int( jCols ),
								 int( jAlgorithm ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "RunAlg", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "RunAlg" );
	}
}

/*
 * Class:     arc_api_deinterlace_ArcDeinterlaceJAPI
 * Method:    RunAlg
 * Signature: (JIIII)V
 */
JNIEXPORT void JNICALL Java_arc_api_deinterlace_ArcDeinterlaceJAPI_RunAlg__JIIII
( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRows, jint jCols, jint jPixelOffset, jint jAlgorithm )
{
	try
	{
		GET_INSTANCE()

		unsigned char* pU16Buf = reinterpret_cast<unsigned char *>( jBufferVA ) + int( jPixelOffset );

		if ( pU16Buf == NULL )
		{
			throw runtime_error( "Image data buffer is NULL!" );
		}

		g_pCDLace.get()->RunAlg( pU16Buf,
								 int( jRows ),
								 int( jCols ),
								 int( jAlgorithm ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "RunAlg", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "RunAlg" );
	}
}

/*
 * Class:     arc_api_deinterlace_ArcDeinterlaceJAPI
 * Method:    RunAlg
 * Signature: (JIIIII)V
 */
JNIEXPORT void JNICALL Java_arc_api_deinterlace_ArcDeinterlaceJAPI_RunAlg__JIIIII
( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRows, jint jCols, jint jPixelOffset, jint jAlgorithm, jint jArg )
{
	try
	{
		GET_INSTANCE()

		unsigned char* pU16Buf = reinterpret_cast<unsigned char *>( jBufferVA ) + int( jPixelOffset );

		if ( pU16Buf == NULL )
		{
			throw runtime_error( "Image data buffer is NULL!" );
		}

		g_pCDLace.get()->RunAlg( pU16Buf,
								 int( jRows ),
								 int( jCols ),
								 int( jAlgorithm ),
								 int( jArg ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "RunAlg", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "RunAlg" );
	}
}

/*
 * Class:     arc_api_deinterlace_ArcDeinterlaceJAPI
 * Method:    GetCustomAlgorithms
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_arc_api_deinterlace_ArcDeinterlaceJAPI_GetCustomAlgorithms
( JNIEnv* pEnv, jclass clazz, jstring jLibPath )
{
	jobjectArray	jStrObjArray		= NULL;
	jclass			jStringClass		= NULL;
	jmethodID		jStringCtor			= NULL;

	const char *pszLibPath = ( jLibPath == NULL ? NULL : pEnv->GetStringUTFChars( jLibPath, 0 ) );

	try
	{
		if ( pszLibPath == NULL )
		{
			throw runtime_error( "Failed to allocate memory for library path name!" );
		}

		GET_INSTANCE()

		if ( !g_pCDLace.get()->FindCustomLibrary( pszLibPath ) )
		{
			return jStrObjArray;
		}

		int dCustomCount = g_pCDLace.get()->GetCustomCount();

		if ( dCustomCount > 0 )
		{
			//  Find the java.util.TreeMap class and its constructor
			// +--------------------------------------------------------------------------+
			jStringClass = pEnv->FindClass( "java/lang/String" );

			if ( jStringClass == NULL || pEnv->ExceptionOccurred() != NULL )
			{
				throw runtime_error( "Failed to find java.lang.String class!" );
			}

			jStringCtor = pEnv->GetMethodID( jStringClass, "<init>", "()V" );

			if ( jStringCtor == NULL || pEnv->ExceptionOccurred() != NULL )
			{
				throw runtime_error( "Failed to find java.lang.String class constructor!" );
			}

			jStrObjArray = pEnv->NewObjectArray( dCustomCount,
											    jStringClass,
											    pEnv->NewStringUTF( "" ) );

			if ( jStrObjArray == NULL || pEnv->ExceptionOccurred() != NULL )
			{
				throw runtime_error( "Failed to create new string object array!" );
			}

			string	sCustomName	= "";
			int		dAlgorithm	= 0;

			//  Loop over the algorithms and add them to the object array
			// +--------------------------------------------------------------------------+
			for ( int i=0; i<dCustomCount; i++ )
			{
				g_pCDLace.get()->GetCustomInfo( i, dAlgorithm, sCustomName );

				pEnv->SetObjectArrayElement( jStrObjArray,
											i,
											pEnv->NewStringUTF( sCustomName.c_str() ) );
			}
		}	// end if count > 0
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCustomAlgorithms", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCustomAlgorithms" );
	}

	if ( pszLibPath != NULL )
	{
		pEnv->ReleaseStringUTFChars( jLibPath, pszLibPath );
	}

	return jStrObjArray;
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

		oss << "( ArcDeinterlaceJAPI::" << sMethod << " ): " << sMsg << ends;

		pEnv->ThrowNew( newExcCls, oss.str().c_str() );
	}
}

void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod )
{
	jclass newExcCls = pEnv->FindClass( "java/lang/Exception" );
	
	if ( newExcCls != NULL )
	{
		pEnv->ThrowNew( newExcCls,
					   ( "( ArcDeinterlaceJAPI::" + sMethod +
					   " ): An unknown error caused an exception!" ).c_str() );
	}
}
