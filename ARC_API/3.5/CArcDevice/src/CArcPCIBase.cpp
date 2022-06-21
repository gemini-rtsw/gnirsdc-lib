#include <iostream>
#include <sstream>
#include <cstdarg>
#include <iomanip>

#ifdef WIN32
#include <windows.h>
#else
#include <errno.h>
#endif

#include "CArcTools.h"
#include "CArcPCIBase.h"
#include "PCIRegs.h"


using namespace std;
using namespace arc;
using namespace arc::device;



// +---------------------------------------------------------------------------+
// | ArrayDeleter                                                              |
// +---------------------------------------------------------------------------+
// | Called by std::shared_ptr to delete the temporary image buffer.      |
// | This method should NEVER be called directly by the user.                  |
// +---------------------------------------------------------------------------+
template<typename T> void CArcPCIBase::ArrayDeleter( T* p )
{
	if ( p != NULL )
	{
		delete [] p;
	}
}


// +----------------------------------------------------------------------------
// |  Constructor
// +----------------------------------------------------------------------------
// |  See CArcPCIBase.h for the class definition
// +----------------------------------------------------------------------------
CArcPCIBase::CArcPCIBase()
{
}


// +----------------------------------------------------------------------------
// |  Destructor
// +----------------------------------------------------------------------------
CArcPCIBase::~CArcPCIBase()
{
	m_pCfgSpList.reset();
	m_pBarList.reset();
}


// +----------------------------------------------------------------------------
// |  GetCfgSp
// +----------------------------------------------------------------------------
// |  Reads and parses the entire PCI configuration space header into readable
// |  text and bit definitions that are stored in a member list variable. The
// |  public methods of this class allow access to this list. This method will
// |  create the member list if it doesn't already exist and clears it if it
// |  does.
// +----------------------------------------------------------------------------
void CArcPCIBase::GetCfgSp()
{
	m_pCfgSpList.reset( new PCIRegList() );

	if ( m_pCfgSpList.get() == NULL )
	{
			CArcTools::ThrowException(
								"CArcPCIBase",
								"GetCfgSp",
								"Failed to allocate CfgSp list pointer!" );
	}

	//  Get the standard PCI register values
	// +------------------------------------------------------------+
	int dRegAddr  = 0;
	int dRegValue = 0;

	dRegAddr  = CFG_VENDOR_ID;
	dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				dRegAddr,
                "Device ID / Vendor ID",
                dRegValue,
                GetDevVenBitList( dRegValue ) );

    dRegAddr  = CFG_COMMAND;
    dRegValue = GetCfgSpDWord( dRegAddr );
    CStringList* cRegBitList = GetCommandBitList( dRegValue, false );
    *cRegBitList += *GetStatusBitList( dRegValue, true );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "Status / Command",
                   dRegValue,
                   cRegBitList );

    dRegAddr  = CFG_REV_ID;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "Base Class / Sub Class / Interface / Revision ID",
                   dRegValue,
                   GetClassRevBitList( dRegValue ) );

    dRegAddr  = CFG_CACHE_SIZE;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "BIST / Header Type / Latency Timer / Cache Line Size",
                   dRegValue,
                   GetBistHeaderLatencyCache( dRegValue, true ) );

    dRegAddr  = CFG_BAR0;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "PCI Base Address 0",
                   dRegValue,
                   GetBaseAddressBitList( dRegValue, false ) );

    dRegAddr  = CFG_BAR1;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "PCI Base Address 1",
                   dRegValue,
                   GetBaseAddressBitList( dRegValue, false ) );

    dRegAddr  = CFG_BAR2;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "PCI Base Address 2",
                   dRegValue,
                   GetBaseAddressBitList( dRegValue, false ) );

    dRegAddr  = CFG_BAR3;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "PCI Base Address 3",
                   dRegValue,
                   GetBaseAddressBitList( dRegValue, false ) );

    dRegAddr  = CFG_BAR4;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "PCI Base Address 4",
                   dRegValue,
                   GetBaseAddressBitList( dRegValue, false ) );

    dRegAddr  = CFG_BAR5;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "PCI Base Address 5",
                   dRegValue,
                   GetBaseAddressBitList( dRegValue, false ) );

    dRegAddr  = CFG_CIS_PTR;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "Cardbus CIS Pointer",
                   dRegValue );

    dRegAddr  = CFG_SUB_VENDOR_ID;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "Subsystem Device ID / Subsystem Vendor ID",
                   dRegValue,
                   GetSubSysBitList( dRegValue ) );

    dRegAddr  = CFG_EXP_ROM_BASE;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "PCI Base Address-to-Local Expansion ROM",
                   dRegValue );

    dRegAddr  = CFG_CAP_PTR;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "Next Capability Pointer",
                   dRegValue );

    dRegAddr  = CFG_RESERVED1;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "Reserved",
                   dRegValue );

    dRegAddr  = CFG_INT_LINE;
    dRegValue = GetCfgSpDWord( dRegAddr );
	AddRegItem( m_pCfgSpList.get(),
				   dRegAddr,
                   "Max_Lat / Min_Grant / Interrupt Pin / Interrupt Line",
                   dRegValue,
                   GetMaxLatGntIntBitList( dRegValue ) );
}

// +----------------------------------------------------------------------------
// |  GetBarSp
// +----------------------------------------------------------------------------
// |  Reads and parses the entire PCI Base Address Registers (BAR) into readable
// |  text and bit definitions that are stored in a member list variable. The
// |  public methods of this class allow access to this list. This method will
// |  create the member list if it doesn't already exist and clears it if it
// |  does. NOTE: Not all BARS or PCI boards have data.
// +----------------------------------------------------------------------------
void CArcPCIBase::GetBarSp()
{
	m_pBarList.reset( new PCIBarList() );

	if ( m_pBarList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetBarSp",
								   "Failed to allocate BAR list pointer!" );
	}
}

// +----------------------------------------------------------------------------
// |  GetCfgSpCount
// +----------------------------------------------------------------------------
// |  Returns the number of elements in the configuration space member list.
// |
// |  Throws std::runtime_error if GetCfgSp hasn't been called first!
// +----------------------------------------------------------------------------
int CArcPCIBase::GetCfgSpCount()
{
	if ( m_pCfgSpList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetCfgSpCount",
								   "Empty register list, call GetCfgSp() first!" );
	}

	return static_cast<int>( m_pCfgSpList->size() );
}

// +----------------------------------------------------------------------------
// |  GetCfgSpAddr
// +----------------------------------------------------------------------------
// |  Returns the address for the specified configuration space member list
// |  register.
// |
// |  <IN> -> dIndex - Index into the cfg sp member list. Use GetCfgSpCount().
// |
// |  Throws std::runtime_error if GetCfgSp hasn't been called first!
// +----------------------------------------------------------------------------
int CArcPCIBase::GetCfgSpAddr( int dIndex )
{
	if ( m_pCfgSpList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetCfgSpAddr",
								   "Empty register list, call GetCfgSp() first!" );
	}

	return m_pCfgSpList->at( dIndex )->dAddr;
}

// +----------------------------------------------------------------------------
// |  GetCfgSpValue
// +----------------------------------------------------------------------------
// |  Returns the value for the specified configuration space member list
// |  register.
// |
// |  <IN> -> dIndex - Index into the cfg sp member list. Use GetCfgSpCount().
// |
// |  Throws std::runtime_error if GetCfgSp hasn't been called first!
// +----------------------------------------------------------------------------
int CArcPCIBase::GetCfgSpValue( int dIndex )
{
	if ( m_pCfgSpList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetCfgSpValue",
								   "Empty register list, call GetCfgSp() first!" );
	}

	return m_pCfgSpList->at( dIndex )->dValue;
}

// +----------------------------------------------------------------------------
// |  GetCfgSpName
// +----------------------------------------------------------------------------
// |  Returns the name for the specified configuration space member list
// |  register.
// |
// |  <IN> -> dIndex - Index into the cfg sp member list. Use GetCfgSpCount().
// |
// |  Throws std::runtime_error if GetCfgSp hasn't been called first!
// +----------------------------------------------------------------------------
string CArcPCIBase::GetCfgSpName( int dIndex )
{
	if ( m_pCfgSpList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetCfgSpName",
								   "Empty register list, call GetCfgSp() first!" );
	}

	return m_pCfgSpList->at( dIndex )->sName;
}

// +----------------------------------------------------------------------------
// |  GetCfgSpBitList
// +----------------------------------------------------------------------------
// |  Returns a pointer to the bit list for the specified configuration space
// |  member list register.
// |
// |  <IN> -> dIndex - Index into the cfg sp member list. Use GetCfgSpCount().
// |  <IN> -> pCount - Reference to local variable that will receive list count.
// |
// |  Throws std::runtime_error if GetCfgSp hasn't been called first!
// +----------------------------------------------------------------------------
string* CArcPCIBase::GetCfgSpBitList( int dIndex, int& pCount )
{
	if ( m_pCfgSpList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetCfgSpBitList",
								   "Empty register list, call GetCfgSp() first!" );
	}

	if ( dIndex >= int( m_pCfgSpList->size() ) )
	{
		CArcTools::ThrowException(
						"CArcPCIBase",
						"GetCfgSpBitList",
						"Configuration space index [ %d ] out of range [ 0 - %d ]",
						 dIndex, m_pCfgSpList->size() );
	}

	CStringList* pBitList = m_pCfgSpList->at( dIndex )->pBitList.get();

	if ( pBitList != NULL )
	{
		m_pTmpCfgBitList.reset( new string[ pBitList->Length() ],
								&CArcPCIBase::ArrayDeleter<string> );

		if ( m_pTmpCfgBitList.get() != NULL )
		{
			for ( int i=0; i<pBitList->Length(); i++ )
			{
				( m_pTmpCfgBitList.get() )[ i ].assign( pBitList->At( i ) );
			}

			pCount = pBitList->Length();
		}

		pCount = pBitList->Length();
	}
	else
	{
		pCount = 0;
	}

	return m_pTmpCfgBitList.get();
}

// +----------------------------------------------------------------------------
// |  GetBarCount
// +----------------------------------------------------------------------------
// |  Returns the number of elements in the base address register member list.
// |
// |  Throws std::runtime_error if GetBarSp hasn't been called first!
// +----------------------------------------------------------------------------
int CArcPCIBase::GetBarCount()
{
	if ( m_pBarList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetBarCount",
								   "Empty register list, call GetBarSp() first!" );
	}

	return static_cast<int>( m_pBarList->size() );
}

// +----------------------------------------------------------------------------
// |  GetBarRegCount
// +----------------------------------------------------------------------------
// |  Returns the number of register elements for the specified base address
// |  member list index.
// |
// |  <IN> -> dIndex - Index into the BAR member list. Use GetBarCount().
// |
// |  Throws std::runtime_error if GetBarSp hasn't been called first!
// +----------------------------------------------------------------------------
int CArcPCIBase::GetBarRegCount( int dIndex )
{
	if ( m_pBarList.get() == NULL || m_pBarList->at( dIndex )->pList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetBarRegCount",
								   "Empty register list, call GetBarSp() first!" );
	}

	return static_cast<int>( m_pBarList->at( dIndex )->pList->size() );
}

// +----------------------------------------------------------------------------
// |  GetBarRegAddr
// +----------------------------------------------------------------------------
// |  Returns the address of the specified base address member list register.
// |
// |  <IN> -> dIndex    - Index into the BAR member list. Use GetBarCount().
// |  <IN> -> dRegIndex - Index into the BAR member list register list. Use
// |                      GetBarRegCount().
// |
// |  Throws std::runtime_error if GetBarSp hasn't been called first!
// +----------------------------------------------------------------------------
int CArcPCIBase::GetBarRegAddr( int dIndex, int dRegIndex )
{
	if ( m_pBarList.get() == NULL || m_pBarList->at( dIndex )->pList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetBarRegAddr",
								   "Empty register list, call GetBarSp() first!" );
	}

	return m_pBarList->at( dIndex )->pList->at( dRegIndex )->dAddr;	
}

// +----------------------------------------------------------------------------
// |  GetBarRegValue
// +----------------------------------------------------------------------------
// |  Returns the value of the specified base address member list register.
// |
// |  <IN> -> dIndex    - Index into the BAR member list. Use GetBarCount().
// |  <IN> -> dRegIndex - Index into the BAR member list register list. Use
// |                      GetBarRegCount().
// |
// |  Throws std::runtime_error if GetBarSp hasn't been called first!
// +----------------------------------------------------------------------------
int CArcPCIBase::GetBarRegValue( int dIndex, int dRegIndex )
{
	if ( m_pBarList.get() == NULL || m_pBarList->at( dIndex )->pList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetBarRegValue",
								   "Empty register list, call GetBarSp() first!" );
	}

	return m_pBarList->at( dIndex )->pList->at( dRegIndex )->dValue;	
}

// +----------------------------------------------------------------------------
// |  GetBarName
// +----------------------------------------------------------------------------
// |  Returns the name of the specified base address.
// |
// |  <IN> -> dIndex - Index into the BAR member list. Use GetBarCount().
// |
// |  Throws std::runtime_error if GetBarSp hasn't been called first!
// +----------------------------------------------------------------------------
string CArcPCIBase::GetBarName( int dIndex )
{
	if ( m_pBarList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetBarName",
								   "Empty register list, call GetBarSp() first!" );
	}

	return m_pBarList->at( dIndex )->sName;
}

// +----------------------------------------------------------------------------
// |  GetBarRegName
// +----------------------------------------------------------------------------
// |  Returns the name of the specified base address member list register.
// |
// |  <IN> -> dIndex    - Index into the BAR member list. Use GetBarCount().
// |  <IN> -> dRegIndex - Index into the BAR member list register list. Use
// |                      GetBarRegCount().
// |
// |  Throws std::runtime_error if GetBarSp hasn't been called first!
// +----------------------------------------------------------------------------
string CArcPCIBase::GetBarRegName( int dIndex, int dRegIndex )
{
	if ( m_pBarList.get() == NULL || m_pBarList->at( dIndex )->pList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetBarRegName",
								   "Empty register list, call GetBarSp() first!" );
	}

	return m_pBarList->at( dIndex )->pList->at( dRegIndex )->sName;
}

// +----------------------------------------------------------------------------
// |  GetBarRegBitListCount
// +----------------------------------------------------------------------------
// |  Returns the number of elements of the specified base address member list
// |  register bit list.
// |
// |  <IN> -> dIndex    - Index into the BAR member list. Use GetBarCount().
// |  <IN> -> dRegIndex - Index into the BAR member list register list. Use
// |                      GetBarRegCount().
// |
// |  Throws std::runtime_error if GetBarSp hasn't been called first!
// +----------------------------------------------------------------------------
int CArcPCIBase::GetBarRegBitListCount( int dIndex, int dRegIndex )
{
	if ( m_pBarList.get() == NULL || m_pBarList->at( dIndex )->pList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetBarRegBitListCount",
								   "Empty register list, call GetBarSp() first!" );
	}

	if ( m_pBarList->at( dIndex )->pList->at( dRegIndex )->pBitList.get() == NULL )
	{
		return 0;
	}

	return m_pBarList->at( dIndex )->pList->at( dRegIndex )->pBitList->Length();
}

// +----------------------------------------------------------------------------
// |  GetBarRegBitListDef
// +----------------------------------------------------------------------------
// |  Returns the bit definition string of the specified base address member
// |  list register bit list index.
// |
// |  <IN> -> dIndex        - Index into the BAR member list. Use GetBarCount().
// |  <IN> -> dRegIndex     - Index into the BAR member list register list. Use
// |                          GetBarRegCount().
// |  <IN> -> dBitListIndex - Index into the BAR member list register list bit
// |                          list. Use GetBarRegBitListCount().
// |
// |  Throws std::runtime_error if GetBarSp hasn't been called first!
// +----------------------------------------------------------------------------
string CArcPCIBase::GetBarRegBitListDef( int dIndex, int dRegIndex, int dBitListIndex )
{
	if ( m_pBarList.get() == NULL || m_pBarList->at( dIndex )->pList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetBarRegBitListDef",
								   "Empty register list, call GetBarSp() first!" );
	}

	if ( m_pBarList->at( dIndex )->pList->at( dRegIndex )->pBitList.get() == NULL )
	{
		return string( "" );
	}

	return m_pBarList->at( dIndex )->pList->at( dRegIndex )->pBitList->At( dBitListIndex );
}

// +----------------------------------------------------------------------------
// |  GetBarRegBitList
// +----------------------------------------------------------------------------
// |  Returns a pointer to the bit list for the specified base address member
// |  list register.
// |
// |  <IN> -> dIndex    - Index into the BAR member list. Use GetBarCount().
// |  <IN> -> dRegIndex - Index into the BAR member list register list. Use
// |                      GetBarRegCount().
// |  <IN> -> pCount    - Reference to local variable that will receive list
// |                      count.
// |
// |  Throws std::runtime_error if GetBarSp hasn't been called first!
// +----------------------------------------------------------------------------
string* CArcPCIBase::GetBarRegBitList( int dIndex, int dRegIndex, int& pCount )
{
	if ( m_pBarList.get() == NULL )
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "GetBarRegBitList",
								   "Empty register list, call GetBarSp() first!" );
	}

	if ( dIndex >= int( m_pBarList->size() ) )
	{
		CArcTools::ThrowException(
				"CArcPCIBase",
				"GetBarRegBitList",
				"Base address register index [ %d ] out of range [ 0 - %d ]",
				 dIndex, m_pBarList->size() );
	}

	CStringList* pBitList =
					m_pBarList->at( dIndex )->pList->at( dRegIndex )->pBitList.get();

	if ( pBitList != NULL )
	{
		m_pTmpBarBitList.reset( new string[ pBitList->Length() ],
								&CArcPCIBase::ArrayDeleter<string> );

		if ( m_pTmpBarBitList.get() != NULL )
		{
			for ( int i=0; i<pBitList->Length(); i++ )
			{
				( m_pTmpBarBitList.get() )[ i ].assign( pBitList->At( i ) );
			}

			pCount = pBitList->Length();
		}
		else
		{
			pCount = 0;
		}
	}

	return m_pTmpBarBitList.get();
}

// +----------------------------------------------------------------------------
// |  PrintCfgSp
// +----------------------------------------------------------------------------
// |  Prints the member configuration list to std out.
// |
// |  Throws std::runtime_error if GetCfgSp hasn't been called first!
// +----------------------------------------------------------------------------
void CArcPCIBase::PrintCfgSp()
{
	cout << endl;
	cout << "_________________________Configuration Space_________________________"
		 << endl << endl;

	for ( int i=0; i<GetCfgSpCount(); i++ )
	{
		cout << "\tAddr: 0x" << hex << GetCfgSpAddr( i ) << dec << endl
			<< "\tValue: 0x" << hex << GetCfgSpValue( i ) << dec << endl
			<< "\tName: " << GetCfgSpName( i ) << endl;

		int dCount = 0;
		std::string* psBitList = GetCfgSpBitList( i, dCount );

		for ( int j=0; j<dCount; j++ )
		{
			cout << "\tBit List[ " << j << " ]: "
				 << psBitList[ j ] << endl;
		}

		cout << endl;
	}

	cout << endl;
}

// +----------------------------------------------------------------------------
// |  PrintBars
// +----------------------------------------------------------------------------
// |  Prints the member base address list to std out.
// |
// |  Throws std::runtime_error if GetBarSp hasn't been called first!
// +----------------------------------------------------------------------------
void CArcPCIBase::PrintBars()
{
	cout << endl;
	cout << "_______________________Configuration Space BARS_______________________"
		 << endl << endl;

	for ( int i=0; i<GetBarCount(); i++ )
	{
		cout << endl
			 << "___________________" << GetBarName( i ) << "___________________"
			 << endl << endl;

		for ( int j=0; j<GetBarRegCount( i ); j++ )
		{
			cout << "\tReg Addr:  0x" << hex
				 << GetBarRegAddr( i, j ) << dec << endl
				 << "\tReg Value: 0x" << hex
				 << GetBarRegValue( i, j ) << dec << endl
				 << "\tReg Name: "
				 << GetBarRegName( i, j ) << endl;

			for ( int k=0; k<GetBarRegBitListCount( i, j ); k++ )
			{
				cout << "\tBit List: "
					 << GetBarRegBitListDef( i, j, k )
					 << endl;
			}

			cout << endl;
		}
	}

	cout << endl;
}

// +----------------------------------------------------------------------------
// |  AddRegItem
// +----------------------------------------------------------------------------
// |  Adds the specified parameters to the specified register data list.
// |  Convenience method.
// |
// |  <IN> -> pDataList - PCI register list that will contain the remaining
// |                      parameters.
// |  <IN> -> dAddr     - Register address
// |  <IN> -> szName    - Register name
// |  <IN> -> dValue    - Register value
// |  <IN> -> pBitList  - String list of register bit definitions.
// |
// |  Throws std::runtime_error if data list pointer is invalid.
// +----------------------------------------------------------------------------
void CArcPCIBase::AddRegItem( PCIRegList* pvDataList, int dAddr, const char* szName,
							  int dValue, CStringList* pBitList )
{
	if ( pvDataList != NULL )
	{
		shared_ptr<PCIRegData> pConfigData( new PCIRegData );

		if ( pConfigData.get() == NULL )
		{
			CArcTools::ThrowException( "CArcPCIBase",
									   "AddRegItem",
									   "Failed to allocate PCIRegData structure!" );
		}

		pConfigData->dAddr     = dAddr;
		pConfigData->sName     = szName;
		pConfigData->dValue    = dValue;

		pConfigData->pBitList.reset( pBitList );

		pvDataList->push_back( pConfigData );
	}
	else
	{
		CArcTools::ThrowException( "CArcPCIBase",
								   "AddRegItem",
								   "Invalid config data list pointer ( NULL )!" );
	}
}


// +----------------------------------------------------------------------------
// |  AddBarItem
// +----------------------------------------------------------------------------
// |  Adds an element with the specified parameters to the base address
// |  register list.
// |
// |  <IN> -> szName  - BAR name
// |  <IN> -> pList   - BAR register list
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
void CArcPCIBase::AddBarItem( std::string sName, PCIRegList* pList )
{
	if ( m_pBarList.get() != NULL )
	{
		shared_ptr<PCIBarData> pLocalItem( new PCIBarData );

		pLocalItem->sName = sName;

		pLocalItem->pList.reset( pList );

		m_pBarList->push_back( pLocalItem );
	}
	else
	{
		CArcTools::ThrowException( "CArcPCIBase", "AddBarItem",
								   "Invalid config data list pointer ( NULL )!" );
	}
}


// +----------------------------------------------------------------------------
// |  GetDevVenBitList
// +----------------------------------------------------------------------------
// |  Sets the bit list strings for the DEVICE ID and VENDOR ID ( 0x0 )PCI Cfg
// |  Sp register.
// |
// |  <IN> -> dData  - The PCI cfg sp DEVICE and VENDOR ID register value.
// |  <IN> -> bDrawSeparator - 'true' to include a line separator within the
// |                            bit list strings.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
CStringList* CArcPCIBase::GetDevVenBitList( int dData, bool bDrawSeparator )
{
	CStringList* pBitList = new CStringList;

	if ( !pBitList->Empty() )
	{
		pBitList->Clear();
	}

    if ( bDrawSeparator )
	{
		*pBitList << "____________________________________________________";
	}

	*pBitList << CArcTools::FormatString( "Device ID: 0x%X", PCI_GET_DEV( dData ) )
			  << CArcTools::FormatString( "Vendor ID: 0x%X", PCI_GET_VEN( dData ) );

	return pBitList;
}

// +----------------------------------------------------------------------------
// |  GetCommandBitList
// +----------------------------------------------------------------------------
// |  Sets the bit list strings for the COMMAND ( 0x4 ) PCI Cfg Sp register.
// |
// |  <IN> -> dData  - The PCI cfg sp COMMAND register value.
// |  <IN> -> bDrawSeparator - 'true' to include a line separator within the
// |                            bit list strings.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
CStringList* CArcPCIBase::GetCommandBitList( int dData, bool bDrawSeparator )
{
    CStringList* pBitList = new CStringList;

    if ( bDrawSeparator )
	{
        *pBitList << "____________________________________________________";
	}

    *pBitList << CArcTools::FormatString( "PCI COMMAND BIT DEFINITIONS ( 0x%X )",
										   PCI_GET_CMD( dData ) )

              << CArcTools::FormatString( "Bit  0 : I/O Access Enable : %d",
										   PCI_GET_CMD_IO_ACCESS_ENABLED( dData ) )

              << CArcTools::FormatString( "Bit  1 : Memory Space Enable : %d",
										   PCI_GET_CMD_MEMORY_ACCESS_ENABLED( dData ) )

              << CArcTools::FormatString( "Bit  2 : Bus Master Enable : %d",
										   PCI_GET_CMD_ENABLE_MASTERING( dData ) )

              << CArcTools::FormatString( "Bit  3 : Special Cycle Enable : %d",
										   PCI_GET_CMD_SPECIAL_CYCLE_MONITORING( dData ) )

              << CArcTools::FormatString( "Bit  4 : Memory Write and Invalidate : %d",
										   PCI_GET_CMD_MEM_WRITE_INVAL_ENABLE( dData ) )

			  << CArcTools::FormatString( "Bit  5 : VGA Palette Snoop : %d",
										   PCI_GET_CMD_PALETTE_SNOOP_ENABLE( dData ) )

              << CArcTools::FormatString( "Bit  6 : Parity Error Response Enable : %d",
										   PCI_GET_CMD_PARITY_ERROR_RESPONSE( dData ) )

              << CArcTools::FormatString( "Bit  7 : Address Stepping Enable : %d",
										   PCI_GET_CMD_WAIT_CYCLE_CONTROL( dData ) )

              << CArcTools::FormatString( "Bit  8 : Internal SERR# Enable : %d",
										   PCI_GET_CMD_SERR_ENABLE( dData ) )

              << CArcTools::FormatString( "Bit  9 : Fast Back-to-Back Enable : %d",
										   PCI_GET_CMD_FAST_BACK2BACK_ENABLE( dData ) )

              << string( "Bit 10-15 : Reserved" );

    return pBitList;
}

// +----------------------------------------------------------------------------
// |  GetStatusBitList
// +----------------------------------------------------------------------------
// |  Sets the bit list strings for the STATUS ( 0x6 ) PCI Cfg Sp register.
// |
// |  <IN> -> dData  - The PCI cfg sp STATUS register value.
// |  <IN> -> bDrawSeparator - 'true' to include a line separator within the
// |                            bit list strings.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
CStringList* CArcPCIBase::GetStatusBitList( int dData, bool bDrawSeparator )
{
    CStringList* pBitList = new CStringList;

    if ( bDrawSeparator )
	{
        *pBitList << "____________________________________________________";
	}

    *pBitList << CArcTools::FormatString( "PCI STATUS BIT DEFINITIONS ( 0x%X )",
										   PCI_GET_STATUS( dData ) )

              << string( "Bit 0-4 : Reserved" )

              << CArcTools::FormatString( "Bit 5 : 66-MHz Capable (Internal Clock Frequency) : %d",
										   PCI_GET_STATUS_66MHZ_CAPABLE( dData ) )

              << string( "Bit 6 : Reserved" )

              << CArcTools::FormatString( "Bit 7 : Fast Back-to-Back Transactions Capable : %d",
                                           PCI_GET_STATUS_FAST_BACK2BACK_CAPABLE( dData ) )

              << CArcTools::FormatString( "Bit 8 : Master Data Parity Error : %d",
                                           PCI_GET_STATUS_DATA_PARITY_REPORTED( dData ) )

              << CArcTools::FormatString( "Bit 9-10 : DEVSEL Timing : %d [ %s ]",
                                           PCI_GET_STATUS_DEVSEL_TIMING( dData ),
                                           PCI_GET_STATUS_GET_DEVSEL_STRING( dData ) )

              << CArcTools::FormatString( "Bit 11 : Signaled Target Abort : %d",
                                           PCI_GET_STATUS_SIGNALLED_TARGET_ABORT( dData ) )

              << CArcTools::FormatString( "Bit 12 : Received Target Abort : %d",
                                           PCI_GET_STATUS_RECEIVED_TARGET_ABORT( dData ) )

              << CArcTools::FormatString( "Bit 13 : Received Master Abort : %d",
                                           PCI_GET_STATUS_RECEIVED_MASTER_ABORT( dData ) )

              << CArcTools::FormatString( "Bit 14 : Signaled System Error : %d",
                                           PCI_GET_STATUS_SIGNALLED_SYSTEM_ERROR( dData ) )

              << CArcTools::FormatString( "Bit 15 : Detected Parity Error : %d",
                                           PCI_GET_STATUS_DETECTED_PARITY_ERROR( dData ) );

    return pBitList;
}

// +----------------------------------------------------------------------------
// |  GetClassRevBitList
// +----------------------------------------------------------------------------
// |  Sets the bit list strings for the CLASS CODE ( 0x8 ) and REVISION ID
// |  ( 0x9 ) PCI Cfg Sp register.
// |
// |  <IN> -> dData  - The PCI cfg sp CLASS CODE and REV ID register value.
// |  <IN> -> bDrawSeparator - 'true' to include a line separator within the
// |                            bit list strings.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
CStringList* CArcPCIBase::GetClassRevBitList( int dData, bool bDrawSeparator )
{
    CStringList* pBitList = new CStringList;

    if ( bDrawSeparator )
	{
        *pBitList << "____________________________________________________";
	}

    *pBitList << CArcTools::FormatString( "Base Class Code: 0x%X [ %s ]",
                                           PCI_GET_BASECLASS( dData ),
                                           PCI_GET_GET_BASECLASS_STRING( dData ) )

              << CArcTools::FormatString( "Sub Class Code: 0x%X", PCI_GET_SUBCLASS( dData ) )
              << CArcTools::FormatString( "Interface: 0x%X", PCI_GET_INTERFACE( dData ) )
              << CArcTools::FormatString( "Revision ID: 0x%X", PCI_GET_REVID( dData ) );

    return pBitList;
}

// +----------------------------------------------------------------------------
// |  GetBistHeaderLatencyCache
// +----------------------------------------------------------------------------
// |  Sets the bit list strings for the BIST ( 0xF ), HEADER TYPE ( 0xE ),
// |  LATENCY TIMER ( 0xD ) and CACHE LINE SIZE ( 0xC ) PCI Cfg Sp registers.
// |
// |  <IN> -> dData  - The PCI cfg sp BIST, HEADER TYPE, LATENCY TIMER and
// |                   CACHE LINE SIZE register value.
// |  <IN> -> bDrawSeparator - 'true' to include a line separator within the
// |                            bit list strings.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
CStringList* CArcPCIBase::GetBistHeaderLatencyCache( int dData, bool bDrawSeparator )
{
    CStringList* pBitList = new CStringList;

    *pBitList << CArcTools::FormatString( "BIST BIT DEFINITIONS ( 0x%X )",
                                           PCI_GET_BIST( dData ) )

              << CArcTools::FormatString( "Bit 0-3 : BIST Completion Code : 0x%X",
                                           PCI_GET_BIST_COMPLETE_CODE( dData ) )

              << string( "Bit 4-5 : Reserved" )

              << CArcTools::FormatString( "Bit 6 : BIST Invoked : %d",
                                           PCI_GET_BIST_INVOKED( dData ) )

              << CArcTools::FormatString( "Bit 7 : Device BIST Capable : %d",
                                           PCI_GET_BIST_CAPABLE( dData ) );

    if ( bDrawSeparator )
	{
        *pBitList << "____________________________________________________";
	}

    *pBitList << CArcTools::FormatString( "Header Type: 0x%X", PCI_GET_HEADER_TYPE( dData ) )
              << CArcTools::FormatString( "Latency Timer: 0x%X", PCI_GET_LATENCY_TIMER( dData ) )
              << CArcTools::FormatString( "Cache Line Size: 0x%X", PCI_GET_CACHE_LINE_SIZE( dData ) );

    return pBitList;
}

// +----------------------------------------------------------------------------
// |  GetBaseAddressBitList
// +----------------------------------------------------------------------------
// |  Sets the bit list strings for the BASE ADDRESS REGISTERS ( 0x10 to 0x24 )
// |  PCI Cfg Sp registers.
// |
// |  <IN> -> dData  - The PCI cfg sp BASE ADDRESS REGISTERS register value.
// |  <IN> -> bDrawSeparator - 'true' to include a line separator within the
// |                            bit list strings.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
CStringList* CArcPCIBase::GetBaseAddressBitList( int dData, bool bDrawSeparator )
{
    CStringList* pBitList = new CStringList;

    if ( bDrawSeparator )
	{
        *pBitList << "____________________________________________________";
	}

    *pBitList << CArcTools::FormatString( "BASE ADDRESS BIT DEFINITIONS ( 0x%X )", dData );

    if ( PCI_GET_BASE_ADDR_TYPE( dData ) == 0 )
    {
        *pBitList << CArcTools::FormatString( "Bit 0 : Memory Space Indicator : %d [ Memory Space ]",
                                               PCI_GET_BASE_ADDR_TYPE( dData ) )

				  << CArcTools::FormatString( "Bit 1-2 : Type: %d [ %s ]",
								               PCI_GET_BASE_ADDR_MEM_TYPE( dData ),
								               PCI_GET_BASE_ADDR_MEM_TYPE_STRING( dData ) )

				  << CArcTools::FormatString( "Bit 3 : Prefetchable : %d",
								               PCI_GET_BASE_ADDR_MEM_PREFETCHABLE( dData ) )

				  << CArcTools::FormatString( "Bit 4-31 : Base Address : 0x%X",
								               PCI_GET_BASE_ADDR( dData ) );
    }
    else
    {
        *pBitList << CArcTools::FormatString( "Bit 0 : Memory Space Indicator : %d [ I/O Space ]",
                                               PCI_GET_BASE_ADDR_TYPE( dData ) )

                  << string( "Bit 1 : Reserved" )

                  << CArcTools::FormatString( "Bit 2-31 : Base Address : 0x%X",
                                               PCI_GET_BASE_ADDR( dData ) );
    }

    return pBitList;
}

// +----------------------------------------------------------------------------
// |  GetSubSysBitList
// +----------------------------------------------------------------------------
// |  Sets the bit list strings for the SUBSYSTEM ID's ( 0x2C ) PCI Cfg Sp
// |  register.
// |
// |  <IN> -> dData  - The PCI cfg sp SUBSYSTEM ID's register value.
// |  <IN> -> bDrawSeparator - 'true' to include a line separator within the
// |                            bit list strings.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
CStringList* CArcPCIBase::GetSubSysBitList( int dData, bool bDrawSeparator )
{
    CStringList* pBitList = new CStringList;

    if ( bDrawSeparator )
	{
        *pBitList << "____________________________________________________";
	}

    *pBitList << CArcTools::FormatString( "Subsystem ID: 0x%X", PCI_GET_DEV( dData ) )
              << CArcTools::FormatString( "Subsystem Vendor ID: 0x%X", PCI_GET_VEN( dData ) );

    return pBitList;
}

// +----------------------------------------------------------------------------
// |  GetMaxLatGntIntBitList
// +----------------------------------------------------------------------------
// |  Sets the bit list strings for the MAX LATENCY ( 0x3F ), MIN GNT ( 0x3E ),
// |  INTERRUPT PIN ( 0x3D ) and INTERRUPT LINE ( 0x3C ) PCI Cfg Sp register.
// |
// |  <IN> -> dData  - The PCI cfg sp BASE ADDRESS REGISTERS register value.
// |  <IN> -> bDrawSeparator - 'true' to include a line separator within the
// |                            bit list strings.
// |
// |  Throws std::runtime_error on error
// +----------------------------------------------------------------------------
CStringList* CArcPCIBase::GetMaxLatGntIntBitList( int dData, bool bDrawSeparator )
{
    CStringList* pBitList = new CStringList;

    if ( bDrawSeparator )
	{
        *pBitList << "____________________________________________________";
	}

    *pBitList << CArcTools::FormatString( "Max_Lat: 0x%X", PCI_GET_MAX_LAT( dData ) )
              << CArcTools::FormatString( "Min_Grant: 0x%X", PCI_GET_MIN_GRANT( dData ) )
              << CArcTools::FormatString( "Interrupt Pin: 0x%X", PCI_GET_INTR_PIN( dData ) )

              << CArcTools::FormatString( "Interrupt Line: 0x%X [ %d ]",
                                           PCI_GET_INTR_LINE( dData ),
                                           PCI_GET_INTR_LINE( dData ) );

    return pBitList;
}
