#ifndef _ARC_DEVICE_CAPI_H_
#define _ARC_DEVICE_CAPI_H_

#include "DllMain.h"


// +------------------------------------------------------------------------------------+
// | Status/Error constants                                                             |
// +------------------------------------------------------------------------------------+
#ifndef ARC_STATUS
#define ARC_STATUS

	#define ARC_STATUS_OK			0
	#define ARC_STATUS_ERROR		1
	#define ARC_MSG_SIZE			128
	#define ARC_ERROR_MSG_SIZE		128

#endif


#ifdef __cplusplus
   extern "C" {		// Using a C++ compiler
#endif


// +----------------------------------------------------------------------------------------------------------------------------+
// | Device constants                                                                                                           |
// +----------------------------------------------------------------------------------------------------------------------------+
CARCDEVICE_API const int extern DEVICE_NOPARAM;


// +----------------------------------------------------------------------------------------------------------------------------+
// | Device access                                                                                                              |
// +----------------------------------------------------------------------------------------------------------------------------+
CARCDEVICE_API const char* ArcDevice_ToString( int* pStatus );

CARCDEVICE_API void ArcDevice_FindDevices( int* pStatus );
CARCDEVICE_API int  ArcDevice_DeviceCount();
CARCDEVICE_API const char** ArcDevice_GetDeviceStringList( int* pStatus );
CARCDEVICE_API void ArcDevice_FreeDeviceStringList();

CARCDEVICE_API int  ArcDevice_IsOpen( int* pStatus );
CARCDEVICE_API void ArcDevice_Open( int dDeviceNumber, int* pStatus );
CARCDEVICE_API void ArcDevice_Open_I( int dDeviceNumber, int dBytes, int* pStatus );
CARCDEVICE_API void ArcDevice_Open_II( int dDeviceNumber, int dRows, int dCols, int* pStatus );
CARCDEVICE_API void ArcDevice_Close();
CARCDEVICE_API void ArcDevice_Reset( int* pStatus );

CARCDEVICE_API void  ArcDevice_MapCommonBuffer( int dBytes, int* pStatus );
CARCDEVICE_API void  ArcDevice_UnMapCommonBuffer( int* pStatus );
CARCDEVICE_API void  ArcDevice_ReMapCommonBuffer( int dBytes, int* pStatus );
CARCDEVICE_API void  ArcDevice_FillCommonBuffer( unsigned short u16Value, int* pStatus );
CARCDEVICE_API void* ArcDevice_CommonBufferVA( int* pStatus );
CARCDEVICE_API unsigned long ArcDevice_CommonBufferPA( int* pStatus );
CARCDEVICE_API int   ArcDevice_CommonBufferSize( int* pStatus );

CARCDEVICE_API int  ArcDevice_GetId( int* pStatus );
CARCDEVICE_API int  ArcDevice_GetStatus( int* pStatus );
CARCDEVICE_API void ArcDevice_ClearStatus( int* pStatus );

CARCDEVICE_API void ArcDevice_Set2xFOTransmitter( int bOnOff, int* pStatus );
CARCDEVICE_API void ArcDevice_LoadDeviceFile( const char* pszFile, int* pStatus );

// +----------------------------------------------------------------------------------------------------------------------------+
// | Setup & general commands                                                                                                    |
// +----------------------------------------------------------------------------------------------------------------------------+
CARCDEVICE_API int  ArcDevice_Command( int dBoardId, int dCommand, int* pStatus );
CARCDEVICE_API int  ArcDevice_Command_I( int dBoardId, int dCommand, int dArg1, int* pStatus );
CARCDEVICE_API int  ArcDevice_Command_II( int dBoardId, int dCommand, int dArg1, int dArg2, int* pStatus );
CARCDEVICE_API int  ArcDevice_Command_III( int dBoardId, int dCommand, int dArg1, int dArg2, int dArg3, int* pStatus );
CARCDEVICE_API int  ArcDevice_Command_IIII( int dBoardId, int dCommand, int dArg1, int dArg2, int dArg3, int dArg4, int* pStatus );

CARCDEVICE_API int  ArcDevice_GetControllerId( int* pStatus );
CARCDEVICE_API void ArcDevice_ResetController( int* pStatus );
CARCDEVICE_API int  ArcDevice_IsControllerConnected( int* pStatus );

CARCDEVICE_API void ArcDevice_SetupController( int bReset, int bTdl, int bPower, int dRows, int dCols, const char* pszTimFile,
											   const char* pszUtilFile, const char* pszPciFile, int* pStatus );

CARCDEVICE_API void ArcDevice_LoadControllerFile( const char* pszFilename, int bValidate, int* pStatus );
CARCDEVICE_API void ArcDevice_SetImageSize( int dRows, int dCols, int* pStatus );

CARCDEVICE_API int  ArcDevice_GetImageRows( int* pStatus );
CARCDEVICE_API int  ArcDevice_GetImageCols( int* pStatus );
CARCDEVICE_API int  ArcDevice_GetCCParams( int* pStatus );
CARCDEVICE_API int  ArcDevice_IsCCParamSupported( int dParameter, int* pStatus );
CARCDEVICE_API int  ArcDevice_IsCCD( int* pStatus );

CARCDEVICE_API int  ArcDevice_IsBinningSet( int* pStatus );
CARCDEVICE_API void ArcDevice_SetBinning( int dRows, int dCols, int dRowFactor, int dColFactor, int* pBinRows, int* pBinCols, int* pStatus );
CARCDEVICE_API void ArcDevice_UnSetBinning( int dRows, int dCols, int* pStatus );

CARCDEVICE_API void ArcDevice_SetSubArray( int* pOldRows, int* pOldCols, int dRow, int dCol, int dSubRows,
										   int dSubCols, int dBiasOffset, int dBiasWidth, int* pStatus );

CARCDEVICE_API void ArcDevice_UnSetSubArray( int dRows, int dCols, int* pStatus );

CARCDEVICE_API int  ArcDevice_IsSyntheticImageMode( int* pStatus );
CARCDEVICE_API void ArcDevice_SetSyntheticImageMode( int bMode, int* pStatus );

// +----------------------------------------------------------------------------------------------------------------------------+
// | Expose commands                                                                                                            |
// +----------------------------------------------------------------------------------------------------------------------------+
CARCDEVICE_API void ArcDevice_SetOpenShutter( int bShouldOpen, int* pStatus );

CARCDEVICE_API void ArcDevice_Expose( float fExpTime, int dRows, int dCols, void ( *pExposeCall )( float ),
									  void ( *pReadCall )( int ), int bOpenShutter, int* pStatus );

CARCDEVICE_API void ArcDevice_StopExposure( int* pStatus );

CARCDEVICE_API void ArcDevice_Continuous( int dRows, int dCols, int dNumOfFrames, float fExpTime,
										  void ( *pFrameCall )( int, int, int, int, void * ),
										  int bOpenShutter, int* pStatus );

CARCDEVICE_API void ArcDevice_StopContinuous( int* pStatus );

CARCDEVICE_API int ArcDevice_IsReadout( int* pStatus );
CARCDEVICE_API int ArcDevice_GetPixelCount( int* pStatus );
CARCDEVICE_API int ArcDevice_GetCRPixelCount( int* pStatus );
CARCDEVICE_API int ArcDevice_GetFrameCount( int* pStatus );

// +----------------------------------------------------------------------------------------------------------------------------+
// | Error & Degug message access                                                                                               |
// +----------------------------------------------------------------------------------------------------------------------------+
CARCDEVICE_API int ArcDevice_ContainsError( int dWord, int* pStatus );
CARCDEVICE_API int ArcDevice_ContainsError_I( int dWord, int dWordMin, int dWordMax, int* pStatus );

CARCDEVICE_API const char*	ArcDevice_GetNextLoggedCmd( int* pStatus );
CARCDEVICE_API int			ArcDevice_GetLoggedCmdCount( int* pStatus );
CARCDEVICE_API void			ArcDevice_SetLogCmds( int bOnOff, int* pStatus );

// +----------------------------------------------------------------------------------------------------------------------------+
// | Temperature control                                                                                                        |
// +----------------------------------------------------------------------------------------------------------------------------+
CARCDEVICE_API  double ArcDevice_GetArrayTemperature( int* pStatus );
CARCDEVICE_API  double ArcDevice_GetArrayTemperatureDN( int* pStatus );
CARCDEVICE_API  void   ArcDevice_SetArrayTemperature( double gTempVal, int* pStatus );
CARCDEVICE_API  void   ArcDevice_LoadTemperatureCtrlData( const char* pszFilename, int* pStatus );
CARCDEVICE_API  void   ArcDevice_SaveTemperatureCtrlData( const char* pszFilename, int* pStatus );

CARCDEVICE_API const char* ArcDevice_GetLastError();

#ifdef __cplusplus
   }
#endif

#endif		// _ARC_IMAGE_CAPI_H_
