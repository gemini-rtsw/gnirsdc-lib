// ArcFitsFileJAPI.cpp : Defines the exported functions for the DLL application.
//

#ifdef WIN32
	#include <windows.h>
#endif

#ifdef __APPLE__
	#include <JavaVM/jni.h>
#else
	#include <jni.h>
#endif

#include "arc_api_fits_ArcFitsFileJAPI.h"
#include "CArcFitsFile.h"

#ifndef WIN32
	#include <cstdint>
#endif

#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <map>

using namespace std;
using namespace arc::fits;


// +-----------------------------------------------------------------------------------------+
// | Private Functions                                                                       |
// +-----------------------------------------------------------------------------------------+
void ThrowJNIException( JNIEnv* pEnv, string sMethod, string sMsg );
void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod );


// +-----------------------------------------------------------------------------------------+
// | Typedefs                                                                                |
// +-----------------------------------------------------------------------------------------+
typedef std::shared_ptr<CArcFitsFile> SharedFitsPtr;

#ifndef WIN32
	typedef uintptr_t INT_PTR;
#endif


// +-----------------------------------------------------------------------------------------+
// | Globals                                                                                 |
// +-----------------------------------------------------------------------------------------+
static map<long,SharedFitsPtr> g_fitsMap;


// +-----------------------------------------------------------------------------------------+
// | API Functions                                                                           |
// +-----------------------------------------------------------------------------------------+

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetAPIConstants
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetAPIConstants( JNIEnv* pEnv, jclass clazz )
{
	jfieldID fid;

	try
	{
		fid = pEnv->GetStaticFieldID( clazz, "READMODE", "I" );
		if ( fid == 0 ) { throw runtime_error( "READMODE field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcFitsFile::READMODE ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "READWRITEMODE", "I" );
		if ( fid == 0 ) { throw runtime_error( "READWRITEMODE field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcFitsFile::READWRITEMODE ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "BPP16", "I" );
		if ( fid == 0 ) { throw runtime_error( "BPP16 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcFitsFile::BPP16 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "BPP32", "I" );
		if ( fid == 0 ) { throw runtime_error( "BPP32 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CArcFitsFile::BPP32 ); }
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
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    WriteFile
 * Signature: (Ljava/lang/String;JII)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_WriteFile
( JNIEnv* pEnv, jclass clazz, jstring jFilename, jlong jBufferVA, jint jRows, jint jCols )
{
	// Get the name of the file to write
	const char* pszFilename = ( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to allocate memory for filename!" );
		}

		unsigned char* pU8Buffer = reinterpret_cast<unsigned char *>( jBufferVA );

		if ( pU8Buffer == NULL )
		{
			throw runtime_error( "NULL image buffer pointer parameter!" );
		}

		CArcFitsFile cFits( pszFilename, int( jRows ), int( jCols ) );

		cFits.Write( pU8Buffer );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "WriteFile", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "WriteFile" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}
}


/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    WriteTextFile
 * Signature: (Ljava/lang/String;JII)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_WriteTextFile
( JNIEnv* pEnv, jclass clazz, jstring jFilename, jlong jBufferVA, jint jRows, jint jCols )
{
	// Get the name of the file to write
	const char* pszFilename =
				( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to allocate memory for filename!" );
		}

		unsigned short* pU16Buffer = reinterpret_cast<unsigned short *>( jBufferVA );

		if ( pU16Buffer == NULL )
		{
			throw runtime_error( "NULL image buffer pointer parameter!" );
		}

		ofstream ofs( pszFilename );

		if ( ofs.is_open() )
		{
			for ( int dPix=0; dPix<int( jRows * jCols ); dPix++ )
			{
				ofs << dec << dPix << "\t\t0x" << hex << uppercase
					<< pU16Buffer[ dPix ] << endl;
			}

			ofs.close();
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "WriteFile", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "WriteFile" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}
}


/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GenerateTestDataFile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GenerateTestDataFile
( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	// Get the name of the file to write
	const char* pszFilename = ( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to allocate memory for filename!" );
		}

		CArcFitsFile cFits( pszFilename );

		cFits.GenerateTestData();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GenerateTestDataFile", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GenerateTestDataFile" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetHeader
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetHeader__Ljava_lang_String_2
( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	jobjectArray	jObjArray	= NULL;
	int				dKeyCount	= 0;

	// Get the name of the file to write
	const char* pszFilename = ( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to allocate memory for filename!" );
		}

		CArcFitsFile cFits( pszFilename );

		string* pHeader = cFits.GetHeader( &dKeyCount );

		if ( dKeyCount == 0 || pHeader == NULL )
		{
			throw runtime_error( "Empty header returned from file!" );
		}

		jObjArray = pEnv->NewObjectArray( dKeyCount,
										  pEnv->FindClass( "java/lang/String" ),
										  pEnv->NewStringUTF( "" ) );

		if ( jObjArray == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to create new object array!" );
		}

		for ( int i=0; i<dKeyCount; i++ )
		{
			pEnv->SetObjectArrayElement( jObjArray,
									     i,
										 pEnv->NewStringUTF( pHeader[ i ].c_str() ) );

			if ( pEnv->ExceptionOccurred() != NULL )
			{
				throw runtime_error( "Failed to add header string to list!" );
			}
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetHeader", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetHeader" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}

	return jObjArray;
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetNumberOfFrames
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetNumberOfFrames__Ljava_lang_String_2
( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	// Get the name of the file to write
	const char* pszFilename = ( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );

	long lNumOfFrames = 0;

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to allocate memory for filename!" );
		}

		CArcFitsFile cFits( pszFilename );

		lNumOfFrames = cFits.GetNumberOfFrames();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetNumberOfFrames", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetNumberOfFrames" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}

	return jlong( lNumOfFrames );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetRows
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetRows__Ljava_lang_String_2
( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	// Get the name of the file to write
	const char* pszFilename = ( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );

	long lRows = 0;

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to allocate memory for filename!" );
		}

		CArcFitsFile cFits( pszFilename );

		lRows = cFits.GetRows();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetRows", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetRows" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}

	return jlong( lRows );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetCols
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetCols__Ljava_lang_String_2
( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	// Get the name of the file to write
	const char* pszFilename = ( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );

	long lCols = 0;

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to allocate memory for filename!" );
		}

		CArcFitsFile cFits( pszFilename );

		lCols = cFits.GetCols();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCols", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCols" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}

	return jlong( lCols );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetNAxis
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetNAxis__Ljava_lang_String_2
( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	// Get the name of the file to write
	const char* pszFilename = ( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );

	int dNAxis = 0;

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to allocate memory for filename!" );
		}

		CArcFitsFile cFits( pszFilename );

		dNAxis = cFits.GetNAxis();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetNAxis", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetNAxis" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}

	return jint( dNAxis );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetBpp
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetBpp__Ljava_lang_String_2
( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	// Get the name of the file to write
	const char* pszFilename = ( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );

	int dBpp = 0;

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to allocate memory for filename!" );
		}

		CArcFitsFile cFits( pszFilename );

		dBpp = static_cast<int>( cFits.GetRows() );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetBpp", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetBpp" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}

	return jint( dBpp );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetHeader
 * Signature: (J)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetHeader__J
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	jobjectArray	jObjArray	= NULL;
	int				dKeyCount	= 0;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		string* pHeader =
				g_fitsMap[ INT_PTR( jFileHandle ) ].get()->GetHeader( &dKeyCount );

		if ( dKeyCount == 0 || pHeader == NULL )
		{
			throw runtime_error( "Empty header returned from file!" );
		}

		jObjArray = pEnv->NewObjectArray( dKeyCount,
										  pEnv->FindClass( "java/lang/String" ),
										  pEnv->NewStringUTF( "" ) );

		if ( jObjArray == NULL || pEnv->ExceptionOccurred() != NULL )
		{
			throw runtime_error( "Failed to create new object array!" );
		}

		for ( int i=0; i<dKeyCount; i++ )
		{
			pEnv->SetObjectArrayElement( jObjArray,
									     i,
										 pEnv->NewStringUTF( pHeader[ i ].c_str() ) );

			if ( pEnv->ExceptionOccurred() != NULL )
			{
				throw runtime_error( "Failed to add header string to list!" );
			}
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetHeader", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetHeader" );
	}

	return jObjArray;
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetNumberOfFrames
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetNumberOfFrames__J
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	long lNumOfFrames = 0;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		lNumOfFrames =
				g_fitsMap[ INT_PTR( jFileHandle ) ].get()->GetNumberOfFrames();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetNumberOfFrames", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetNumberOfFrames" );
	}

	return jlong( lNumOfFrames );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetRows
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetRows__J
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	long lRows = 0;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		lRows = g_fitsMap[ INT_PTR( jFileHandle ) ].get()->GetRows();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetRows", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetRows" );
	}

	return jlong( lRows );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetCols
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetCols__J
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	long lCols = 0;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		lCols = g_fitsMap[ INT_PTR( jFileHandle ) ].get()->GetCols();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCols", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCols" );
	}

	return jlong( lCols );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetNAxis
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetNAxis__J
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	int dNAxis = 0;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		dNAxis = g_fitsMap[ INT_PTR( jFileHandle ) ].get()->GetNAxis();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetNAxis", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetNAxis" );
	}

	return jint( dNAxis );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GetBpp
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GetBpp__J
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	int dBpp = 0;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		dBpp = g_fitsMap[ INT_PTR( jFileHandle ) ].get()->GetBpp();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetBpp", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetBpp" );
	}

	return jint( dBpp );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    GenerateTestData
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_GenerateTestData
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		g_fitsMap[ INT_PTR( jFileHandle ) ].get()->GenerateTestData();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GenerateTestData", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GenerateTestData" );
	}
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    ReOpen
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_ReOpen
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		g_fitsMap[ INT_PTR( jFileHandle ) ].get()->ReOpen();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "ReOpen", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "ReOpen" );
	}
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    WriteKeyword
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_WriteKeyword
( JNIEnv * pEnv, jclass clazz, jstring jkey, jstring jkeyVal, jstring jComment, jstring jFilename )
{
	char* pszKey      = NULL;
	char* pszKeyVal   = NULL;
	char* pszComment  = NULL;
	char* pszFilename = NULL;

	try
	{
		jclass stringClass = pEnv->FindClass( "Ljava/lang/String;" );
	
		if ( stringClass == NULL )
		{
			throw runtime_error( "Internal error. Failed to find \"String\" class!" );
		}

		jmethodID stringMatchesID = pEnv->GetMethodID( stringClass,
													  "matches",
													  "(Ljava/lang/String;)Z" );
		
		if ( stringMatchesID == NULL )
		{
			throw runtime_error( "Internal error. Failed to find \"String.matches()\" method!" );
		}

		jstring integerRegEx = pEnv->NewStringUTF( "[0-9]+" );
		jstring doubleRegEx  = pEnv->NewStringUTF( "[0-9]*[.]{1}[0-9]+" );
		jstring logicalRegEx = pEnv->NewStringUTF( "[TF]{1}" );
			
		if ( integerRegEx == NULL || doubleRegEx == NULL || logicalRegEx == NULL )
		{
			throw runtime_error( "Internal error. Regular expressions for parsing are NULL!" );
		}

		pszKey      = ( ( jkey      != NULL ) ? ( char * )pEnv->GetStringUTFChars( jkey, 0 )      : NULL );
		pszKeyVal   = ( ( jkeyVal   != NULL ) ? ( char * )pEnv->GetStringUTFChars( jkeyVal, 0 )   : NULL );
		pszComment  = ( ( jComment  != NULL ) ? ( char * )pEnv->GetStringUTFChars( jComment, 0 )  : NULL );
		pszFilename = ( ( jFilename != NULL ) ? ( char * )pEnv->GetStringUTFChars( jFilename, 0 ) : NULL );

		//
		// NOTE: The comment field can be NULL
		//
		if ( pszKey == NULL )
		{
			throw runtime_error( "Internal error. FITS keyword failed to be parsed!" );
		}

		else if ( pszKeyVal == NULL )
		{
			throw runtime_error( "Internal error. FITS keyword value failed to be parsed!" );
		}

		else if ( pszFilename == NULL )
		{
			throw runtime_error( "Internal error. FITS filename failed to be parsed!" );
		}
			
		CArcFitsFile cFits( pszFilename, CArcFitsFile::READWRITEMODE );
					
		if ( pEnv->CallObjectMethod( jkeyVal, stringMatchesID, doubleRegEx ) )
		{
			double doubleKeyVal = atof( pszKeyVal );
						
			cFits.WriteKeyword( pszKey,
								&doubleKeyVal,
								CArcFitsFile::FITS_DOUBLE_KEY,
								pszComment );
		}
					
		else if ( pEnv->CallObjectMethod( jkeyVal, stringMatchesID, integerRegEx ) )
		{
			int intKeyVal = atoi( pszKeyVal );
		
			cFits.WriteKeyword( pszKey,
								&intKeyVal,
								CArcFitsFile::FITS_INTEGER_KEY,
								pszComment );
		}
					
		else if ( pEnv->CallObjectMethod( jkeyVal, stringMatchesID, logicalRegEx ) )
		{
			cFits.WriteKeyword( pszKey,
								pszKeyVal,
								CArcFitsFile::FITS_LOGICAL_KEY,
								pszComment );
		}
					
		else
		{
			cFits.WriteKeyword( pszKey,
								pszKeyVal,
								CArcFitsFile::FITS_STRING_KEY,
								pszComment );
		} 
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "WriteKeyword", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "WriteKeyword" );
	}

	//
	// Free the allocated memory for strings
	//
	if ( pszKey      != NULL ) { pEnv->ReleaseStringUTFChars( jkey, pszKey );           }
	if ( pszKeyVal   != NULL ) { pEnv->ReleaseStringUTFChars( jkeyVal, pszKeyVal );     }
	if ( pszFilename != NULL ) { pEnv->ReleaseStringUTFChars( jFilename, pszFilename ); }
	if ( pszComment  != NULL ) { pEnv->ReleaseStringUTFChars( jComment, pszComment );   }
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    WriteComment
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_WriteComment
( JNIEnv* pEnv, jclass clazz, jstring jComment, jstring jFilename )
{
	// Get the comment and the name of the file to write
	const char* pszComment  = ( ( jComment  != NULL ) ? pEnv->GetStringUTFChars( jComment,  0 ) : NULL );
	const char* pszFilename = ( ( jFilename != NULL ) ? pEnv->GetStringUTFChars( jFilename, 0 ) : NULL );
	
	try
	{
		if ( pszComment == NULL )
		{
			throw runtime_error( "Failed to convert the comment parameter!" );
		}

		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to convert the filename parameter!" );
		}

		CArcFitsFile cFits( pszFilename, CArcFitsFile::READWRITEMODE );
				
		cFits.WriteKeyword( NULL,
						    ( void * )pszComment,
						    CArcFitsFile::FITS_COMMENT_KEY,
						    NULL );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "WriteComment", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "WriteComment" );
	}

	if ( pszComment  != NULL ) { pEnv->ReleaseStringUTFChars( jComment,  pszComment );  }
	if ( pszFilename != NULL ) { pEnv->ReleaseStringUTFChars( jFilename, pszFilename ); }
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    WriteHistory
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_WriteHistory
( JNIEnv* pEnv, jclass clazz, jstring jHistory, jstring jFilename )
{
	// Get the history and the name of the file to write
	const char* pszHistory  = ( ( jHistory  != NULL ) ? pEnv->GetStringUTFChars( jHistory,  0 ) : NULL );
	const char* pszFilename = ( ( jFilename != NULL ) ? pEnv->GetStringUTFChars( jFilename, 0 ) : NULL );
	
	try
	{
		if ( pszHistory == NULL )
		{
			throw runtime_error( "Failed to convert the history parameter!" );
		}

		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to convert the filename parameter!" );
		}

		CArcFitsFile cFits( pszFilename, CArcFitsFile::READWRITEMODE );
				
		cFits.WriteKeyword( NULL,
						    ( void * )pszHistory,
						    CArcFitsFile::FITS_HISTORY_KEY,
						    NULL );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "WriteHistory", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "WriteHistory" );
	}

	if ( pszHistory  != NULL ) { pEnv->ReleaseStringUTFChars( jHistory,  pszHistory );  }
	if ( pszFilename != NULL ) { pEnv->ReleaseStringUTFChars( jFilename, pszFilename ); }
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    WriteDate
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_WriteDate
( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	// Get the name of the file to write
	const char* pszFilename = ( ( jFilename != NULL ) ? pEnv->GetStringUTFChars( jFilename, 0 ) : NULL );
	
	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to convert the filename parameter!" );
		}

		CArcFitsFile cFits( pszFilename, CArcFitsFile::READWRITEMODE );
				
		cFits.WriteKeyword( NULL,
							NULL,
							CArcFitsFile::FITS_DATE_KEY,
							NULL );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "WriteDate", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "WriteDate" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    Create
 * Signature: (Ljava/lang/String;III)J
 */
JNIEXPORT jlong JNICALL Java_arc_api_fits_ArcFitsFileJAPI_Create
( JNIEnv* pEnv, jclass clazz, jstring jFilename, jint jRows, jint jCols, jint jBitsPerPixel )
{
	INT_PTR dRetVal = 0;

	// Get the FITS filename
	const char* pszFilename = ( jFilename != NULL ? pEnv->GetStringUTFChars( jFilename, 0 ) : NULL );

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to convert filename parameter!" );
		}

		std::shared_ptr<CArcFitsFile> pCFits(
											new CArcFitsFile( pszFilename,
														   int( jRows ),
														   int( jCols ),
														   int( jBitsPerPixel ) ) );

		g_fitsMap.insert(
					pair<long,SharedFitsPtr>(
							reinterpret_cast<long>( pCFits.get() ), pCFits ) ); 

		dRetVal = INT_PTR( pCFits.get() );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Create", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Create" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}

	return jlong( dRetVal );
}


/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    Open
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL
Java_arc_api_fits_ArcFitsFileJAPI_Open( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	INT_PTR dRetVal = 0;

	// Get the FITS filename
	const char* pszFilename =
				( jFilename != NULL ? pEnv->GetStringUTFChars( jFilename, 0 ) : NULL );

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to convert filename parameter!" );
		}

		std::shared_ptr<CArcFitsFile> pCFits( new CArcFitsFile( pszFilename ) );

		g_fitsMap.insert(
					pair<long,SharedFitsPtr>(
							reinterpret_cast<long>( pCFits.get() ), pCFits ) ); 

		dRetVal = INT_PTR( pCFits.get() );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Open", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Open" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}

	return jlong( dRetVal );
}


/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    Resize
 * Signature: (JII)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_Resize
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle, jint jRows, jint jCols )
{
	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		g_fitsMap[ INT_PTR( jFileHandle ) ].get()->Resize( int( jRows ),
														   int( jCols ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Resize", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Resize" );
	}
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    Write
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_Write__JJ
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle, jlong jBufferVA )
{
	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		g_fitsMap[ INT_PTR( jFileHandle ) ].get()->Write( pU8Buf );
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
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    Write
 * Signature: (JJIII)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_Write__JJIII
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle, jlong jBufferVA, jint jBytes, jint bytesToSkip, jint jFPixel )
{
	try
	{
		unsigned char* pU8Buf =
				reinterpret_cast<unsigned char *>( jBufferVA ) + bytesToSkip;
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		g_fitsMap[ INT_PTR( jFileHandle ) ].get()->Write( pU8Buf,
														  int( jBytes ),
														  int( jFPixel ) );
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
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    Read
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_arc_api_fits_ArcFitsFileJAPI_Read
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	unsigned char* pU8Buf = NULL;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		pU8Buf = reinterpret_cast<unsigned char *>(
						g_fitsMap[ INT_PTR( jFileHandle ) ].get()->Read() );

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


/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    WriteSubImage
 * Signature: (JIIIIJ)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_WriteSubImage
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle, jint llrow, jint llcol, jint urrow, jint urcol, jlong jBufferVA )
{
	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		g_fitsMap[ INT_PTR( jFileHandle ) ].get()->WriteSubImage( pU8Buf,
																  int( llrow ),
																  int( llcol ),
																  int( urrow ),
																  int( urcol ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "WriteSubImage", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "WriteSubImage" );
	}
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    ReadSubImage
 * Signature: (JIIII)J
 */
JNIEXPORT jlong JNICALL Java_arc_api_fits_ArcFitsFileJAPI_ReadSubImage
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle, jint llrow, jint llcol, jint urrow, jint urcol )
{
	unsigned char* pU8Buf = NULL;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		pU8Buf = reinterpret_cast<unsigned char *>(
				  g_fitsMap[ INT_PTR( jFileHandle ) ].get()->ReadSubImage( int( llrow ),
																		   int( llcol ),
																		   int( urrow ),
																		   int( urcol ) ) );

		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Failed to read image data!" );
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "ReadSubImage", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "ReadSubImage" );
	}

	return jlong( pU8Buf );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    Create3D
 * Signature: (Ljava/lang/String;III)J
 */
JNIEXPORT jlong JNICALL Java_arc_api_fits_ArcFitsFileJAPI_Create3D
( JNIEnv* pEnv, jclass clazz, jstring jFilename, jint jRows, jint jCols, jint jBitsPerPixel )
{
	INT_PTR dRetVal = 0;

	// Get the FITS filename
	const char* pszFilename = ( jFilename != NULL ? pEnv->GetStringUTFChars( jFilename, 0 ) : NULL );

	try
	{
		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to convert filename parameter!" );
		}

		std::shared_ptr<CArcFitsFile> pCFits(
										new CArcFitsFile( pszFilename,
														   int( jRows ),
														   int( jCols ),
														   int( jBitsPerPixel ),
														   true ) );

		g_fitsMap.insert(
					pair<long,SharedFitsPtr>(
							reinterpret_cast<long>( pCFits.get() ), pCFits ) ); 

		dRetVal = INT_PTR( pCFits.get() );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Create3D", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Create3D" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}

	return jlong( dRetVal );
}


/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    Write3D
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_Write3D__JJ
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle, jlong jBufferVA )
{
	try
	{
		unsigned char* pU8Buf = reinterpret_cast<unsigned char *>( jBufferVA );
			
		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		g_fitsMap[ INT_PTR( jFileHandle ) ].get()->Write3D( pU8Buf );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Write3D", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Write3D" );
	}
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    Write3D
 * Signature: (JJI)V
 */
JNIEXPORT void JNICALL Java_arc_api_fits_ArcFitsFileJAPI_Write3D__JJI
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle, jlong jBufferVA, jint jByteOffset )
{
	try
	{
		unsigned char* pU8Buf =
						reinterpret_cast<unsigned char *>( jBufferVA ) +
						int( jByteOffset );

		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Image buffer is NULL!" );
		}

		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		g_fitsMap[ INT_PTR( jFileHandle ) ].get()->Write3D( pU8Buf );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Write3D", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Write3D" );
	}
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    Read3D
 * Signature: (JI)J
 */
JNIEXPORT jlong JNICALL Java_arc_api_fits_ArcFitsFileJAPI_Read3D
( JNIEnv* pEnv, jclass clazz, jlong jFileHandle, jint jImageNumber )
{
	unsigned char* pU8Buf = NULL;

	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		pU8Buf = reinterpret_cast<unsigned char *>(
				 g_fitsMap[ INT_PTR( jFileHandle ) ].get()->Read3D( int( jImageNumber ) ) );

		if ( pU8Buf == NULL )
		{
			throw runtime_error( "Failed to read image data!" );
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Read3D", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Read3D" );
	}

	return jlong( pU8Buf );
}

/*
 * Class:     arc_api_fits_ArcFitsFileJAPI
 * Method:    Close
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_arc_api_fits_ArcFitsFileJAPI_Close( JNIEnv* pEnv, jclass clazz, jlong jFileHandle )
{
	try
	{
		if ( jFileHandle == 0 )
		{
			throw runtime_error( "Invalid FITS file handle" );
		}

		map<long,SharedFitsPtr>::iterator it;

		it = g_fitsMap.find( INT_PTR( jFileHandle ) );

		if ( it != g_fitsMap.end() )
		{
			g_fitsMap[ INT_PTR( jFileHandle ) ].reset();

			g_fitsMap.erase( it );
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "CloseFile", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "CloseFile" );
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

		oss << "( ArcFitsAPI::" << sMethod << " ): " << sMsg << ends;

		pEnv->ThrowNew( newExcCls, oss.str().c_str() );
	}
}

void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod )
{
	jclass newExcCls = pEnv->FindClass( "java/lang/Exception" );
	
	if ( newExcCls != NULL )
	{
		pEnv->ThrowNew( newExcCls,
					   ( "( ArcFitsAPI::" + sMethod +
					   " ): An unknown error caused an exception!" ).c_str() );
	}
}
