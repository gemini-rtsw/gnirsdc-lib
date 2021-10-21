#ifndef _CARC_PCI_H_
#define _CARC_PCI_H_

#ifdef WIN32
#pragma warning( disable: 4251 )
#endif

#include <vector>
#include <string>
#include <memory>
#include "CArcPCIBase.h"
#include "CStringList.h"
#include "DllMain.h"




namespace arc
{
	namespace device
	{

		class CARCDEVICE_API CArcPCI : public CArcPCIBase
		{
			public:
				 CArcPCI( void );
				~CArcPCI( void );

				const std::string ToString();

				//  CArcPCIBase methods
				// +-------------------------------------------------+
				int  GetCfgSpByte( int dOffset );
				int  GetCfgSpWord( int dOffset );
				int  GetCfgSpDWord( int dOffset );

				void SetCfgSpByte( int dOffset, int dValue );
				void SetCfgSpWord( int dOffset, int dValue );
				void SetCfgSpDWord( int dOffset, int dValue );

				void GetCfgSp();
				void GetBarSp();


				//  Device access
				// +-------------------------------------------------+
				static void FindDevices();
				static int  DeviceCount();
				static const std::string* GetDeviceStringList();

				bool IsOpen();
				void Open( int dDeviceNumber = 0 );
				void Open( int dDeviceNumber, int dBytes );
				void Open( int dDeviceNumber, int dRows, int dCols );
				void Close();
				void Reset();

				bool GetCommonBufferProperties();
				void MapCommonBuffer( int dBytes = 0 );
				void UnMapCommonBuffer();

				int  GetId();
				int  GetStatus();
				void ClearStatus();
				void SetStatus( int dStatus );

				void Set2xFOTransmitter( bool bOnOff );
				void LoadDeviceFile( const std::string sFilename );


				//  Setup & General commands
				// +-------------------------------------------------+
				int  Command( int dBoardId, int dCommand, int dArg1 = -1, int dArg2 = -1, int dArg3 = -1, int dArg4 = -1 );

				int  GetControllerId();
				void ResetController();
				bool IsControllerConnected();


				//  Expose commands
				// +-------------------------------------------------+
				void StopExposure();
				bool IsReadout();
				int  GetPixelCount();
				int  GetCRPixelCount();
				int  GetFrameCount();


				//  PCI only commands
				// +-------------------------------------------------+
				void SetHctr( int dVal );
				int  GetHstr();
				int  GetHctr();

				int  PCICommand( int dCommand );
				int  IoctlDevice( int dIoctlCmd, int dArg = -1 );
				int  IoctlDevice( int dIoctlCmd, int dArg[], int dArgCount );


				//  Driver ioctl commands
				// +------------------------------------------------------------------------------
				static const int ASTROPCI_GET_HCTR				= 0x01;
				static const int ASTROPCI_GET_PROGRESS			= 0x02;
				static const int ASTROPCI_GET_DMA_ADDR			= 0x03;
				static const int ASTROPCI_GET_HSTR				= 0x04;
				static const int ASTROPCI_MEM_MAP				= 0x05;
				static const int ASTROPCI_GET_DMA_SIZE			= 0x06;
				static const int ASTROPCI_GET_FRAMES_READ		= 0x07;
				static const int ASTROPCI_HCVR_DATA				= 0x10;
				static const int ASTROPCI_SET_HCTR				= 0x11;
				static const int ASTROPCI_SET_HCVR				= 0x12;
				static const int ASTROPCI_PCI_DOWNLOAD			= 0x13;
				static const int ASTROPCI_PCI_DOWNLOAD_WAIT		= 0x14;
				static const int ASTROPCI_COMMAND				= 0x15;
				static const int ASTROPCI_MEM_UNMAP				= 0x16;
				static const int ASTROPCI_ABORT					= 0x17;
				static const int ASTROPCI_CONTROLLER_DOWNLOAD	= 0x19;
				static const int ASTROPCI_GET_CR_PROGRESS		= 0x20;
				static const int ASTROPCI_GET_DMA_LO_ADDR		= 0x21;
				static const int ASTROPCI_GET_DMA_HI_ADDR		= 0x22;
				static const int ASTROPCI_GET_CONFIG_BYTE		= 0x30;
				static const int ASTROPCI_GET_CONFIG_WORD		= 0x31;
				static const int ASTROPCI_GET_CONFIG_DWORD		= 0x32;
				static const int ASTROPCI_SET_CONFIG_BYTE		= 0x33;
				static const int ASTROPCI_SET_CONFIG_WORD		= 0x34;
				static const int ASTROPCI_SET_CONFIG_DWORD		= 0x35;


				//  Status register ( HSTR ) constants
				// +------------------------------------------------------------------------------
				static const int HTF_BIT_MASK					= 0x00000038;

				enum {
					TIMEOUT_STATUS = 0,
					DONE_STATUS,
					READ_REPLY_STATUS,
					ERROR_STATUS,
					SYSTEM_RESET_STATUS,
					READOUT_STATUS,
					BUSY_STATUS
				};


				//  PCI commands
				// +----------------------------------------------------------------------------
				static const int PCI_RESET						= 0x8077;
				static const int ABORT_READOUT					= 0x8079;
				static const int BOOT_EEPROM					= 0x807B;
				static const int READ_HEADER					= 0x81;
				static const int RESET_CONTROLLER				= 0x87;
				static const int INITIALIZE_IMAGE_ADDRESS		= 0x91;
				static const int WRITE_COMMAND					= 0xB1;


			private:
				int  GetContinuousImageSize( int dImageSize );
				int  SmallCamDLoad( int dBoardId, std::vector<int>& vData );
				void LoadPCIFile( const std::string sFilename, const bool& bAbort = false );
				void LoadGen23ControllerFile( const std::string sFilename, bool bValidate, const bool& bAbort = false );
				void SetByteSwapping();

				const std::string FormatPCICommand( int dCmd, int dReply, int dArg = -1, int dSysErr = -1 );
				const std::string FormatPCICommand( int dCmd, int dReply, int dArg[], int dArgCount, int dSysErr = -1 );

				CStringList* GetHSTRBitList( int dData, bool bDrawSeparator = false );

				//  Smart pointer array deleter
				// +--------------------------------------------------------------------+
				template<typename T> static void ArrayDeleter( T* p );

				static std::vector<ArcDev_t>		m_vDevList;
				static std::shared_ptr<std::string>	m_psDevList;
		};

	}	// end device namespace
}	// end arc namespace

#endif	// _CARC_PCI_H_
