// CArcDeinterlace.cpp : Defines the exported functions for the DLL application.
//

#ifdef WIN32
	#include <windows.h>
#else
	#include <dlfcn.h>
	#include <dirent.h>
#endif

#include <memory>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include "CArcDeinterlace.h"

using namespace std;
using namespace arc::deinterlace;


// +----------------------------------------------------------------------------
// | OS dependent macros
// +----------------------------------------------------------------------------
#ifdef WIN32
	#define ArcLoadLibrary( path, lib )			LoadLibraryA( lib.c_str() )
	#define ArcFindLibrarySymbol( lib, name )	GetProcAddress( lib, name )
	#define ArcZeroMemory( buf, size )			ZeroMemory( buf, size )
	#define ArcCopyMemory( dst, src, size )		CopyMemory( dst, src, size )
	#define ArcFreeLibrary( handle )			FreeLibrary( handle )
	#define ArcSysMsg( msg )					string( msg )
#else
	#define ArcLoadLibrary( path, lib )			dlopen( string( path + "/" + lib ).c_str(), RTLD_LAZY )
	#define ArcFindLibrarySymbol( lib, name )	dlsym( lib, name )
	#define ArcZeroMemory( buf, size )			memset( buf, 0, size )
	#define ArcCopyMemory( dst, src, size )		memcpy( dst, src, size )
	#define ArcFreeLibrary( handle )			dlclose( handle )
	#define ArcSysMsg( msg )					string( msg ) + string( " - " ) + dlerror()
#endif


// +----------------------------------------------------------------------------
// | Default constructor
// +----------------------------------------------------------------------------
CArcDeinterlace::CArcDeinterlace()
{
	m_hCustomLib		= NULL;
	m_fnRunCustomAlg	= NULL;
	m_fnGetCustomName	= NULL;
	m_fnGetCustomCount	= NULL;
	m_fnGetErrorMsg		= NULL;
	m_dCustomCount		= 0;
}

// +----------------------------------------------------------------------------
// |  Class Destructor
// +----------------------------------------------------------------------------
CArcDeinterlace::~CArcDeinterlace()
{
	if ( m_hCustomLib != NULL )
	{
		ArcFreeLibrary( m_hCustomLib );
	}

#ifdef WIN32
	SetDllDirectoryA( NULL );
#endif

	m_pNewData.reset();
}

// +----------------------------------------------------------------------------
// | Deleter
// +----------------------------------------------------------------------------
// | Called by std::shared_ptr to delete the temporary image buffer.  This
// | method should NEVER be called directly by the user.
// +----------------------------------------------------------------------------
template<typename T> void CArcDeinterlace::ArrayDeleter( T* p )
{
	if ( p != NULL )
	{
		delete [] p;
	}
}

// +---------------------------------------------------------------------------------------
// | RunAlg - Deinterlace Image
// +---------------------------------------------------------------------------------------
// |
// | PARAMETERS:	mem_fd   File descriptor that references the image.
// |                dRows	 The number of rows in the image.
// |                dCols	 The number of columns in the image.
// | 
// | DESCRIPTION:	The deinterlacing algorithms work on the principle that the
// | ccd/ir array will read out the data in a predetermined order depending on
// | the type of readout being implemented.  Here's how they look:     			
// | 
// | split-parallel          split-serial              quad CCD                quad IR          	
// | ----------------       ----------------        ----------------        ----------------     	
// | |     1  ------->|     |        |       |      |<-----  |  ---->|      | -----> | ----> |    	
// | |                |     |        |       |      |   3    |   2   |      |   0    |   1   |    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |_______________ |     |        |       |      |________|_______|      |________|_______|    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |                |     |        |       |      |        |       |      |        |       |    	
// | |                |     |   0    |   1   |      |   0    |   1   |      |   3    |   2   |    	
// | |<--------  0    |     |<------ |------>|      |<-----  |  ---->|      | -----> | ----> |    	
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
// +-----------------------------------------------------------------------------------------
void CArcDeinterlace::RunAlg( void* pData, int dRows, int dCols, int dAlgorithm, int dArg )
{
	unsigned short *pOldData = NULL;	// Old image buffer pointer

	// NOTE ****** Instead of memcpy pOldData to m_pNewData, maybe just return the
	// buffer pointer.

	// Form a pointer to the current image buffer
	// -------------------------------------------------------------------
	pOldData = ( unsigned short * )pData;

	// Allocate a new buffer to hold the deinterlaced image
	// -------------------------------------------------------------------
	m_pNewData.reset( new unsigned short[ dCols * dRows ],
					  &CArcDeinterlace::ArrayDeleter<unsigned short> );

	if ( m_pNewData.get() == NULL || pOldData == NULL )
	{
		ThrowException( "RunAlg",
				"Error in allocating temporary image buffer for deinterlacing." );
	}

	switch( dAlgorithm )
	{
		// +-------------------------------------------------------------------+
		// |                   NO DEINTERLACING                                |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_NONE:
		{
			// Do nothing
		}
		break;

		// +-------------------------------------------------------------------+
		// |                       PARALLEL READOUT                            |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_PARALLEL:
		{
			Parallel( pOldData, dRows, dCols );
		}
		break;

		// +-------------------------------------------------------------------+
		// |                        SERIAL READOUT                             |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_SERIAL:
		{
			Serial( pOldData, dRows, dCols );
		}
		break;

		// +-------------------------------------------------------------------+
		// |                         QUAD READOUT                              |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_CCD_QUAD:
		{
			CCDQuad( pOldData, dRows, dCols );
		}
		break;

		// +-------------------------------------------------------------------+
		// |                         IR QUAD READOUT                           |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_IR_QUAD:
		{
			IRQuad( pOldData, dRows, dCols );
		}
		break;

		// +-------------------------------------------------------------------+
		// |            CORRELATED DOUBLE SAMPLING IR QUAD READOUT             |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_CDS_IR_QUAD:
		{
			IRQuadCDS( pOldData, dRows, dCols );
		}
		break;

		// +-------------------------------------------------------------------+
		// |         HawaiiRG READOUT                                          |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_HAWAII_RG:
		{
			HawaiiRG( pOldData, dRows, dCols, dArg );
		}
		break;

		// +-------------------------------------------------------------------+
		// |         STA1600 READOUT                                           |
		// +-------------------------------------------------------------------+
		case DEINTERLACE_STA1600:
		{
			STA1600( pOldData, dRows, dCols );
		}
		break;

		// +-------------------------------------------------------------------+
		// |         Default                                                   |
		// +-------------------------------------------------------------------+
		default:
		{
			// +---------------------------------------------------------------+
			// |         Custom Algorithm Check                                |
			// +---------------------------------------------------------------+
			if ( m_dCustomCount > 0 && dAlgorithm >= DEINTERLACE_CUSTOM )
			{
				RunCustomAlg( pData, dRows, dCols, dAlgorithm, dArg );
			}
			else
			{
				ostringstream oss;

				oss << "Invalid deinterlace algorithm ("
					<< dAlgorithm << ")!" << ends;

				ThrowException( "RunAlg", oss.str() );
			}
		}
		break;
	}	// End switch
}

// +-------------------------------------------------------------------+
// | FindCustomLibrary                                                 |
// +-------------------------------------------------------------------+
// | Searches the specified path for a custom deinterlace library.     |
// | Returns 'true' if a library was found, 'false' otherwise.         |
// |                                                                   |
// | Throws a std::runtime_error                                       |
// |                                                                   |
// | <IN> -> sLibPath : The path to search for libraries.              |
// +-------------------------------------------------------------------+
bool CArcDeinterlace::FindCustomLibrary( const std::string sLibPath )
{
	bool bSuccess = false;

	vector<string> vDirs;

#ifdef WIN32
	GetDirList( sLibPath + "\\*.dll", vDirs );
#elif defined( __APPLE__ )
	// Place holder
#else
	GetDirList( sLibPath + "/*.so", vDirs );
#endif

	if ( vDirs.size() > 0 )
	{
		for ( int i=0; i<int( vDirs.size() ); i++ )
		{
			LoadCustomLibrary( sLibPath, vDirs.at( i ) );

			if ( m_hCustomLib != NULL )
			{
				bSuccess = true;
				break;
			}
		}
	}

	return bSuccess;
}

// +-------------------------------------------------------------------+
// | GetCustomCount                                                    |
// +-------------------------------------------------------------------+
// | Returns the number of algorithms contained within a custom        |
// | library.                                                          |
// |                                                                   |
// | Throws a std::runtime_error                                       |
// +-------------------------------------------------------------------+
int CArcDeinterlace::GetCustomCount()
{
	m_dCustomCount = 0;

	//  If the handle is valid, try to get the function address
 	// +--------------------------------------------------------+
	if ( m_hCustomLib != NULL )
	{
		if ( m_fnGetCustomCount == NULL )
		{
			m_fnGetCustomCount =
					( GetCustomCountFUNC )ArcFindLibrarySymbol(
														m_hCustomLib,
														"GetCustomCount" );
		}

		if ( m_fnGetCustomCount != NULL )
		{
			m_dCustomCount = m_fnGetCustomCount();
		}
		else
		{
			ThrowException(
					"GetCustomCount",
					 ArcSysMsg( "Failed to find custom count function!" ) );
		}
	}

	return m_dCustomCount;
}

// +-------------------------------------------------------------------+
// | GetCustomInfo                                                     |
// +-------------------------------------------------------------------+
// | Returns the custom algorithm information ( algorithm definition   |
// | and name ) for the specified index into the list of custom        |
// | algorithms contained within a library.                            |
// |                                                                   |
// | Throws a std::runtime_error                                       |
// |                                                                   |
// | <IN> -> index      : The index into the custom library list.      |
// | <IN> -> dAlgorithm : The algorithm constant.                      |
// | <IN> -> name       : The name of the algorithm.                   |
// +-------------------------------------------------------------------+
void CArcDeinterlace::GetCustomInfo( int index, int& dAlgorithm, string& name )
{
	const int BUF_SIZE = 100;
	char szBuffer[ BUF_SIZE ];

	ArcZeroMemory( szBuffer, BUF_SIZE );

	//  If the handle is valid, try to get the function address
 	// +--------------------------------------------------------+
	if ( m_hCustomLib != NULL )
	{
		if ( m_fnGetCustomName == NULL )
		{
			m_fnGetCustomName =
					( GetCustomNameFUNC )ArcFindLibrarySymbol(
													m_hCustomLib,
													"GetCustomName" );
		}

		//  If the function address is valid, call the function
 		// +--------------------------------------------------------+
		if ( m_fnGetCustomName != NULL )
		{
			m_fnGetCustomName( index, szBuffer, BUF_SIZE );
		}
		else
		{
			ThrowException(
					"GetCustomInfo",
					ArcSysMsg( "Failed to find custom algorithm name function!" ) );
		}
    }

	dAlgorithm = CArcDeinterlace::DEINTERLACE_CUSTOM + index;

	name = szBuffer;
}

// +-------------------------------------------------------------------+
// | RunCustomAlg                                                      |
// +-------------------------------------------------------------------+
// | Deinterlaces the specified data using a custom algorithm.         |
// |                                                                   |
// | Throws a std::runtime_error                                       |
// |                                                                   |
// |  <IN> -> pData      - Pointer to the image data to deinterlace    |
// |  <IN> -> dCols      - Number of dCols in image to deinterlace     |
// |  <IN> -> dRows      - Number of rows in image to deinterlace      |
// |  <IN> -> dAlgorithm - Algorithm number                            |
// |  <IN> -> dArg       - Any algorithm specific argument             |
// +-------------------------------------------------------------------+
void CArcDeinterlace::RunCustomAlg( void* pData, int dRows, int dCols, int dAlgorithm, int dArg )
{
	int dSuccess = 0;

	//  If the handle is valid, try to get the function address
 	// +--------------------------------------------------------+
	if ( m_hCustomLib != NULL )
	{
		if ( m_fnRunCustomAlg == NULL )
		{
			m_fnRunCustomAlg =
					( RunCustomAlgFUNC )ArcFindLibrarySymbol(
														m_hCustomLib,
														"RunCustomAlg" );
		}
 
		//  If the function address is valid, call the function
 		// +--------------------------------------------------------+
		if ( m_fnRunCustomAlg != NULL )
		{
			int dAlg =
					( dAlgorithm - CArcDeinterlace::DEINTERLACE_CUSTOM );

			dSuccess = 
				m_fnRunCustomAlg( pData,
								  dRows,
								  dCols,
								  dAlg,
								  dArg );

			if ( dSuccess <= 0 )
			{
				ThrowException(
						"RunCustomAlg",
						 ArcSysMsg( GetCustomErrorMsg() ) );
			}
		}
		else
		{
			ThrowException(
					"RunCustomAlg",
					 ArcSysMsg( "Failed to find custom algorithm run function!" ) );
		}
    }
}


// +-------------------------------------------------------------------+
// | Parallel                                                          |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                       Parallel                                    | 	
// |                +---------------------+                            |     	
// |                |          1  ------->|                            |  	
// |                |                     |                            |    	
// |                |                     |                            |
// |                |_____________________|                            |   	
// |                |                     |                            |   	
// |                |                     |                            |   	
// |                |                     |                            |   	
// |                |<--------  0         |                            |  	
// |                +---------------------+                            |   	
// |                                                                   |
// |  <IN>  -> dRows - Number of rows in image to deinterlace          |
// |  <IN>  -> dCols - Number of dCols in image to deinterlace         |
// |  <IN>  -> pData - Pointer to the image pixels to deinterlace      |
// +-------------------------------------------------------------------+
void CArcDeinterlace::Parallel( unsigned short *pData, int dRows, int dCols )
{
	if ( ( ( float )dRows/2 ) != ( int )dRows/2 )
	{
		ThrowException(	"Parallel",
				"Number of ROWS must be EVEN for DEINTERLACE_PARALLEL." );
	}

	for ( int i=0; i<( dCols * dRows )/2; i++ )
	{
		*( m_pNewData.get() + i )                         = *( pData + ( 2 * i ) );
		*( m_pNewData.get() + ( dCols * dRows ) - i - 1 ) = *( pData + ( 2 * i ) + 1 );
	}

	ArcCopyMemory( pData,
				   m_pNewData.get(),
				   ( dCols * dRows * sizeof( unsigned short ) ) );
}

// +-------------------------------------------------------------------+
// | Serial                                                            |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                         Serial                                    | 	
// |                +----------+----------+                            |     	
// |                |          |          |                            |  	
// |                |          |          |                            |    	
// |                |          |          |                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |     0    |    1     |                            |
// |                |<-------- | -------->|                            |
// |                +----------+----------+                            |   	
// |                                                                   |
// |  <IN>  -> dRows - Number of rows in image to deinterlace          |
// |  <IN>  -> dCols - Number of dCols in image to deinterlace         |
// |  <IN>  -> pData - Pointer to the image pixels to deinterlace      |
// +-------------------------------------------------------------------+
void CArcDeinterlace::Serial( unsigned short *pData, int dRows, int dCols )
{
	int p1    = 0;
	int p2    = 0;
	int begin = 0;
	int end   = 0;

	if ( ( float )dCols/2 != ( int )dCols/2 ) 
	{
		ThrowException(	"Serial",
				"Number of COLS must be EVEN for DEINTERLACE_SERIAL." );
	}

	for ( int i=0; i<dRows; i++ )
	{
		// Leave in +0 for clarity
		p1      = i * dCols + 0; // Position in raw image
		p2      = i * dCols + 1;
		begin   = i * dCols + 0; // Position in deinterlaced image
		end     = i * dCols + dCols - 1;
       				
		for ( int j=0; j<dCols; j+=2 )
		{
			*( m_pNewData.get() + begin ) = *( pData + p1 );
			*( m_pNewData.get() + end )   = *( pData + p2 );
			++begin;
			--end;
			p1 += 2;
			p2 += 2;
		}
	}

	ArcCopyMemory( pData,
				   m_pNewData.get(),
				   ( dCols * dRows * sizeof( unsigned short ) ) );
}

// +-------------------------------------------------------------------+
// | CCDQuad                                                           |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                        CCD Quad                                   | 	
// |                +----------+----------+                            |     	
// |                | <--------|--------> |                            |  	
// |                |     3    |    2     |                            |    	
// |                |          |          |                            |
// |                |__________|__________|                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |     0    |    1     |                            |    	
// |                | <--------|--------> |                            |  	
// |                +----------+----------+                            |   	
// |                                                                   |
// |  <IN>  -> dRows - Number of rows in image to deinterlace          |
// |  <IN>  -> dCols - Number of dCols in image to deinterlace         |
// |  <IN>  -> pData - Pointer to the image pixels to deinterlace      |
// +-------------------------------------------------------------------+
void CArcDeinterlace::CCDQuad( unsigned short *pData, int dRows, int dCols )
{
	int i       = 0;
	int j       = 0;
	int counter = 0;
	int end     = 0;
	int begin   = 0;

	if ( ( float )dCols/2 != ( int )dCols/2 || ( float )dRows/2 != ( int )dRows/2 )
	{
		ThrowException(
			"CCDQuad",
			"Number of COLS and ROWS must be EVEN for DEINTERLACE_CCD_QUAD." );
	}

	while( i < dCols*dRows )
	{
		if ( counter%( dCols/2 ) == 0 )
		{
			end = ( dCols * dRows ) - ( dCols * j ) - 1;
           	begin = ( dCols * j ) + 0;	// Left in 0 for clarity
           	j++;						// Number of completed rows
           	counter = 0;				// Reset for next convergece
		}
        				
		*( m_pNewData.get() + begin + counter )             = *( pData + i++ );	// front_row--->
		*( m_pNewData.get() + begin + dCols - 1 - counter ) = *( pData + i++ );	// front_row<--
		*( m_pNewData.get() + end - counter )               = *( pData + i++ );	// end_row<----
		*( m_pNewData.get() + end - dCols + 1 + counter )   = *( pData + i++ );	// end_row---->

		counter++;
	}

	ArcCopyMemory( pData,
				   m_pNewData.get(),
				   ( dCols * dRows * sizeof( unsigned short ) ) );
}

// +-------------------------------------------------------------------+
// | IRQuad                                                            |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                        IR Quad                                    | 	
// |                +----------+----------+                            |     	
// |                | -------> |--------> |                            |  	
// |                |    0     |    1     |                            |    	
// |                |          |          |                            |
// |                |__________|__________|                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |    3     |    2     |                            |    	
// |                | -------> |--------> |                            |  	
// |                +----------+----------+                            |   	
// |                                                                   |
// |  <IN>  -> dRows - Number of rows in image to deinterlace          |
// |  <IN>  -> dCols - Number of dCols in image to deinterlace         |
// |  <IN>  -> pData - Pointer to the image pixels to deinterlace      |
// +-------------------------------------------------------------------+
void CArcDeinterlace::IRQuad( unsigned short *pData, int dRows, int dCols )
{
	int i       = 0;
	int j       = dRows - 1;
	int counter = 0;
	int end     = 0;
	int begin   = 0;

	if ( ( float )dCols/2 != ( int )dCols/2 || ( float )dRows/2 != ( int )dRows/2 )
	{
		ThrowException(
			"IRQuad",
			"Number of COLS and ROWS must be EVEN for DEINTERLACE_IR_QUAD." );
	}

	while( i < dCols*dRows )
	{
		if ( counter%( dCols/2 ) == 0 )
		{
			end = ( j - ( dRows/2 ) )*dCols;
			begin = j*dCols;
			j--; 			// Nnumber of completed rows
			counter = 0; 	// Reset for next convergece
		}

		*( m_pNewData.get() + begin + counter )               = *( pData + i++ ); // front_row--->
		*( m_pNewData.get() + begin + ( dCols/2 ) + counter ) = *( pData + i++ ); // front_row<--
		*( m_pNewData.get() + end + ( dCols/2 ) + counter )   = *( pData + i++ ); // end_row<----
		*( m_pNewData.get() + end + counter )                 = *( pData + i++ ); // end_row---->
		counter++;
	}

	ArcCopyMemory( pData,
				   m_pNewData.get(),
				   ( dCols * dRows * sizeof( unsigned short ) ) );
}

// +-------------------------------------------------------------------+
// | IRQuad (Correlated Double Sampling )                              |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                      IR Quad CDS                                  | 	
// |                +----------+----------+                            |     	
// |                | -------> |--------> |                            |  	
// |                |    0     |    1     |                            |    	
// |                |          |          |                            |
// |                |__________|__________|                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |    3     |    2     |                            |    	
// |                | -------> |--------> |                            |  	
// |                +----------+----------+                            |   	
// |                | -------> |--------> |                            |  	
// |                |    0     |    1     |                            |    	
// |                |          |          |                            |
// |                |__________|__________|                            |
// |                |          |          |                            |
// |                |          |          |                            |
// |                |    3     |    2     |                            |    	
// |                | -------> |--------> |                            |  	
// |                +----------+----------+                            |   	
// |                                                                   |
// |  <IN>  -> dRows - Number of rows in image to deinterlace          |
// |  <IN>  -> dCols - Number of dCols in image to deinterlace         |
// |  <IN>  -> pData - Pointer to the image pixels to deinterlace      |
// +-------------------------------------------------------------------+
void CArcDeinterlace::IRQuadCDS( unsigned short *pData, int dRows, int dCols )
{
	int i       = 0;
	int j       = 0;
	int counter = 0;
	int end     = 0;
	int begin   = 0;
	int oldRows = 0;

	unsigned short* pOldStart = pData;
	unsigned short* pNewStart = m_pNewData.get();

	if ( pOldStart == NULL || pNewStart == NULL )
	{
		ThrowException(	"IRQuadCDS",
						"One or more image buffers are NULL." );
	}

	else if ( float( dCols/2 ) != int( dCols/2 ) ||
			  float( dRows/2 ) != int( dRows/2 ) )
	{
		ThrowException(
			"IRQuadCDS",
			"Number of COLS and ROWS must be EVEN for DEINTERLACE_CDS_IR_QUAD." );
	}

	// Set the the number of rows to half the image size.
	oldRows	= dRows;
	dRows	= ( dRows / 2 );

	// Deinterlace the two image halves separately.
	for ( int imageSection = 0; imageSection < 2; imageSection++ )
	{
		i		= 0;
		j		= dRows - 1;
		counter	= 0;
		end		= 0;
		begin	= 0;

		while( i < ( dCols * dRows ) )
		{
   			if ( ( counter % ( dCols / 2 ) ) == 0 )
   			{
				end = ( j - ( dRows/2 ) )*dCols;
				begin = j*dCols;
				j--; 			// Number of completed rows
				counter = 0; 	// Reset for next convergece
			}

			*( pNewStart + begin + counter )                 = *( pOldStart + i++ ); // front_row--->
			*( pNewStart + begin + ( dCols / 2 ) + counter ) = *( pOldStart + i++ ); // front_row<--
			*( pNewStart + end + ( dCols / 2 ) + counter )   = *( pOldStart + i++ ); // end_row<----
			*( pNewStart + end + counter )                   = *( pOldStart + i++ ); // end_row---->

			counter++;
		}

		pOldStart += ( dRows * dCols );
		pNewStart += ( dRows * dCols );
	}

	ArcCopyMemory( pData,
				   m_pNewData.get(),
				   ( dCols * oldRows * sizeof( unsigned short ) ) );
}


// +-------------------------------------------------------------------+
// | HawaiiRG                                                          |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                          HawaiiRG                                 | 	
// |              +-------+-------+-------+-------+                    |
// |              |       |       |       |       |                    |
// |              |       |       |       |       |                    |
// |              |       |       |       |       |                    |
// |              |       |       |       |       |                    |
// |              |       |       |       |       |                    |
// |              |       |       |       |       |                    |
// |              |   0   |   1   |   2   |   3   |                    |
// |              | ----> | ----> | ----> | ----> |                    |
// |              +-------+-------+-------+-------+                    |
// |                                                                   |
// |  <IN>  -> dRows - Number of rows in image to deinterlace          |
// |  <IN>  -> dCols - Number of dCols in image to deinterlace         |
// |  <IN>  -> pData - Pointer to the image pixels to deinterlace      |
// |  <IN>  -> dArg  - The number of channels in the image (16, 32, ..)|
// +-------------------------------------------------------------------+
void CArcDeinterlace::HawaiiRG( unsigned short *pData, int dRows, int dCols, int dArg )
{
	const int ERR = 0x455252;

	if ( ( float )dCols/2 != ( int )dCols/2 )
	{
		ThrowException(
				"HawaiiRG",
				"Number of COLS must be EVEN for DEINTERLACE_HAWAII_RG." );
	}

	else if ( dArg == 1 )
	{
		// Ignore and don't de-interlace. Bob requested this
		// action on March 30, 2012.
	}

	else if ( dArg <= 0 || dArg == ERR )
	{
		ThrowException(
			"HawaiiRG",
			"The number of readout channels must be supplied for DEINTERLACE_HAWAII_RG." );
	}

	else if ( dArg % 2 != 0 )
	{
		ThrowException(
			"HawaiiRG",
			"The readout channel count must be EVEN for DEINTERLACE_HAWAII_RG." );
	}

	else
	{
		unsigned short *rowPtr = m_pNewData.get();
		int offset             = dCols / dArg;
		int dataIndex          = 0;

		for ( int r=0; r<dRows; r++ )
		{
			rowPtr = m_pNewData.get() + ( dCols * r );

			for ( int c=0; c<dCols/dArg; c++ )
			{
				for ( int i=0; i<dArg; i++ )
				{
					rowPtr[ c + i * offset ] = pData[ dataIndex++ ];
				}
			}
		}

		ArcCopyMemory( pData,
					   m_pNewData.get(),
					   ( dCols * dRows * sizeof( unsigned short ) ) );
	}
}

// +-------------------------------------------------------------------+
// | STA1600                                                           |
// +-------------------------------------------------------------------+
// |                                                                   |
// |                            STA1600                                | 	
// |                                                                   |
// |                <-+     <-+     <-+           <-+                  |
// |              +---|---+---|---+---|---+     |---|---+              |
// |              |   |   |   |   |   |   |     |   |   |              |
// |              |   8   |   9   |   10  | ... |  15   |              |
// |              |       |       |       |     |       |              |
// |              |       |       |       |     |       |              |
// |              +-------+-------+-------+ ... +-------+              |
// |              |       |       |       |     |       |              |
// |              |       |       |       |     |       |              |
// |              |   0   |   1   |   2   | ... |   7   |              |
// |              |   |   |   |   |   |   |     |   |   |              |
// |              +---|---+---|---+---|---+     |---|---+              |
// |                  |       |       |             |                  |
// |                <-+     <-+     <-+           <-+                  |
// |                                                                   |
// |  <IN>  -> pData - Pointer to the image pixels to deinterlace      |
// |  <IN>  -> dRows - Number of rows in image to deinterlace          |
// |  <IN>  -> dCols - Number of dCols in image to deinterlace         |
// +-------------------------------------------------------------------+
void CArcDeinterlace::STA1600( unsigned short *pData, int dRows, int dCols )
{
	if ( ( dCols % 16 ) != 0  )
	{
		ThrowException(
			"STA1600",
			"Number of COLS must be a multiple of 16 for DEINTERLACE_STA1600" );
	}

	if ( ( dRows % 2 ) != 0  )
	{
		ThrowException(
			"STA1600",
			"Number of ROWS must be a multiple of 2 for DEINTERLACE_STA1600" );
	}

	unsigned short *botPtr = m_pNewData.get();
	unsigned short *topPtr = m_pNewData.get() + ( dCols * ( dRows - 1 ) );
	long offset = dCols / 8;		// Needs to be long because of array size
	long dataIndex = 0;			// Data index

	for ( long r=0; r<dRows/2; r++ )
	{
		topPtr = m_pNewData.get() + ( dCols * ( dRows - r - 1 ) );
		botPtr = m_pNewData.get() + ( dCols * r );

		for ( long c=0; c<( dCols / 8 ); c++ )
		{
			botPtr[ c + 7 * offset ] = pData[ dataIndex++ ];
			botPtr[ c + 6 * offset ] = pData[ dataIndex++ ];
			botPtr[ c + 5 * offset ] = pData[ dataIndex++ ];
			botPtr[ c + 4 * offset ] = pData[ dataIndex++ ];
			botPtr[ c + 3 * offset ] = pData[ dataIndex++ ];
			botPtr[ c + 2 * offset ] = pData[ dataIndex++ ];
			botPtr[ c + 1 * offset ] = pData[ dataIndex++ ];
			botPtr[ c + 0 * offset ] = pData[ dataIndex++ ];

			topPtr[ c + 7 * offset ] = pData[ dataIndex++ ];
			topPtr[ c + 6 * offset ] = pData[ dataIndex++ ];
			topPtr[ c + 5 * offset ] = pData[ dataIndex++ ];
			topPtr[ c + 4 * offset ] = pData[ dataIndex++ ];
			topPtr[ c + 3 * offset ] = pData[ dataIndex++ ];
			topPtr[ c + 2 * offset ] = pData[ dataIndex++ ];
			topPtr[ c + 1 * offset ] = pData[ dataIndex++ ];
			topPtr[ c + 0 * offset ] = pData[ dataIndex++ ];
		}
	}

	ArcCopyMemory( pData,
				   m_pNewData.get(),
				   ( dCols * dRows * sizeof( unsigned short ) ) );
}


// +-------------------------------------------------------------------+
// | ThrowException                                                    |
// +-------------------------------------------------------------------+
// | Throws a std::runtime_error using the specified message string.   |
// |                                                                   |
// | <IN> -> sMethodName : The method where the exception occurred.    |
// | <IN> -> sMsg        : The error message.                          |
// +-------------------------------------------------------------------+
void CArcDeinterlace::ThrowException( std::string sMethodName, std::string sMsg )
{
	ostringstream oss;

	oss << "( CArcDeinterlace::"
		<< ( sMethodName.empty() ? "???" : sMethodName )
		<< "() ): "
		<< sMsg
		<< ends;

	throw std::runtime_error( ( const std::string )oss.str() );
}


// +-------------------------------------------------------------------+
// | GetDirList                                                        |
// +-------------------------------------------------------------------+
// | Returns the list of files within the specified directory path.    |
// | Note: the "." and ".." path listings are excluded from the list.  |
// |                                                                   |
// | Throws a std::runtime_error                                       |
// |                                                                   |
// | <IN>  -> sPath : The directory path to list the file for.         |
// | <OUT> -> vDirs : The vector list of all files found within the    |
// |                  specified directory path.                        |
// +-------------------------------------------------------------------+
void CArcDeinterlace::GetDirList( const string& sPath, vector<string>& vDirs )
{
#ifdef WIN32

	WIN32_FIND_DATAA tFindFileData;

	HANDLE hFind = FindFirstFileA( sPath.c_str(), &tFindFileData );

	if ( hFind == INVALID_HANDLE_VALUE ) 
	{
		ostringstream oss;

		oss << "Invalid File Handle. GetLastError reports: "
			<< GetLastError()
			<< ends;

		ThrowException( "GetDirList", oss.str() );
	} 
	else 
	{
		string sFile( tFindFileData.cFileName );

		if ( sFile.compare( "." ) != 0 && sFile.compare( ".." ) != 0 )
		{
			vDirs.push_back( sFile );
		}

		//  List all the other files in the directory
		// +-------------------------------------------------+
		while ( FindNextFileA( hFind, &tFindFileData ) != 0 ) 
		{
			string sFile( tFindFileData.cFileName );

			if ( sFile.compare( "." ) != 0 && sFile.compare( ".." ) != 0 )
			{
				vDirs.push_back( sFile );
			}
		}

		FindClose( hFind );
	}

#else

	struct dirent* pDirEntry = NULL;
	DIR	*pDir = NULL;

	pDir = opendir( sPath.c_str() );

	if ( pDir == NULL )
	{
		ostringstream oss;

		oss << "Failed to open dir:: " << sPath;

		ThrowException( "GetDirList", oss.str() );
	}
	else
	{
		while ( ( pDirEntry = readdir( pDir ) ) != NULL )
		{
			std::string sDirEntry( pDirEntry->d_name );

			if ( sDirEntry.compare( "." ) != 0 && sDirEntry.compare( ".." ) != 0 )
			{
				vDirs.push_back( sDirEntry );
			}
		}

		closedir( pDir );
	}

#endif
}


// +-------------------------------------------------------------------+
// | LoadCustomLibrary                                                 |
// +-------------------------------------------------------------------+
// | Loads the dynamic library ( .dll, .so ) using the specified path  |
// | and file name.                                                    |
// |                                                                   |
// | Throws a std::runtime_error                                       |
// |                                                                   |
// | <IN> -> sLibPath : The library path.                              |
// | <IN> -> sLibName : The library file name.                         |
// +-------------------------------------------------------------------+
void CArcDeinterlace::LoadCustomLibrary( const string sLibPath, const string sLibName )
{
#ifdef WIN32

	if ( !SetDllDirectoryA( sLibPath.c_str() ) )
	{
		ostringstream oss;

		oss << "Failed to set library path [ "
			<< sLibPath
			<< " ]!";

		ThrowException( "LoadCustomLibrary", oss.str() );
	}

#endif

	//  Get a handle to the DLL module
	// +--------------------------------------------------------+
	ArcCustomLib hCustomLib = ArcLoadLibrary( sLibPath, sLibName );

	if ( hCustomLib == NULL )
	{
		ostringstream oss;

		oss << "Failed to find load library [ PATH: "
			<< sLibPath
			<< ", LIB: "
			<< sLibName
#ifdef WIN32
			<< " ]! GetLastError: "
			<< GetLastError();
#else
			<< " ]! dlerror: "
			<< dlerror();
#endif

		ThrowException( "LoadCustomLibrary", oss.str() );
	}

	if ( !IsCustomLibrary( hCustomLib ) )
	{
		ArcFreeLibrary( hCustomLib );
	}
	else
	{
		m_hCustomLib = hCustomLib;
	}
}


// +-------------------------------------------------------------------+
// | IsCustomLibrary                                                   |
// +-------------------------------------------------------------------+
// | Verifies that the specified library handle points to a custom     |
// | deinterlace library. Returns 'true' if the "IS_CUSTOM_ALGORITHM"  |
// | system returns 1; 'false' otherwise ( i.e. the symbol is not      |
// | found ).                                                          |
// |                                                                   |
// | <IN> -> hCustomLib : A custom library handle.                     |
// +-------------------------------------------------------------------+
bool CArcDeinterlace::IsCustomLibrary( ArcCustomLib hCustomLib )
{
	bool bSuccess  = false;
	int* pIsCustom = NULL;

	//  If the handle is valid, try to get the function address
 	// +--------------------------------------------------------+
	if ( hCustomLib != NULL )
	{
		pIsCustom = ( int * )ArcFindLibrarySymbol(
										hCustomLib,
										"IS_CUSTOM_ALGORITHM" );

		if ( pIsCustom != NULL )
		{
			bSuccess = ( *pIsCustom != 0 );
		}
	}

	return bSuccess;
}


// +-------------------------------------------------------------------+
// | GetErrorMessage                                                   |
// +-------------------------------------------------------------------+
// | Returns any error message that may have occurred while            |
// | deinterlacing with a custom deinterlace library.                  |
// +-------------------------------------------------------------------+
string CArcDeinterlace::GetCustomErrorMsg()
{
	const int dBufferSize = 100;
	char szBuffer[ dBufferSize ];

	//  If the handle is valid, try to get the function address
 	// +--------------------------------------------------------+
	if ( m_hCustomLib != NULL )
	{
		if ( m_fnGetErrorMsg == NULL )
		{
			m_fnGetErrorMsg =
					( GetErrorMsgFUNC )ArcFindLibrarySymbol(
													m_hCustomLib,
													"GetErrorMsg" );
		}

		if ( m_fnGetErrorMsg != NULL )
		{
			m_fnGetErrorMsg( szBuffer, dBufferSize );
		}
		else
		{
			ThrowException(
					"GetCustomErrorMsg",
					 ArcSysMsg( "Failed to find custom error message function!" ) );
		}
	}

	return string( szBuffer );
}