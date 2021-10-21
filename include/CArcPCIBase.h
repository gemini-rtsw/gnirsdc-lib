#ifndef _CARC_PCIBASE_H_
#define _CARC_PCIBASE_H_

#ifdef WIN32
#pragma warning( disable: 4251 )
#endif

#include <vector>
#include <string>
#include <memory>
#include "CArcDevice.h"
#include "CStringList.h"
#include "DllMain.h"



namespace arc
{
	namespace device
	{

		// +-------------------------------------------------------------------+
		// |  PCI configuration space register data value                      |
		// +-------------------------------------------------------------------+
		typedef std::auto_ptr<CStringList> CStrListPtr;

		typedef struct PCI_REG_DATA
		{
			CStrListPtr		pBitList;
			std::string		sName;
			int				dValue;
			int				dAddr;
		} PCIRegData;

		typedef std::shared_ptr<PCIRegData> PCIRegDataPtr;
		typedef std::vector<PCIRegDataPtr> PCIRegList;


		// +-------------------------------------------------------------------+
		// |  PCI configuration space register bar item ( used to name and     |
		// |  contain a set of local device configuration registers; if they   |
		// |  exist )                                                          |
		// +-------------------------------------------------------------------+
		typedef std::shared_ptr<PCIRegList> PCIRegListPtr;

		typedef struct PCI_BAR_DATA
		{
			std::string		sName;
			PCIRegListPtr	pList;
		} PCIBarData;

		typedef std::shared_ptr<PCIBarData> PCIBarDataPtr;
		typedef std::vector<PCIBarDataPtr> PCIBarList;


		// +-------------------------------------------------------------------+
		// |  PCI Base Class exported in CArcDevice.dll                        |
		// +-------------------------------------------------------------------+
		class CARCDEVICE_API CArcPCIBase : public CArcDevice
		{
			public:
				CArcPCIBase();
				virtual ~CArcPCIBase();

				virtual int  GetCfgSpByte( int dOffset )  = 0;
				virtual int  GetCfgSpWord( int dOffset )  = 0;
				virtual int  GetCfgSpDWord( int dOffset ) = 0;

				virtual void SetCfgSpByte( int dOffset, int dValue )  = 0;
				virtual void SetCfgSpWord( int dOffset, int dValue )  = 0;
				virtual void SetCfgSpDWord( int dOffset, int dValue ) = 0;

				virtual void GetCfgSp();
				virtual void GetBarSp();

				int          GetCfgSpCount();
				int          GetCfgSpAddr( int dIndex );
				int          GetCfgSpValue( int dIndex );
				std::string  GetCfgSpName( int dIndex );
				std::string* GetCfgSpBitList( int dIndex, int& pCount );

				int          GetBarCount();
				std::string  GetBarName( int dIndex );

				int          GetBarRegCount( int dIndex );
				int          GetBarRegAddr( int dIndex, int dRegIndex );
				int          GetBarRegValue( int dIndex, int dRegIndex );
				std::string  GetBarRegName( int dIndex, int dRegIndex );

				int          GetBarRegBitListCount( int dIndex, int dRegIndex );
				std::string  GetBarRegBitListDef( int dIndex, int dRegIndex, int dBitListIndex );

				std::string* GetBarRegBitList( int dIndex, int dRegIndex, int& pCount );

				void PrintCfgSp();
				void PrintBars();

			protected:
				void AddRegItem( PCIRegList* pvDataList, int dAddr, const char* szName, int dValue, CStringList* pBitList = NULL );

				void AddBarItem( std::string qsName, PCIRegList* pList );

				CStringList* GetDevVenBitList( int dData, bool bDrawSeparator = false );
				CStringList* GetCommandBitList( int dData, bool bDrawSeparator = false );
				CStringList* GetStatusBitList( int dData, bool bDrawSeparator = false );
				CStringList* GetClassRevBitList( int dData, bool bDrawSeparator = false );
				CStringList* GetBistHeaderLatencyCache( int dData, bool bDrawSeparator = false );
				CStringList* GetBaseAddressBitList( int dData, bool bDrawSeparator = false );
				CStringList* GetSubSysBitList( int dData, bool bDrawSeparator = false );
				CStringList* GetMaxLatGntIntBitList( int dData, bool bDrawSeparator = false );

				//  Smart pointer array deleter
				// +--------------------------------------------------------------------+
				template<typename T> static void ArrayDeleter( T* p );

				std::shared_ptr<PCIRegList> m_pCfgSpList;
				std::shared_ptr<PCIBarList> m_pBarList;

				std::shared_ptr<std::string> m_pTmpCfgBitList;
				std::shared_ptr<std::string> m_pTmpBarBitList;
		};

	}	// end device namespace
}	// end arc namespace


#endif		// _CARC_PCIBASE_H_
