#ifndef _ARC_DEINTERLACE_CAPI_H_
#define _ARC_DEINTERLACE_CAPI_H_

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


#ifdef __cplusplus
   extern "C" {		// Using a C++ compiler
#endif

// +------------------------------------------------------------------------------------+
// |  C Interface - Deinterlace Alogrithms                                              |
// +------------------------------------------------------------------------------------+
CDEINTERLACE_API extern const int DEINTERLACE_NONE;
CDEINTERLACE_API extern const int DEINTERLACE_PARALLEL;
CDEINTERLACE_API extern const int DEINTERLACE_SERIAL;
CDEINTERLACE_API extern const int DEINTERLACE_CCD_QUAD;
CDEINTERLACE_API extern const int DEINTERLACE_IR_QUAD;
CDEINTERLACE_API extern const int DEINTERLACE_CDS_IR_QUAD;
CDEINTERLACE_API extern const int DEINTERLACE_HAWAII_RG;
CDEINTERLACE_API extern const int DEINTERLACE_STA1600;


// +------------------------------------------------------------------------------------+
// |  C Interface - Deinterlace Functions                                               |
// +------------------------------------------------------------------------------------+
CDEINTERLACE_API void ArcDeinterlace_RunAlg( void* pData, int dRows, int dCols, int dAlgorithm, int* pStatus );

CDEINTERLACE_API void ArcDeinterlace_RunAlgWArg( void* pData, int dRows, int dCols, int dAlgorithm, int dArg, int* pStatus );

CDEINTERLACE_API const char* ArcDeinterlace_GetLastError();

#ifdef __cplusplus
   }
#endif

#endif		// _ARC_DEINTERLACE_CAPI_H_
