#ifndef _CARC_DEVICE_H_
#define _CARC_DEVICE_H_


#include "CExpIFace.h"
#include "CConIFace.h"
#include "TempCtrl.h"
#include "DllMain.h"
#include "ArcOSDefs.h"
#include "CLog.h"

#if defined( linux ) || defined( __linux )
	#include <sys/types.h>
#endif


namespace arc
{
	namespace device
	{

		// +------------------------------------------------+
		// | Image buffer info                              |
		// +------------------------------------------------+
		typedef struct ARC_DEVICE_BUFFER
		{
			void*  pUserAddr;
			ulong  ulPhysicalAddr;
			size_t dSize;
		} ImgBuf_t;


		// +------------------------------------------------+
		// | Device info                                    |
		// +------------------------------------------------+
		typedef struct ARC_DEVICE_INFO
		{
			std::string			sName;

	#ifdef __APPLE__
			io_service_t		tService;
	#endif
		} ArcDev_t;


		// +------------------------------------------------+
		// | CArcDevice class definition                    |
		// +------------------------------------------------+
		class CARCDEVICE_API CArcDevice
		{
			public:
				CArcDevice( void );
				virtual ~CArcDevice( void );

				virtual const std::string ToString() = 0;


				//  Device access
				// +-------------------------------------------------+
				virtual bool IsOpen();
				virtual void Open( int dDeviceNumber = 0 ) = 0;
				virtual void Open( int dDeviceNumber, int dBytes ) = 0;
				virtual void Open( int dDeviceNumber, int dRows, int dCols ) = 0;
				virtual void Close() = 0;
				virtual void Reset() = 0;

				virtual void  MapCommonBuffer( int dBytes = 0 ) = 0;
				virtual void  UnMapCommonBuffer() = 0;
				virtual void  ReMapCommonBuffer( int dBytes = 0 );
				virtual void  FillCommonBuffer( unsigned short u16Value = 0 );
				virtual void* CommonBufferVA();
				virtual ulong CommonBufferPA();
				virtual int   CommonBufferSize();

				virtual int  GetId() = 0;
				virtual int  GetStatus() = 0;
				virtual void ClearStatus() = 0;

				virtual void Set2xFOTransmitter( bool bOnOff ) = 0;
				virtual void LoadDeviceFile( const std::string sFile ) = 0;


				//  Setup & General commands
				// +-------------------------------------------------+
				virtual int  Command( int dBoardId, int dCommand, int dArg1 = NOPARAM, int dArg2 = NOPARAM, int dArg3 = NOPARAM, int dArg4 = NOPARAM ) = 0;
				virtual int  GetControllerId() = 0;
				virtual void ResetController() = 0;
				virtual bool IsControllerConnected() = 0;

				virtual void SetupController( bool bReset, bool bTdl, bool bPower, int dRows, int dCols, const std::string sTimFile,
											  const std::string sUtilFile = "", const std::string sPciFile = "",
											  const bool& bAbort = false );

				virtual void LoadControllerFile( const std::string sFilename, bool bValidate = true, const bool& bAbort = false );
				virtual void SetImageSize( int dRows, int dCols );

				virtual int  GetImageRows();
				virtual int  GetImageCols();
				virtual int  GetCCParams();
				virtual bool IsCCParamSupported( int dParameter );
				virtual bool IsCCD();

				virtual bool IsBinningSet();
				virtual void SetBinning( int dRows, int dCols, int dRowFactor, int dColFactor, int* pBinRows = NULL, int* pBinCols = NULL );
				virtual void UnSetBinning( int dRows, int dCols );

				virtual void SetSubArray( int& dOldRows, int& dOldCols, int dRow, int dCol, int dSubRows, int dSubCols, int dBiasOffset, int dBiasWidth );
				virtual void UnSetSubArray( int dRows, int dCols );

				virtual bool IsSyntheticImageMode();
				virtual void SetSyntheticImageMode( bool bMode );


				//  Expose commands
				// +-------------------------------------------------+
				virtual void SetOpenShutter( bool bShouldOpen );

				virtual void Expose( float fExpTime, int dRows, int dCols, const bool& bAbort = false,
									 CExpIFace* pExpIFace = NULL, bool bOpenShutter = true );

				virtual void StopExposure() = 0;

				virtual void Continuous( int dRows, int dCols, int dNumOfFrames, float fExpTime, const bool& bAbort = false,
										 CConIFace* pConIFace = NULL, bool bOpenShutter = true );

				virtual void StopContinuous();

				virtual bool IsReadout() = 0;
				virtual int  GetPixelCount() = 0;
				virtual int  GetCRPixelCount() = 0;
				virtual int  GetFrameCount() = 0;


				//  Error & Degug message access
				// +-------------------------------------------------+
				virtual bool ContainsError( int dWord );
				virtual bool ContainsError( int dWord, int dWordMin, int dWordMax );

				virtual const std::string	GetNextLoggedCmd();
				virtual int					GetLoggedCmdCount();
				virtual void				SetLogCmds( bool bOnOff );


				//  Temperature control
				// +-------------------------------------------------+
				virtual double GetArrayTemperature();
				virtual double GetArrayTemperatureDN();
				virtual void   SetArrayTemperature( double gTempVal );
				virtual void   LoadTemperatureCtrlData( const std::string sFilename );
				virtual void   SaveTemperatureCtrlData( const std::string sFilename );


				//  Maximum number of command parameters the controller will accept 
				// +------------------------------------------------------------------+
				static const int CTLR_CMD_MAX		= 6;


				//  Timeout loop count for image readout                             
				// +------------------------------------------------------------------+
				static const int READ_TIMEOUT		= 200;


				//  Invalid parameter value                           
				// +------------------------------------------------------------------+
				static const int NOPARAM			= -1;


			protected:
				virtual bool  GetCommonBufferProperties() = 0;

				virtual void   SetDefaultTemperatureValues();
				virtual double ADUToVoltage( int dAdu, bool bArc12 = false, bool bHighGain = false );
				virtual double VoltageToADU( double gVoltage, bool bArc12 = false, bool bHighGain = false );
				virtual double CalculateAverageTemperature();
				virtual double CalculateVoltage( double gTemperature );
				virtual double CalculateTemperature( double gVoltage );

				virtual int  GetContinuousImageSize( int dImageSize ) = 0;
				virtual int  SmallCamDLoad( int dBoardId, std::vector<int>& vData ) = 0;
				virtual void LoadSmallCamControllerFile( const std::string sFilename, bool bValidate, const bool& bAbort = false );
				virtual void LoadGen23ControllerFile( const std::string sFilename, bool bValidate, const bool& bAbort = false ) = 0;
				virtual void SetByteSwapping() = 0;

				virtual std::string FormatDLoadString( int dReply, int dBoardId, std::vector<int>& vData );

				//  Temperature control variables
				// +--------------------------------------+
				double gTmpCtrl_DT670Coeff1;
				double gTmpCtrl_DT670Coeff2;
				double gTmpCtrl_SDAduOffset;
				double gTmpCtrl_SDAduPerVolt;
				double gTmpCtrl_HGAduOffset;
				double gTmpCtrl_HGAduPerVolt;
				double gTmpCtrl_SDVoltTolerance;
				double gTmpCtrl_SDDegTolerance;
				int    gTmpCtrl_SDNumberOfReads;
				int    gTmpCtrl_SDVoltToleranceTrials;

				TmpCtrlCoeff_t TmpCtrl_SD_2_12K;
				TmpCtrlCoeff_t TmpCtrl_SD_12_24K;
				TmpCtrlCoeff_t TmpCtrl_SD_24_100K;
				TmpCtrlCoeff_t TmpCtrl_SD_100_475K;

				Arc_DevHandle	m_hDevice;		// Driver file descriptor
				CLog			m_cApiLog;
				ImgBuf_t 		m_tImgBuffer;
				bool	 		m_bStoreCmds;	// 'true' stores cmd strings in queue
				int		 		m_dCCParam;
		};

	}	// end device namespace
}	// end arc namespace


#endif	// _CARC_DEVICE_H_
