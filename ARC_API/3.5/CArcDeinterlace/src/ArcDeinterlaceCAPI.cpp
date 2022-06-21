// ArcDeinterlaceCAPI.cpp : Defines the exported functions for the DLL application.
//
#ifndef WIN32
	#include <cstdio>
#endif

#include "DllMain.h"
#include "ArcDeinterlaceCAPI.h"
#include "CArcDeinterlace.h"

using namespace std;
using namespace arc::deinterlace;



// +------------------------------------------------------------------------------------+
// | Deinterlace constants
// +------------------------------------------------------------------------------------+
const int DEINTERLACE_NONE        = CArcDeinterlace::DEINTERLACE_NONE;
const int DEINTERLACE_PARALLEL    = CArcDeinterlace::DEINTERLACE_PARALLEL;
const int DEINTERLACE_SERIAL      = CArcDeinterlace::DEINTERLACE_SERIAL;
const int DEINTERLACE_CCD_QUAD    = CArcDeinterlace::DEINTERLACE_CCD_QUAD;
const int DEINTERLACE_IR_QUAD     = CArcDeinterlace::DEINTERLACE_IR_QUAD;
const int DEINTERLACE_CDS_IR_QUAD = CArcDeinterlace::DEINTERLACE_CDS_IR_QUAD;
const int DEINTERLACE_HAWAII_RG   = CArcDeinterlace::DEINTERLACE_HAWAII_RG;
const int DEINTERLACE_STA1600	  = CArcDeinterlace::DEINTERLACE_STA1600;


// +------------------------------------------------------------------------------------+
// | Globals
// +------------------------------------------------------------------------------------+
static auto_ptr<CArcDeinterlace> g_pCDLacer( 0 );
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
							ptr.reset( new CArcDeinterlace() );					\
																				\
							if ( ptr.get() == NULL )							\
							{													\
								throw runtime_error( string( "( " ) + fnc +		\
								string( " ): Invalid class pointer!" ) );		\
							}													\
						}


// +--------------------------------------------------------------------------------------------
// | RunAlg - Deinterlace Image
// +--------------------------------------------------------------------------------------------
// |
// | DESCRIPTION:	The deinterlacing algorithms work on the principle that the
// | ccd/ir array will read out the data in a predetermined order depending on
// | the type of readout being implemented.  Here's how they look:     			
// | 
// | split-parallel          split-serial              quad CCD                quad IR          	
// | ----------------       ----------------        ----------------        ----------------     	
// | |     1  ------->|     |        |------>|      |<-----  |  ---->|      | -----> | ----> |    	
// | |                |     |        |   1   |      |   3    |   2   |      |   0    |   1   |    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |_______________ |     |        |       |      |________|_______|      |________|_______|    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |                |     |   0    |       |      |   0    |   1   |      |   3    |   2   |    	
// | |<--------  0    |     |<------ |       |      |<-----  |  ---->|      | -----> | ----> |    	
// |  ----------------       ----------------        ----------------        ----------------     	
// | 
// | CDS quad IR          	
// | ----------------     	
// | | -----> | ----> |    	
// | |   0    |   1   |    	
// | |        |       |	               HawaiiRG
// | |________|_______|    ---------------------------------
// | |        |       |    |       |       |       |       |
// | |        |       |    |       |       |       |       |
// | |   3    |   2   |    |       |       |       |       |
// | | -----> | ----> |    |       |       |       |       |
// |  ----------------     |       |       |       |       |
// | | -----> | ----> |    |       |       |       |       |
// | |   0    |   1   |    |   0   |   1   |   2   |   3   |
// | |        |       |    | ----> | ----> | ----> | ----> |
// | |________|_______|    ---------------------------------
// | |        |       |    	
// | |        |       |    	
// | |   3    |   2   |    	
// | | -----> | ----> |    	
// | ----------------   
// |
// |  <IN>  -> pData      - Pointer to the image pData to deinterlace
// |  <IN>  -> dCols      - Number of dCols in image to deinterlace
// |  <IN>  -> dRows      - Number of rows in image to deinterlace
// |  <IN>  -> dAlgorithm - Algorithm number that corresponds to deint. method
// |  <OUT> -> pStatus    - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +--------------------------------------------------------------------------------------------
CDEINTERLACE_API void ArcDeinterlace_RunAlg( void *pData, int dRows, int dCols, int dAlgorithm, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDeinterlace_RunAlg", g_pCDLacer )

		g_pCDLacer.get()->RunAlg( pData, dRows, dCols, dAlgorithm );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +--------------------------------------------------------------------------------------------
// | RunAlg - Deinterlace Image
// +--------------------------------------------------------------------------------------------
// |
// | DESCRIPTION:	The deinterlacing algorithms work on the principle that the
// | ccd/ir array will read out the data in a predetermined order depending on
// | the type of readout being implemented.  Here's how they look:     			
// | 
// | split-parallel          split-serial              quad CCD                quad IR          	
// | ----------------       ----------------        ----------------        ----------------     	
// | |     1  ------->|     |        |------>|      |<-----  |  ---->|      | -----> | ----> |    	
// | |                |     |        |   1   |      |   3    |   2   |      |   0    |   1   |    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |_______________ |     |        |       |      |________|_______|      |________|_______|    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |                |     |   0    |       |      |   0    |   1   |      |   3    |   2   |    	
// | |<--------  0    |     |<------ |       |      |<-----  |  ---->|      | -----> | ----> |    	
// |  ----------------       ----------------        ----------------        ----------------     	
// | 
// | CDS quad IR          	
// | ----------------     	
// | | -----> | ----> |    	
// | |   0    |   1   |    	
// | |        |       |	               HawaiiRG
// | |________|_______|    ---------------------------------
// | |        |       |    |       |       |       |       |
// | |        |       |    |       |       |       |       |
// | |   3    |   2   |    |       |       |       |       |
// | | -----> | ----> |    |       |       |       |       |
// |  ----------------     |       |       |       |       |
// | | -----> | ----> |    |       |       |       |       |
// | |   0    |   1   |    |   0   |   1   |   2   |   3   |
// | |        |       |    | ----> | ----> | ----> | ----> |
// | |________|_______|    ---------------------------------
// | |        |       |    	
// | |        |       |    	
// | |   3    |   2   |    	
// | | -----> | ----> |    	
// | ----------------   
// |
// |  <IN>  -> pData      - Pointer to the image pData to deinterlace
// |  <IN>  -> dCols      - Number of dCols in image to deinterlace
// |  <IN>  -> dRows      - Number of rows in image to deinterlace
// |  <IN>  -> dAlgorithm - Algorithm number that corresponds to deint. method
// |  <IN>  -> dArg       - Any algorithm specific argument
// |  <OUT> -> pStatus    - Status equals ARC_STATUS_OK or ARC_STATUS_ERROR
// +--------------------------------------------------------------------------------------------
CDEINTERLACE_API void ArcDeinterlace_RunAlgWArg( void *pData, int dRows, int dCols, int dAlgorithm, int dArg, int* pStatus )
{
	*pStatus = ARC_STATUS_OK;

	try
	{
		VERIFY_CLASS_PTR( "ArcDeinterlace_RunAlg", g_pCDLacer )

		g_pCDLacer.get()->RunAlg( pData, dRows, dCols, dAlgorithm, dArg );
	}
	catch ( exception& e )
	{
		*pStatus = ARC_STATUS_ERROR;

		ArcSprintf( g_szErrMsg, ARC_ERROR_MSG_SIZE, "%s", e.what() );
	}
}


// +--------------------------------------------------------------------------------------------
// | GetLastError
// +--------------------------------------------------------------------------------------------
// | Returns the last error message reported.
// +--------------------------------------------------------------------------------------------
CDEINTERLACE_API const char* ArcDeinterlace_GetLastError()
{
	return const_cast<const char *>( g_szErrMsg );
}

