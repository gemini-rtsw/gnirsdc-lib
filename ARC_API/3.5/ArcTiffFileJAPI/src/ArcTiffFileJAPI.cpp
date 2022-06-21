// ArcTiffFileJAPI.cpp : Defines the exported functions for the DLL application.
//

#ifdef __APPLE__
	#include <JavaVM/jni.h>
#else
	#include <jni.h>
#endif

#include "arc_api_tiff_ArcTiffFileJAPI.h"
#include "CArcTiffFile.h"

#ifdef WIN32
	#include <BaseTsd.h>
#else
	#include <cstdint>
#endif

#include <exception>
#include <sstream>
#include <string>
#include <memory>
#include <map>

using namespace std;
using namespace arc::tiff;


// +-----------------------------------------------------------------------------------------+
// | Private Functions                                                                       |
// +-----------------------------------------------------------------------------------------+
void ThrowJNIException( JNIEnv* pEnv, string sMethod, string sMsg );
void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod );


// +-----------------------------------------------------------------------------------------+
// | Typedefs                                                                                |
// +-----------------------------------------------------------------------------------------+
typedef std::shared_ptr<CArcTiffFile> SharedTiffPtr;

#ifndef WIN32
	typedef uintptr_t INT_PTR;
#endif


// +-----------------------------------------------------------------------------------------+
// | Globals                                                                                 |
// +-----------------------------------------------------------------------------------------+
static map<long,SharedTiffPtr> g_tiffMap;


// +-----------------------------------------------------------------------------------------+
// | API Functions                                                                           |
// +-----------------------------------------------------------------------------------------+

/*
 * Class:     arc_api_tiff_ArcTiffFileJAPI
 * Method:    GetAPIConstants
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_arc_api_tiff_ArcTiffFileJAPI_GetAPIConstants( JNIEnv* pEnv, jclass clazz )
{
	jfieldID fid;

	try
	{
		fid = pEnv->GetStaticFieldID( clazz, "READMODE", "I" );
		if ( fid == 0 ) { throw runtime_error( "READMODE field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcTiffFile::READMODE ); }

		fid = pEnv->GetStaticFieldID( clazz, "WRITEMODE", "I" );
		if ( fid == 0 ) { throw runtime_error( "WRITEMODE field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcTiffFile::WRITEMODE ); }

		fid = pEnv->GetStaticFieldID( clazz, "BPP8", "I" );
		if ( fid == 0 ) { throw runtime_error( "BPP8 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcTiffFile::BPP8 ); }

		fid = pEnv->GetStaticFieldID( clazz, "BPP16", "I" );
		if ( fid == 0 ) { throw runtime_error( "BPP16 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcTiffFile::BPP16 ); }	
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
 * Class:     arc_api_tiff_ArcTiffFileJAPI
 * Method:    WriteFile
 * Signature: (Ljava/lang/String;JIII)V
 */
JNIEXPORT void JNICALL Java_arc_api_tiff_ArcTiffFileJAPI_WriteFile
( JNIEnv* pEnv, jclass clazz, jstring jFileName, jlong jBufferVA, jint jRows, jint jCols, jint jBpp )
{
	// Get the FITS filename
	const char* pszFileName =
					( jFileName != NULL ? pEnv->GetStringUTFChars( jFileName, 0 ) : NULL );

	try
	{
		if ( pszFileName == NULL )
		{
			throw runtime_error( "Failed to convert filename parameter!" );
		}

		unsigned short* pU16Buf = reinterpret_cast<unsigned short *>( jBufferVA );
			
		if ( pU16Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		std::shared_ptr<CArcTiffFile> pCTiff( new CArcTiffFile( pszFileName ) );

		pCTiff.get()->Write( pU16Buf,
							 int( jRows ),
							 int( jCols ),
							 int( jBpp ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "WriteFile", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "WriteFile" );
	}

	if ( pszFileName != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFileName, pszFileName );
	}
}


/*
 * Class:     arc_api_tiff_ArcTiffFileJAPI
 * Method:    Create
 * Signature: (Ljava/lang/String;I)J
 */
JNIEXPORT jlong JNICALL
Java_arc_api_tiff_ArcTiffFileJAPI_Create( JNIEnv* pEnv, jclass clazz, jstring jFileName, jint jMode )
{
	INT_PTR dRetVal = 0;

	// Get the FITS filename
	const char* pszFileName =
					( jFileName != NULL ? pEnv->GetStringUTFChars( jFileName, 0 ) : NULL );

	try
	{
		if ( pszFileName == NULL )
		{
			throw runtime_error( "Failed to convert filename parameter!" );
		}

		std::shared_ptr<CArcTiffFile> pCTiff( new CArcTiffFile( pszFileName, int( jMode ) ) );

		g_tiffMap.insert(
					pair<long,SharedTiffPtr>(
								reinterpret_cast<long>( pCTiff.get() ), pCTiff ) ); 

		dRetVal = INT_PTR( pCTiff.get() );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Create", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Create" );
	}

	if ( pszFileName != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFileName, pszFileName );
	}

	return jlong( dRetVal );
}


/*
 * Class:     arc_api_tiff_ArcTiffFileJAPI
 * Method:    Close
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_arc_api_tiff_ArcTiffFileJAPI_Close( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid TIFF file handle" );
		}

		map<long,SharedTiffPtr>::iterator it;

		it = g_tiffMap.find( INT_PTR( jFileHandle ) );

		if ( it != g_tiffMap.end() )
		{
			g_tiffMap[ INT_PTR( jFileHandle ) ].reset();

			g_tiffMap.erase( it );
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Close", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Close" );
	}
}

/*
 * Class:     arc_api_tiff_ArcTiffFileJAPI
 * Method:    GetRows
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_tiff_ArcTiffFileJAPI_GetRows( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	int dRows = 0;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid TIFF file handle" );
		}

		dRows = g_tiffMap[ INT_PTR( jFileHandle ) ].get()->GetRows();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetRows", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetRows" );
	}

	return static_cast<jint>( dRows );
}

/*
 * Class:     arc_api_tiff_ArcTiffFileJAPI
 * Method:    GetCols
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_tiff_ArcTiffFileJAPI_GetCols( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	int dCols = 0;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid TIFF file handle" );
		}

		dCols = g_tiffMap[ INT_PTR( jFileHandle ) ].get()->GetCols();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCols", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCols" );
	}

	return static_cast<jint>( dCols );
}

/*
 * Class:     arc_api_tiff_ArcTiffFileJAPI
 * Method:    GetBpp
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_tiff_ArcTiffFileJAPI_GetBpp( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	int dBpp = 0;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid TIFF file handle" );
		}

		dBpp = g_tiffMap[ INT_PTR( jFileHandle ) ].get()->GetBpp();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetBpp", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetBpp" );
	}

	return static_cast<jint>( dBpp );
}

/*
 * Class:     arc_api_tiff_ArcTiffFileJAPI
 * Method:    Write
 * Signature: (JJIII)V
 */
JNIEXPORT void JNICALL Java_arc_api_tiff_ArcTiffFileJAPI_Write
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle, jlong jBufferVA, jint jRows, jint jCols, jint jBpp )
{
	try
	{
		unsigned short* pU16Buf = reinterpret_cast<unsigned short *>( jBufferVA );
			
		if ( pU16Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid TIFF file handle" );
		}

		g_tiffMap[ INT_PTR( jFileHandle ) ].get()->Write( pU16Buf,
														  int( jRows ),
														  int( jCols ),
														  int( jBpp ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Write", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Write" );
	}
}

/*
 * Class:     arc_api_tiff_ArcTiffFileJAPI
 * Method:    Read
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_arc_api_tiff_ArcTiffFileJAPI_Read( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	unsigned char* pU8Buf = NULL;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid TIFF file handle" );
		}

		pU8Buf =
			reinterpret_cast<unsigned char *>(
						g_tiffMap[ INT_PTR( jFileHandle ) ].get()->Read() );

		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Failed to read image data!" );
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Read", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Read" );
	}

	return jlong( pU8Buf );
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

		oss << "( ArcTiffFileJAPI::" << sMethod << " ): " << sMsg << ends;

		pEnv->ThrowNew( newExcCls, oss.str().c_str() );
	}
}

void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod )
{
	jclass newExcCls = pEnv->FindClass( "java/lang/Exception" );
	
	if ( newExcCls != NULL )
	{
		pEnv->ThrowNew( newExcCls,
					   ( "( ArcTiffFileJAPI::" + sMethod +
					   " ): An unknown error caused an exception!" ).c_str() );
	}
}
