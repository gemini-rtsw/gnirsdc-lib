// ArcDisplayJAPI.cpp : Defines the exported functions for the DLL application.
//

#ifdef __APPLE__
	#include <JavaVM/jni.h>
#else
	#include <jni.h>
#endif

#include "arc_api_display_ArcDisplayJAPI.h"
#include "CArcDisplay.h"

#include <exception>
#include <memory>
#include <string>
#include <sstream>

using namespace std;
using namespace arc::display;


// +-----------------------------------------------------------------------------------------+
// | Private Macros                                                                          |
// +-----------------------------------------------------------------------------------------+
#define GET_INSTANCE()															\
		if ( g_pCDisplay.get() == NULL )										\
		{																		\
			g_pCDisplay.reset( new CArcDisplay() );								\
		}																		\
																				\
		if ( g_pCDisplay.get() == NULL )										\
		{																		\
			throw runtime_error( "Failed to create CArcDisplay instance!" );	\
		}


// +-----------------------------------------------------------------------------------------+
// | Private Functions                                                                       |
// +-----------------------------------------------------------------------------------------+
void ThrowJNIException( JNIEnv* pEnv, string sMethod, string sMsg );
void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod );


// +-----------------------------------------------------------------------------------------+
// | Globals                                                                                 |
// +-----------------------------------------------------------------------------------------+
static auto_ptr<CArcDisplay> g_pCDisplay( 0 );


// +-----------------------------------------------------------------------------------------+
// | API Functions                                                                           |
// +-----------------------------------------------------------------------------------------+

/*
 * Class:     arc_api_display_ArcDisplayJAPI
 * Method:    LaunchDisplay
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_arc_api_display_ArcDisplayJAPI_LaunchDisplay( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		GET_INSTANCE()

		g_pCDisplay.get()->Launch();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "LaunchDisplay", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "LaunchDisplay" );
	}
}

/*
 * Class:     arc_api_display_ArcDisplayJAPI
 * Method:    TerminateDisplay
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_arc_api_display_ArcDisplayJAPI_TerminateDisplay( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		if ( g_pCDisplay.get() != NULL )
		{
			g_pCDisplay.get()->Terminate();
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "TerminateDisplay", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "TerminateDisplay" );
	}
}

/*
 * Class:     arc_api_display_ArcDisplayJAPI
 * Method:    DisplayImage
 * Signature: (JII)V
 */
JNIEXPORT void JNICALL Java_arc_api_display_ArcDisplayJAPI_DisplayImage
( JNIEnv* pEnv, jclass clazz, jlong jBufferVA, jint jRows, jint jCols )
{
	try
	{
		GET_INSTANCE()

		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		g_pCDisplay.get()->Clear();

		g_pCDisplay.get()->Show( pU8Buf, int( jRows ), int( jCols ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "DisplayImage", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "DisplayImage" );
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

		oss << "( ArcDisplayJAPI::" << sMethod << " ): " << sMsg << ends;

		pEnv->ThrowNew( newExcCls, oss.str().c_str() );
	}
}

void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod )
{
	jclass newExcCls = pEnv->FindClass( "java/lang/Exception" );
	
	if ( newExcCls != NULL )
	{
		pEnv->ThrowNew( newExcCls,
					   ( "( ArcDisplayJAPI::" + sMethod +
					   " ): An unknown error caused an exception!" ).c_str() );
	}
}
