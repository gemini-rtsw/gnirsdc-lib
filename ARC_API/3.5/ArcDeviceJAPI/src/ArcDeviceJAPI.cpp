// ArcDeviceJAPI.cpp : Defines the exported functions for the DLL application.
//

#ifdef __APPLE__
	#include <JavaVM/jni.h>
#else
	#include <jni.h>
#endif

#include "arc_api_device_ArcDeviceJAPI.h"
#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcTools.h"
#include "ArcOSDefs.h"
#include "ArcDefs.h"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cstring>
#include <memory>
#include <cmath>
#include <list>
#include <ios>

//#include <fstream>
//std::ofstream dbgStream( "CameraAPI_Debug.txt" );

using namespace std;
using namespace arc;
using namespace arc::device;


// +-----------------------------------------------------------------------------------------+
// | Private Macros                                                                          |
// +-----------------------------------------------------------------------------------------+
#ifdef WIN32
	#define ASSERT_DEVICE_PTR()								\
						if ( !g_pCArcDev.get() ) {			\
						throw runtime_error(				\
						"No device is open!" ); }
#else
	#define ASSERT_DEVICE_PTR()								\
						if ( !g_pCArcDev.get() ) {			\
						throw runtime_error(				\
						"No device is open!" ); }
#endif


// +-----------------------------------------------------------------------------------------+
// | Private Functions                                                                       |
// +-----------------------------------------------------------------------------------------+
void ThrowJNIException( JNIEnv* pEnv, string sMethod, string sMsg );
void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod );
bool NotZero( jfloat f ) { return ( ( f != 0 ) ); }


// +-----------------------------------------------------------------------------------------+
// | Globals                                                                                 |
// +-----------------------------------------------------------------------------------------+
static auto_ptr<CArcDevice> g_pCArcDev( 0 );
static bool g_bAbort = false;


// +-----------------------------------------------------------------------------------------+
// | API Functions                                                                           |
// +-----------------------------------------------------------------------------------------+

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetAPIConstants
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_arc_api_device_ArcDeviceJAPI_GetAPIConstants
( JNIEnv* pEnv, jclass clazz )
{
	jfieldID fid;

	try
	{
		fid = pEnv->GetStaticFieldID( clazz, "PCI_ID", "I" );
		if ( fid == 0 ) { throw runtime_error( "PCI_ID field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, PCI_ID ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "TIM_ID", "I" );
		if ( fid == 0 ) { throw runtime_error( "TIM_ID field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, TIM_ID ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "UTIL_ID", "I" );
		if ( fid == 0 ) { throw runtime_error( "UTIL_ID field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, UTIL_ID ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "X_MEM", "I" );
		if ( fid == 0 ) { throw runtime_error( "X_MEM field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, X_MEM ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "Y_MEM", "I" );
		if ( fid == 0 ) { throw runtime_error( "Y_MEM field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, Y_MEM ); }

		fid = pEnv->GetStaticFieldID( clazz, "P_MEM", "I" );
		if ( fid == 0 ) { throw runtime_error( "P_MEM field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, P_MEM ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "R_MEM", "I" );
		if ( fid == 0 ) { throw runtime_error( "R_MEM field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, R_MEM ); }

		fid = pEnv->GetStaticFieldID( clazz, "DON", "I" );
		if ( fid == 0 ) { throw runtime_error( "DON field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, DON ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ERR", "I" );
		if ( fid == 0 ) { throw runtime_error( "ERR field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ERR ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SYR", "I" );
		if ( fid == 0 ) { throw runtime_error( "SYR field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SYR ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "TOUT", "I" );
		if ( fid == 0 ) { throw runtime_error( "TOUT field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, TOUT ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "READOUT", "I" );
		if ( fid == 0 ) { throw runtime_error( "READOUT field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ROUT ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "TDL", "I" );
		if ( fid == 0 ) { throw runtime_error( "TDL field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, TDL ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "RDM", "I" );
		if ( fid == 0 ) { throw runtime_error( "RDM field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, RDM ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "WRM", "I" );
		if ( fid == 0 ) { throw runtime_error( "WRM field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, WRM ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "POF", "I" );
		if ( fid == 0 ) { throw runtime_error( "POF field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, POF ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "PON", "I" );
		if ( fid == 0 ) { throw runtime_error( "PON field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, PON ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SET", "I" );
		if ( fid == 0 ) { throw runtime_error( "SET field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SET ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "RET", "I" );
		if ( fid == 0 ) { throw runtime_error( "RET field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, RET ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SEX", "I" );
		if ( fid == 0 ) { throw runtime_error( "SEX field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SEX ); }

		fid = pEnv->GetStaticFieldID( clazz, "PEX", "I" );
		if ( fid == 0 ) { throw runtime_error( "PEX field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, PEX ); }

		fid = pEnv->GetStaticFieldID( clazz, "REX", "I" );
		if ( fid == 0 ) { throw runtime_error( "REX field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, REX ); }

		fid = pEnv->GetStaticFieldID( clazz, "CLR", "I" );
		if ( fid == 0 ) { throw runtime_error( "CLR field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CLR ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "OSH", "I" );
		if ( fid == 0 ) { throw runtime_error( "OSH field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, OSH ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "CSH", "I" );
		if ( fid == 0 ) { throw runtime_error( "CSH field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CSH ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "IDL", "I" );
		if ( fid == 0 ) { throw runtime_error( "IDL field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, IDL ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "STP", "I" );
		if ( fid == 0 ) { throw runtime_error( "STP field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, STP ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SMX", "I" );
		if ( fid == 0 ) { throw runtime_error( "SMX field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SMX ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "CLK", "I" );
		if ( fid == 0 ) { throw runtime_error( "CLK field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CLK ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "VID", "I" );
		if ( fid == 0 ) { throw runtime_error( "VID field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, VID ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SBN", "I" );
		if ( fid == 0 ) { throw runtime_error( "SBN field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SBN ); }

		fid = pEnv->GetStaticFieldID( clazz, "SGN", "I" );
		if ( fid == 0 ) { throw runtime_error( "SGN field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SGN ); }

		fid = pEnv->GetStaticFieldID( clazz, "SOS", "I" );
		if ( fid == 0 ) { throw runtime_error( "SOS field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SOS ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SSS", "I" );
		if ( fid == 0 ) { throw runtime_error( "SSS field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SSS ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SSP", "I" );
		if ( fid == 0 ) { throw runtime_error( "SSP field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SSP ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "RNC", "I" );
		if ( fid == 0 ) { throw runtime_error( "RNC field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, RNC ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SID", "I" );
		if ( fid == 0 ) { throw runtime_error( "SID field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SID ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "AMP_0", "I" );
		if ( fid == 0 ) { throw runtime_error( "AMP_0 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, AMP_0 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "AMP_1", "I" );
		if ( fid == 0 ) { throw runtime_error( "AMP_1 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, AMP_1 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "AMP_2", "I" );
		if ( fid == 0 ) { throw runtime_error( "AMP_2 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, AMP_2 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "AMP_3", "I" );
		if ( fid == 0 ) { throw runtime_error( "AMP_3 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, AMP_3 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "AMP_L", "I" );
		if ( fid == 0 ) { throw runtime_error( "AMP_L field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, AMP_L ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "AMP_R", "I" );
		if ( fid == 0 ) { throw runtime_error( "AMP_R field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, AMP_R ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "AMP_LR", "I" );
		if ( fid == 0 ) { throw runtime_error( "AMP_LR field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, AMP_LR ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "AMP_ALL", "I" );
		if ( fid == 0 ) { throw runtime_error( "AMP_ALL field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, AMP_ALL ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SPLIT_SERIAL", "I" );
		if ( fid == 0 ) { throw runtime_error( "SPLIT_SERIAL field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SPLIT_SERIAL ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SPLIT_PARALLEL", "I" );
		if ( fid == 0 ) { throw runtime_error( "SPLIT_PARALLEL field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SPLIT_PARALLEL ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "VIDGENI", "I" );
		if ( fid == 0 ) { throw runtime_error( "VIDGENI field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, VIDGENI ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "TIMGENI", "I" );
		if ( fid == 0 ) { throw runtime_error( "TIMGENI field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, TIMGENI ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SHUTTER_CC", "I" );
		if ( fid == 0 ) { throw runtime_error( "SHUTTER_CC field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SHUTTER_CC ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "TEMP_SIDIODE", "I" );
		if ( fid == 0 ) { throw runtime_error( "TEMP_SIDIODE field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, TEMP_SIDIODE ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "TEMP_LINEAR", "I" );
		if ( fid == 0 ) { throw runtime_error( "TEMP_LINEAR field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, TEMP_LINEAR ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "SUBARRAY", "I" );
		if ( fid == 0 ) { throw runtime_error( "SUBARRAY field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, SUBARRAY ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "BINNING", "I" );
		if ( fid == 0 ) { throw runtime_error( "BINNING field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, BINNING ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "MPP_CC", "I" );
		if ( fid == 0 ) { throw runtime_error( "MPP_CC field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, MPP_CC ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "CLKDRVGENI", "I" );
		if ( fid == 0 ) { throw runtime_error( "CLKDRVGENI field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CLKDRVGENI ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "MLO", "I" );
		if ( fid == 0 ) { throw runtime_error( "MLO field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, MLO ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "NGST", "I" );
		if ( fid == 0 ) { throw runtime_error( "NGST field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, NGST ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ARC41", "I" );
		if ( fid == 0 ) { throw runtime_error( "ARC41 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ARC41 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ARC42", "I" );
		if ( fid == 0 ) { throw runtime_error( "ARC42 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ARC42 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ARC44", "I" );
		if ( fid == 0 ) { throw runtime_error( "ARC44 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ARC44 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ARC45", "I" );
		if ( fid == 0 ) { throw runtime_error( "ARC45 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ARC45 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ARC46", "I" );
		if ( fid == 0 ) { throw runtime_error( "ARC46 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ARC46 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ARC47", "I" );
		if ( fid == 0 ) { throw runtime_error( "ARC47 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ARC47 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ARC48", "I" );
		if ( fid == 0 ) { throw runtime_error( "ARC48 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ARC48 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ARC20", "I" );
		if ( fid == 0 ) { throw runtime_error( "ARC20 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ARC20 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ARC22", "I" );
		if ( fid == 0 ) { throw runtime_error( "ARC22 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ARC22 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ARC50", "I" );
		if ( fid == 0 ) { throw runtime_error( "ARC50 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ARC50 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "ARC32", "I" );
		if ( fid == 0 ) { throw runtime_error( "ARC32 field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, ARC32 ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "CONT_RD", "I" );
		if ( fid == 0 ) { throw runtime_error( "CONT_RD field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CONT_RD ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "FO_2X_TRANSMITR", "I" );
		if ( fid == 0 ) { throw runtime_error( "FO_2X_TRANSMITR field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, FO_2X_TRANSMITR ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "CR_WRITE", "I" );
		if ( fid == 0 ) { throw runtime_error( "CR_WRITE field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CR_WRITE ); }
		
		fid = pEnv->GetStaticFieldID( clazz, "CR_COADD", "I" );
		if ( fid == 0 ) { throw runtime_error( "CR_COADD field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CR_COADD ); }

		fid = pEnv->GetStaticFieldID( clazz, "CR_DEBUG", "I" );
		if ( fid == 0 ) { throw runtime_error( "CR_DEBUG field does not exist!" ); }
		else { pEnv->SetStaticIntField( clazz, fid, CR_DEBUG ); }
	}
	catch ( exception& e )
	{
		ThrowJNIException( pEnv, "GetAPIConstants", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetAPIConstants" );
	}
}


/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetDeviceList
 * Signature: ([Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetDeviceList( JNIEnv* pEnv, jclass clazz, jobjectArray jDevArr )
{
	jobjectArray	jDriverList		= 0;
	int				dDeviceCount	= 0;

	string*			psPCIDevList	= NULL;
	string*			psPCIeDevList	= NULL;

	CArcPCIe		cArcPCIe;
	CArcPCI			cArcPCI;

	try
	{
		//
		//  Get the PCI device list
		// -------------------------------------------------
		CArcPCI::FindDevices();
			
		//
		// Get the PCI device string list. Ignore NULL list,
		// will handle down below.
		//
		psPCIDevList = const_cast< string * >( CArcPCI::GetDeviceStringList() );
	}
	catch ( ... ) {}

	try
	{
		//
		//  Get the PCIe device list
		// -------------------------------------------------
		CArcPCIe::FindDevices();
			
		//
		// Get the PCIe device string list. Ignore NULL list,
		// will handle down below.
		//
		psPCIeDevList = const_cast< string * >( CArcPCIe::GetDeviceStringList() );
	}
	catch ( ... ) {}

	try
	{
		if ( ( psPCIDevList == NULL ) && ( psPCIeDevList == NULL ) )
		{
			throw runtime_error( "Failed to retrieve device string list!" );
		}

		//
		// Get the available device count
		//
		dDeviceCount = CArcPCI::DeviceCount() + CArcPCIe::DeviceCount();
			
		if ( dDeviceCount > 0 )
		{
			jDriverList =
					( jobjectArray )pEnv->NewObjectArray( dDeviceCount,
														 pEnv->FindClass( "java/lang/String" ),
														 pEnv->NewStringUTF( "" ) );
				
			if ( jDriverList != NULL )
			{
				ostringstream oss;
				
				//
				// Add the PCIe devices
				//
				for ( int i=0; i<CArcPCIe::DeviceCount(); i++ )
				{
					pEnv->SetObjectArrayElement(
										jDriverList,
										i,
										pEnv->NewStringUTF( psPCIeDevList[ i ].c_str() ) );
						
					if ( pEnv->ExceptionOccurred() != NULL )
					{
						throw runtime_error( "Failed to add PCIe device name to list!" );
					}
				}

				//
				// Add the PCI devices
				//
				for ( int i=0, j=CArcPCIe::DeviceCount(); i<CArcPCI::DeviceCount(); i++, j++ )
				{
					pEnv->SetObjectArrayElement(
										jDriverList,
										j,
										pEnv->NewStringUTF( psPCIDevList[ i ].c_str() ) );
						
					if ( pEnv->ExceptionOccurred() != NULL )
					{
						throw runtime_error( "Failed to add PCI device name to list!" );
					}
				}
			}
			else
			{
				throw runtime_error( "Failed create array for device list!" );
			}
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetDeviceList", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetDeviceList" );
	}

	return jDriverList;
}


/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    IsOpen
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_arc_api_device_ArcDeviceJAPI_IsOpen
( JNIEnv* pEnv, jclass clazz )
{
	bool bDeviceOpen = false;
	
	try
	{
		// NOTE: This method should not cause an exception if
		//       the device is not set.
		if ( g_pCArcDev.get() != NULL )
		{
			bDeviceOpen = g_pCArcDev.get()->IsOpen();
		}
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "IsOpen", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "IsOpen" );
	}

	return bDeviceOpen;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    IsCCD
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_arc_api_device_ArcDeviceJAPI_IsCCD( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		ASSERT_DEVICE_PTR()
	
		return g_pCArcDev.get()->IsCCD();
	}
	catch ( ... ) {}

	return JNI_FALSE;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetId
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_arc_api_device_ArcDeviceJAPI_GetId( JNIEnv* pEnv, jclass clazz )
{
	jint jId = 0;

	try
	{
		ASSERT_DEVICE_PTR()

		jId = jint( g_pCArcDev.get()->GetId() );
	}
	catch ( exception &e )
	{
		ThrowJNIException( pEnv, "GetId", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetId" );
	}

	return jId;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    ToString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_arc_api_device_ArcDeviceJAPI_ToString( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		ASSERT_DEVICE_PTR()

		return pEnv->NewStringUTF( g_pCArcDev.get()->ToString().c_str() );
	}
	catch ( exception& e )
	{
		return pEnv->NewStringUTF( e.what() );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCommonBufferVA
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCommonBufferVA( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		ASSERT_DEVICE_PTR()

		return jlong( g_pCArcDev.get()->CommonBufferVA() );
	}
	catch ( ... ) {}

	return 0;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCommonBufferPA
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCommonBufferPA( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		ASSERT_DEVICE_PTR()

		return jlong( g_pCArcDev.get()->CommonBufferPA() );
	}
	catch ( ... ) {}

	return 0;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCommonBufferSize
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCommonBufferSize( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		ASSERT_DEVICE_PTR()

		return g_pCArcDev.get()->CommonBufferSize();
	}
	catch ( ... ) {}

	return 0;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    FillCommonBuffer
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_FillCommonBuffer( JNIEnv* pEnv, jclass clazz, jint jFillValue )
{
	try
	{
		ASSERT_DEVICE_PTR()

		if ( jFillValue < 0 || jFillValue >= pow( 2.0, 16.0 ) )
		{
			ostringstream oss;

			oss << "Fill value out of range: 0 - "
				<< static_cast<long>( pow( 2.0, 16.0 ) )
				<< ends;

			CArcTools::ThrowException( "CArcDevice",
									   "FillCommonBuffer",
									    oss.str() );
		}

		g_pCArcDev.get()->FillCommonBuffer( ( unsigned short )jFillValue );
	}
	catch ( exception &e )
	{
		ThrowJNIException( pEnv, "FillImageBuffer", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "FillImageBuffer" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    SetAbort
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_SetAbort( JNIEnv* pEnv, jclass clazz, jboolean jbOnOff )
{
	g_bAbort = ( jbOnOff != 0 );
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    LoadTmpCtrlFile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_LoadTmpCtrlFile( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	// Get the data filename
	const char* pszFilename =
					( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );

	try
	{
		ASSERT_DEVICE_PTR()

		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to convert filename parameter!" );
		}

		g_pCArcDev.get()->LoadTemperatureCtrlData( pszFilename );
	}
	catch ( exception &e )
	{
		ThrowJNIException( pEnv, "LoadTmpCtrlFile", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "LoadTmpCtrlFile" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    SaveTmpCtrlFile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_SaveTmpCtrlFile( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	// Get the data filename
	const char* pszFilename = ( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );
	
	try
	{
		ASSERT_DEVICE_PTR()

		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to convert filename parameter!" );
		}

		g_pCArcDev.get()->SaveTemperatureCtrlData( pszFilename );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SaveTmpCtrlFile", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SaveTmpCtrlFile" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    LogAPICmdsAPI
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_LogAPICmdsAPI( JNIEnv* pEnv, jclass clazz, jboolean jLogCmds )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->SetLogCmds( ( jLogCmds == 1 ? true : false ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "LogAPICmdsAPI", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "LogAPICmdsAPI" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetLoggedAPICmds
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetLoggedAPICmds( JNIEnv* pEnv, jclass clazz )
{
	jobjectArray jObjArray	= NULL;
	string       sBuffer	= "";
	int          dLogCount	= 0;

	try
	{
		ASSERT_DEVICE_PTR()

		dLogCount = g_pCArcDev.get()->GetLoggedCmdCount();

		if ( dLogCount > 0 )
		{
			jObjArray = pEnv->NewObjectArray( g_pCArcDev.get()->GetLoggedCmdCount(),
											 pEnv->FindClass( "java/lang/String" ),
											 pEnv->NewStringUTF( "" ) );

			if ( jObjArray == NULL )
			{
				throw runtime_error( "Failed create list for logged API commands!" );
			}

			sBuffer = g_pCArcDev.get()->GetNextLoggedCmd();

			int dElem = 0;

			while ( !sBuffer.empty() )
			{
				pEnv->SetObjectArrayElement( jObjArray,
											dElem,
											pEnv->NewStringUTF( sBuffer.c_str() ) );

				if ( pEnv->ExceptionOccurred() != NULL )
				{
					throw runtime_error( "Failed to add logged API command to list!" );
				}
					
				sBuffer = g_pCArcDev.get()->GetNextLoggedCmd();

				dElem++;
			}
		}
	}
	catch ( exception &e )
	{
		ThrowJNIException( pEnv, "GetLoggedAPICmds", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetLoggedAPICmds" );
	}

	return jObjArray;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    OpenNative
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_OpenNative__Ljava_lang_String_2( JNIEnv* pEnv, jclass clazz, jstring jDeviceName )
{
	//
	// Get the name of the device to open
	//
	const char* pszDeviceName = ( jDeviceName == NULL ? NULL : pEnv->GetStringUTFChars( jDeviceName, 0 ) );

	try
	{
		if ( pszDeviceName == NULL )
		{
			throw runtime_error( "Failed to convert device name parameter!" );
		}

		CArcTools::CTokenizer cTokenizer;
		cTokenizer.Victim( pszDeviceName );

		string sDevice = cTokenizer.Next();

		if ( sDevice.compare( "PCI" ) == 0 )
		{
			CArcPCI::FindDevices();

			g_pCArcDev.reset( new CArcPCI() );
		}

		else if ( sDevice.compare( "PCIe" ) == 0 )
		{
			CArcPCIe::FindDevices();
					
			g_pCArcDev.reset( new CArcPCIe() );
		}

		else
		{
			throw std::runtime_error( "Invalid device: " + sDevice );
		}

		cTokenizer.Next();		// Dump "Device" Text
					
		int dDeviceNumber = atoi( cTokenizer.Next().c_str() );

		g_pCArcDev.get()->Open( dDeviceNumber );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "OpenNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "OpenNative" );
	}

	if ( pszDeviceName != NULL )
	{
		pEnv->ReleaseStringUTFChars( jDeviceName, pszDeviceName );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    OpenNative
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_arc_api_device_ArcDeviceJAPI_OpenNative__Ljava_lang_String_2I
( JNIEnv* pEnv, jclass clazz, jstring jDeviceName, jint jBytes )
{
	try
	{
		Java_arc_api_device_ArcDeviceJAPI_OpenNative__Ljava_lang_String_2( pEnv, clazz, jDeviceName );

		g_pCArcDev.get()->MapCommonBuffer( int( jBytes ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "OpenNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "OpenNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    OpenNative
 * Signature: (Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_arc_api_device_ArcDeviceJAPI_OpenNative__Ljava_lang_String_2II
( JNIEnv* pEnv, jclass clazz, jstring jDeviceName, jint jRows, jint jCols )
{
	try
	{
		Java_arc_api_device_ArcDeviceJAPI_OpenNative__Ljava_lang_String_2( pEnv, clazz, jDeviceName );

		g_pCArcDev.get()->MapCommonBuffer( int( jRows * jCols ) * sizeof( unsigned short ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "OpenNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "OpenNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    CloseNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_CloseNative( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->Close();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "CloseNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "CloseNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    MapCommonBufferNative
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_MapCommonBufferNative( JNIEnv* pEnv, jclass clazz, jint bytes )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->MapCommonBuffer( bytes );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "MapCommonBufferNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "MapCommonBufferNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    ReMapCommonBufferNative
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_ReMapCommonBufferNative( JNIEnv* pEnv, jclass clazz, jint bytes )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->ReMapCommonBuffer( bytes );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "ReMapCommonBufferNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "ReMapCommonBufferNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    UnMapCommonBufferNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_UnMapCommonBufferNative( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->UnMapCommonBuffer();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "UnMapCommonBufferNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "UnMapCommonBufferNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    ClearStatusNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_ClearStatusNative( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->ClearStatus();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "ClearStatusNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "ClearStatusNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetStatusNative
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetStatusNative( JNIEnv* pEnv, jclass clazz )
{
	int dRetVal = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		dRetVal = g_pCArcDev.get()->GetStatus();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetStatusNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetStatusNative" );
	}
	
	return dRetVal;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetPixelCountNative
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetPixelCountNative( JNIEnv* pEnv, jclass clazz )
{
	int dRetVal = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		dRetVal = g_pCArcDev.get()->GetPixelCount();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetPixelCountNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetPixelCountNative" );
	}
	
	return dRetVal;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCRPixelCountNative
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCRPixelCountNative( JNIEnv* pEnv, jclass clazz )
{
	int dRetVal = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		dRetVal = g_pCArcDev.get()->GetCRPixelCount();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCRPixelCountNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCRPixelCountNative" );
	}
	
	return dRetVal;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetFrameCountNative
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetFrameCountNative( JNIEnv* pEnv, jclass clazz )
{
	int dRetVal = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		dRetVal = g_pCArcDev.get()->GetFrameCount();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetFrameCountNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetFrameCountNative" );
	}
	
	return dRetVal;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    IsReadoutNative
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_arc_api_device_ArcDeviceJAPI_IsReadoutNative( JNIEnv* pEnv, jclass clazz )
{
	bool bIsReadout = false;
	
	try
	{
		ASSERT_DEVICE_PTR()

		bIsReadout = g_pCArcDev.get()->IsReadout();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "IsReadoutNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "IsReadoutNative" );
	}
	
	return bIsReadout;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    CommandNative
 * Signature: (IIIIII)I
 */
JNIEXPORT jint JNICALL Java_arc_api_device_ArcDeviceJAPI_CommandNative
(JNIEnv* pEnv, jclass clazz, jint jBoardId, jint jCmd, jint jArg1, jint jArg2, jint jArg3, jint jArg4 )
{
	int dRetVal = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		dRetVal = g_pCArcDev.get()->Command( int( jBoardId ),
										     int( jCmd ),
										     int( jArg1 ),
										     int( jArg2 ),
										     int( jArg3 ),
										     int( jArg4 ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "CommandNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "CommandNative" );
	}
	
	return dRetVal;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    LoadDeviceFileNative
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_LoadDeviceFileNative( JNIEnv* pEnv, jclass clazz, jstring jFilename )
{
	//
	// Get the filename
	//
	const char* pszFilename = ( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );
	
	try
	{
		ASSERT_DEVICE_PTR()

		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to create filename parameter!" );
		}

		g_pCArcDev.get()->LoadDeviceFile( ( const char * )pszFilename );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "LoadDeviceFileNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "LoadDeviceFileNative" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    LoadControllerFileNative
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_arc_api_device_ArcDeviceJAPI_LoadControllerFileNative
( JNIEnv* pEnv, jclass clazz, jstring jFilename, jint jValidate )
{
	//
	// Get the filename
	//
	const char* pszFilename = ( jFilename == NULL ? NULL : pEnv->GetStringUTFChars( jFilename, 0 ) );

	try
	{
		ASSERT_DEVICE_PTR()

		if ( pszFilename == NULL )
		{
			throw runtime_error( "Failed to create filename parameter!" );
		}

		g_pCArcDev.get()->LoadControllerFile( ( const char * )pszFilename,
										    ( jValidate == 1 ? true : false ),
										     g_bAbort );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "LoadControllerFileNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "LoadControllerFileNative" );
	}

	if ( pszFilename != NULL )
	{
		pEnv->ReleaseStringUTFChars( jFilename, pszFilename );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    SetImageSizeNative
 * Signature: (II)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_SetImageSizeNative(JNIEnv* pEnv, jclass clazz, jint rows, jint cols )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->SetImageSize( rows, cols );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SetImageSizeNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SetImageSizeNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetImageRowsNative
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetImageRowsNative( JNIEnv* pEnv, jclass clazz )
{
	int dRows = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		dRows = g_pCArcDev.get()->GetImageRows();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetImageRowsNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetImageRowsNative" );
	}
	
	return dRows;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetImageColsNative
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetImageColsNative( JNIEnv* pEnv, jclass clazz)
{
	int dCols = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		dCols = g_pCArcDev.get()->GetImageCols();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetImageColsNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetImageColsNative" );
	}
	
	return dCols;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCCParamsNative
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCCParamsNative(JNIEnv* pEnv, jclass clazz )
{
	int dCcpVal = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		dCcpVal = g_pCArcDev.get()->GetCCParams();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCCParamsNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCCParamsNative" );
	}
	
	return dCcpVal;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    IsCCParamSupportedNative
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL
Java_arc_api_device_ArcDeviceJAPI_IsCCParamSupportedNative( JNIEnv* pEnv, jclass clazz, jint parameter )
{
	bool bIsSupported = false;
	
	try
	{
		ASSERT_DEVICE_PTR()

		bIsSupported = g_pCArcDev.get()->IsCCParamSupported( parameter );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "IsCCParamSupportedNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "IsCCParamSupportedNative" );
	}
	
	return bIsSupported;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    IsControllerConnectedNative
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_arc_api_device_ArcDeviceJAPI_IsControllerConnectedNative( JNIEnv* pEnv, jclass clazz )
{
	bool bResult = false;

	try
	{
		ASSERT_DEVICE_PTR()

		bResult = g_pCArcDev.get()->IsControllerConnected();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "IsControllerConnectedNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "IsControllerConnectedNative" );
	}

	return jboolean( bResult );
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetArrayTemperatureNative
 * Signature: ()D
 */
JNIEXPORT jdouble JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetArrayTemperatureNative( JNIEnv* pEnv, jclass clazz )
{
	double fAvgTemp = 0.0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		fAvgTemp = g_pCArcDev.get()->GetArrayTemperature();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetArrayTemperatureNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetArrayTemperatureNative" );
	}
	
	return fAvgTemp;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetArrayTemperatureDNNative
 * Signature: ()D
 */
JNIEXPORT jdouble JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetArrayTemperatureDNNative( JNIEnv* pEnv, jclass clazz )
{
	double gDn = 0.0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		gDn = g_pCArcDev.get()->GetArrayTemperatureDN();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetArrayTemperatureDNNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetArrayTemperatureDNNative" );
	}
	
	return gDn;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    SetArrayTemperatureNative
 * Signature: (D)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_SetArrayTemperatureNative( JNIEnv* pEnv, jclass clazz, jdouble tempVal )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->SetArrayTemperature( tempVal );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SetArrayTemperatureNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SetArrayTemperatureNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    SetBinningNative
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_arc_api_device_ArcDeviceJAPI_SetBinningNative
( JNIEnv* pEnv, jclass clazz, jint jRows, jint jCols, jint jRowFactor, jint jColFactor )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->SetBinning( jRows,
									jCols,
									jRowFactor,
									jColFactor );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SetBinningNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SetBinningNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    UnSetBinningNative
 * Signature: (II)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_UnSetBinningNative( JNIEnv* pEnv, jclass clazz, jint jRows, jint jCols )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->UnSetBinning( jRows, jCols );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "UnSetBinningNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "UnSetBinningNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    SetOpenShutterNative
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_SetOpenShutterNative( JNIEnv* pEnv, jclass clazz, jboolean jShouldOpen )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->SetOpenShutter( ( jShouldOpen == JNI_TRUE ? true : false ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SetOpenShutterNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SetOpenShutterNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    SetSyntheticImageModeNative
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_SetSyntheticImageModeNative( JNIEnv* pEnv, jclass clazz, jboolean jUseSynMode )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->SetSyntheticImageMode( ( jUseSynMode == JNI_TRUE ? true : false ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SetSyntheticImageModeNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SetSyntheticImageModeNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    IsSyntheticImageModeNative
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_arc_api_device_ArcDeviceJAPI_IsSyntheticImageModeNative( JNIEnv* pEnv, jclass clazz )
{
	bool bIsSet = false;
	
	try
	{
		ASSERT_DEVICE_PTR()

		bIsSet = g_pCArcDev.get()->IsSyntheticImageMode();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "IsSyntheticImageModeNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "IsSyntheticImageModeNative" );
	}
	
	return bIsSet;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    ResetNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_arc_api_device_ArcDeviceJAPI_ResetNative( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->Reset();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "ResetNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "ResetNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    ResetControllerNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_ResetControllerNative( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->ResetController();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "ResetControllerNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "ResetControllerNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    StopExposureNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_StopExposureNative( JNIEnv* pEnv, jclass clazz )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->StopExposure();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "StopExposureNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "StopExposureNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    Set2xFOTransmitterNative
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_Set2xFOTransmitterNative( JNIEnv* pEnv, jclass clazz, jboolean jOnOff )
{
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->Set2xFOTransmitter( ( jOnOff != 0 ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "Set2xFOTransmitterNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "Set2xFOTransmitterNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCfgByteNative
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCfgByteNative( JNIEnv* pEnv, jclass clazz, jint jOffset )
{
	int dRetVal = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		dRetVal =
			( ( CArcPCIBase * )g_pCArcDev.get() )->GetCfgSpByte( int( jOffset ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCfgByteNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCfgByteNative" );
	}
	
	return dRetVal;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCfgWordNative
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCfgWordNative( JNIEnv* pEnv, jclass clazz, jint jOffset )
{
	int dRetVal = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		dRetVal =
			( ( CArcPCIBase * )g_pCArcDev.get() )->GetCfgSpWord( int( jOffset ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCfgWordNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCfgWordNative" );
	}
	
	return dRetVal;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCfgDWordNative
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCfgDWordNative( JNIEnv* pEnv, jclass clazz, jint jOffset )
{
	int dRetVal = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		dRetVal =
			( ( CArcPCIBase * )g_pCArcDev.get() )->GetCfgSpDWord( int( jOffset ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCfgDWordNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCfgDWordNative" );
	}
	
	return dRetVal;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    SetCfgByteNative
 * Signature: (II)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_SetCfgByteNative( JNIEnv* pEnv, jclass clazz, jint jOffset, jint jValue )
{
	try
	{
		ASSERT_DEVICE_PTR()

		( ( CArcPCIBase * )g_pCArcDev.get() )->SetCfgSpByte( int( jOffset ),
														     int( jValue ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SetCfgByteNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SetCfgByteNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    SetCfgWordNative
 * Signature: (II)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_SetCfgWordNative( JNIEnv* pEnv, jclass clazz, jint jOffset, jint jValue )
{
	try
	{
		ASSERT_DEVICE_PTR()

		( ( CArcPCIBase * )g_pCArcDev.get() )->SetCfgSpWord( int( jOffset ),
														     int( jValue ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SetCfgWordNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SetCfgWordNative" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    SetCfgDWordNative
 * Signature: (II)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_SetCfgDWordNative( JNIEnv* pEnv, jclass clazz, jint jOffset, jint jValue )
{
	try
	{
		ASSERT_DEVICE_PTR()

		( ( CArcPCIBase * )g_pCArcDev.get() )->SetCfgSpDWord( int( jOffset ),
															  int( jValue ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SetCfgDWordNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SetCfgDWordNative" );
	}
}


/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    SetupControllerNative
 * Signature: (ZZZIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_arc_api_device_ArcDeviceJAPI_SetupControllerNative
( JNIEnv* pEnv, jclass clazz, jboolean jReset, jboolean jTdl, jboolean jPower,
  jint jRows, jint jCols, jstring jTimFile, jstring jUtilFile, jstring jPciFile )
{
	// Get the lod filenames
	const char* pszTimFile  = ( jTimFile  == NULL ? NULL : pEnv->GetStringUTFChars( jTimFile,  0 ) );
	const char* pszUtilFile = ( jUtilFile == NULL ? NULL : pEnv->GetStringUTFChars( jUtilFile, 0 ) );
	const char* pszPciFile  = ( jPciFile  == NULL ? NULL : pEnv->GetStringUTFChars( jPciFile,  0 ) );
	
	try
	{
		ASSERT_DEVICE_PTR()

		g_pCArcDev.get()->SetupController( ( jReset != 0 ),
										   ( jTdl != 0 ),
										   ( jPower != 0 ),
										   static_cast<int>( jRows ),
										   static_cast<int>( jCols ),
										   ( pszTimFile == NULL  ? "" : pszTimFile  ),
										   ( pszUtilFile == NULL ? "" : pszUtilFile ),
										   ( pszPciFile == NULL  ? "" : pszPciFile  ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "SetupControllerNative", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "SetupControllerNative" );
	}

	// Free the allocated memory for the files
	if ( pszTimFile  != NULL ) pEnv->ReleaseStringUTFChars( jTimFile,  pszTimFile );
	if ( pszUtilFile != NULL ) pEnv->ReleaseStringUTFChars( jUtilFile, pszUtilFile );
	if ( pszPciFile  != NULL ) pEnv->ReleaseStringUTFChars( jPciFile,  pszPciFile );
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCfgSpCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCfgSpCount( JNIEnv* pEnv, jclass clazz )
{
	int dCount = 0;

	try
	{
		ASSERT_DEVICE_PTR()

		( ( CArcPCIBase * )g_pCArcDev.get() )->GetCfgSp();
	
		dCount = ( ( CArcPCIBase * )g_pCArcDev.get() )->GetCfgSpCount();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCfgSpCount", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCfgSpCount" );
	}

	return jint( dCount );
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCfgSpAddr
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCfgSpAddr( JNIEnv* pEnv, jclass clazz, jint jIndex )
{
	int dAddr = 0;

	try
	{
		ASSERT_DEVICE_PTR()

		dAddr = ( ( CArcPCIBase * )g_pCArcDev.get() )->GetCfgSpAddr( int( jIndex ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCfgSpAddr", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCfgSpAddr" );
	}

	return jint( dAddr );
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCfgSpValue
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCfgSpValue( JNIEnv* pEnv, jclass clazz, jint jIndex )
{
	int dValue = 0;

	try
	{
		ASSERT_DEVICE_PTR()

		dValue = ( ( CArcPCIBase * )g_pCArcDev.get() )->GetCfgSpValue( int( jIndex ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCfgSpValue", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCfgSpValue" );
	}

	return jint( dValue );
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCfgSpBitList
 * Signature: (I)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCfgSpBitList( JNIEnv* pEnv, jclass clazz, jint jIndex )
{
	jobjectArray jBitList   = NULL;
	string*      pBitList   = NULL;
	int          dListCount = 0;
	
	try
	{
		ASSERT_DEVICE_PTR()

		//
		// User must free returned array
		//
		pBitList = ( ( CArcPCIBase * )g_pCArcDev.get() )->GetCfgSpBitList( int( jIndex ),
																		   dListCount );
			
		if ( pBitList != NULL && dListCount > 0 )
		{
			jBitList = ( jobjectArray )pEnv->NewObjectArray( dListCount,
															pEnv->FindClass( "java/lang/String" ),
															pEnv->NewStringUTF( "" ) );
				
			if ( jBitList != NULL )
			{
				//
				// Add the bit list items
				//
				for ( int i=0; i<dListCount; i++ )
				{
					pEnv->SetObjectArrayElement( jBitList,
												i,
												pEnv->NewStringUTF( pBitList[ i ].c_str() ) );
						
					if ( pEnv->ExceptionOccurred() != NULL )
					{
						throw std::runtime_error( "Failed to add bit to list!" );
					}
				}
			}
			else
			{
				throw std::runtime_error( "Failed create array for bit list!" );
			}
		}
	}
	catch ( std::exception &e )
	{
		if ( pBitList != NULL )
		{
			delete [] pBitList;
		}
		
		ThrowJNIException( pEnv, "GetCfgSpBitList", e.what() );
	}
	catch ( ... )
	{
		if ( pBitList != NULL )
		{
			delete [] pBitList;
		}
		
		ThrowUnknownJNIException( pEnv, "GetCfgSpBitList" );
	}
	
	return jBitList;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetCfgSpName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetCfgSpName( JNIEnv* pEnv, jclass clazz, jint jIndex )
{
	jstring jsName = NULL;

	try
	{
		ASSERT_DEVICE_PTR()

		jsName = pEnv->NewStringUTF(
					( ( CArcPCIBase * )g_pCArcDev.get() )->GetCfgSpName(
															int( jIndex ) ).c_str() );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetCfgSpName", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetCfgSpName" );
	}

	return jsName;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetBarCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetBarCount( JNIEnv* pEnv, jclass clazz )
{
	int dCount = 0;

	try
	{
		ASSERT_DEVICE_PTR()

		( ( CArcPCIBase * )g_pCArcDev.get() )->GetBarSp();
		
		dCount = ( ( CArcPCIBase * )g_pCArcDev.get() )->GetBarCount();
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetBarCount", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetBarCount" );
	}

	return jint( dCount );
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetBarName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetBarName( JNIEnv* pEnv, jclass clazz, jint jIndex )
{
	jstring jsName = NULL;

	try
	{
		ASSERT_DEVICE_PTR()

		jsName = pEnv->NewStringUTF(
					( ( CArcPCIBase * )g_pCArcDev.get() )->GetBarName(
														int( jIndex ) ).c_str() );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetBarName", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetBarName" );
	}

	return jsName;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetBarRegCount
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetBarRegCount( JNIEnv* pEnv, jclass clazz, jint jIndex )
{
	int dCount = 0;

	try
	{
		ASSERT_DEVICE_PTR()

		dCount = ( ( CArcPCIBase * )g_pCArcDev.get() )->GetBarRegCount( int( jIndex ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetBarRegCount", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetBarRegCount" );
	}

	return jint( dCount );
}
/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetBarRegAddr
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetBarRegAddr( JNIEnv* pEnv, jclass clazz, jint jIndex, jint jRegIndex )
{
	int dAddr = 0;

	try
	{
		ASSERT_DEVICE_PTR()

		dAddr = ( ( CArcPCIBase * )g_pCArcDev.get() )->GetBarRegAddr( int( jIndex ),
																	  int( jRegIndex ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetBarRegAddr", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetBarRegAddr" );
	}

	return jint( dAddr );
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetBarRegValue
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetBarRegValue( JNIEnv* pEnv, jclass clazz, jint jIndex, jint jRegIndex )
{
	int dValue = 0;

	try
	{
		ASSERT_DEVICE_PTR()

		dValue = ( ( CArcPCIBase * )g_pCArcDev.get() )->GetBarRegValue( int( jIndex ),
																	    int( jRegIndex ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetBarRegValue", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetBarRegValue" );
	}

	return dValue;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetBarRegName
 * Signature: (II)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetBarRegName( JNIEnv* pEnv, jclass clazz, jint jIndex, jint jRegIndex )
{
	jstring jsName = NULL;

	try
	{
		ASSERT_DEVICE_PTR()

		jsName = pEnv->NewStringUTF(
						( ( CArcPCIBase * )g_pCArcDev.get() )->GetBarRegName( int( jIndex ),
																			  int( jRegIndex ) ).c_str() );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetBarRegName", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetBarRegName" );
	}

	return jsName;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetBarRegBitListCount
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetBarRegBitListCount( JNIEnv* pEnv, jclass clazz, jint jIndex, jint jRegIndex )
{
	int dCount = 0;

	try
	{
		ASSERT_DEVICE_PTR()

		dCount = ( ( CArcPCIBase * )g_pCArcDev.get() )->GetBarRegBitListCount( int( jIndex ),
																			   int( jRegIndex ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetBarRegBitListCount", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetBarRegBitListCount" );
	}

	return jint( dCount );
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    GetBarRegBitListDef
 * Signature: (III)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_arc_api_device_ArcDeviceJAPI_GetBarRegBitListDef( JNIEnv* pEnv, jclass clazz, jint jIndex, jint jRegIndex, jint jBitListIndex )
{
	jstring jsBitDef = NULL;

	try
	{
		ASSERT_DEVICE_PTR()

		jsBitDef = pEnv->NewStringUTF(
					  ( ( CArcPCIBase * )g_pCArcDev.get() )->GetBarRegBitListDef( int( jIndex ),
																				  int( jRegIndex ),
																				  int( jBitListIndex ) ).c_str() );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "GetBarRegBitListDef", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "GetBarRegBitListDef" );
	}

	return jsBitDef;
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    WriteBar
 * Signature: (III)V
 */
JNIEXPORT void JNICALL
Java_arc_api_device_ArcDeviceJAPI_WriteBar( JNIEnv* pEnv, jclass clazz, jint jBar, jint jReg, jint jValue )
{
	string sID( g_pCArcDev.get()->ToString() );

	try
	{
		ASSERT_DEVICE_PTR()

		if ( sID.find( "PCIe" ) == string::npos )
		{
			throw runtime_error( "WriteBar only available for ARC-66 ( PCIe )!" );
		}

		( ( CArcPCIe * )g_pCArcDev.get() )->WriteBar( int( jBar ),
													  int( jReg ),
													  ( 0xAC000000 |  int( jValue ) ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "WriteBar", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "WriteBar" );
	}
}

/*
 * Class:     arc_api_device_ArcDeviceJAPI
 * Method:    ReadBar
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_arc_api_device_ArcDeviceJAPI_ReadBar
( JNIEnv* pEnv, jclass clazz, jint jBar, jint jReg )
{
	int dValue = 0;

	string sID( g_pCArcDev.get()->ToString() );

	try
	{
		ASSERT_DEVICE_PTR()

		if ( sID.find( "PCIe" ) == string::npos )
		{
			throw runtime_error( "ReadBar only available for ARC-66 ( PCIe )!" );
		}

		dValue = ( ( CArcPCIe * )g_pCArcDev.get() )->ReadBar( int( jBar ),
															  int( jReg ) );
	}
	catch ( std::exception &e )
	{
		ThrowJNIException( pEnv, "ReadBar", e.what() );
	}
	catch ( ... )
	{
		ThrowUnknownJNIException( pEnv, "ReadBar" );
	}

	return dValue;
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

		oss << "( ArcDeviceJAPI::" << sMethod << " ): " << sMsg << ends;

		pEnv->ThrowNew( newExcCls, oss.str().c_str() );
	}
}

void ThrowUnknownJNIException( JNIEnv* pEnv, string sMethod )
{
	jclass newExcCls = pEnv->FindClass( "java/lang/Exception" );
	
	if ( newExcCls != NULL )
	{
		pEnv->ThrowNew( newExcCls,
					   ( "( ArcDeviceJAPI::" + sMethod +
					   " ): An unknown error caused an exception!" ).c_str() );
	}
}
