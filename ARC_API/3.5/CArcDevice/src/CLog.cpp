//
// Log.cpp : Defines a string message logging class
//
#include <fstream>
#include "CLog.h"
#include "CArcTools.h"

#ifdef WIN32
	#include <windows.h>
#else
	#include <iostream>
	#include <cstring>
#endif


using namespace std;
using namespace arc;


std::queue<std::string>::size_type CLog::Q_MAX = 256;	// Maximum number of messages to log


// +----------------------------------------------------------------------------
// |  SetMaxSize
// +----------------------------------------------------------------------------
// |  Sets the maximum number of messages that the Q can hold.
// |
// |  <IN> -> dSize - The maximum number of message the Q can hold. Must be > 0.
// +----------------------------------------------------------------------------
void CLog::SetMaxSize( int dSize )
{
	if ( dSize > 0 )
	{
		Q_MAX = queue<std::string>::size_type( dSize );
	}
}

// +----------------------------------------------------------------------------
// |  Put
// +----------------------------------------------------------------------------
// |  Inserts a message into the log queue. It dumps the oldest message if
// |  the queue size is greater than or equal to Q_MAX.
// |
// |  <IN> -> fmt - C-printf style format (sort of):
// |				%d   -> Integer
// |				%f   -> Double
// |				%s   -> Char *, std::string not accepted
// |				%x,X -> Integer as hex, produces uppercase only
// |				%e   -> Special case that produces a string message from
// |				        one of the system functions ::GetLastError or
// |				        strerror, depending on the OS.
// +----------------------------------------------------------------------------
void CLog::Put( const char *szFmt, ... )
{
	va_list ap;
	va_start( ap, szFmt );

	string sTemp = CArcTools::FormatString( szFmt, ap );

	va_end( ap );

	//
	// Check the Q size. Dump the oldest if
	// the Q is too big.
	//
	if ( m_sQueue.size() >= Q_MAX )
	{
		m_sQueue.pop();
	}

	if ( !sTemp.empty() )
	{
		m_sQueue.push( sTemp );
	}
}

// +----------------------------------------------------------------------------
// |  GetNext
// +----------------------------------------------------------------------------
// |  Returns the oldest string ( as char * ) from the front of the queue. Also
// |  pops the message from the queue. Applications should call Empty() to check
// |  if more messages are available.
// |
// |  <OUT> -> The oldest message in the queue
// +----------------------------------------------------------------------------
const string CLog::GetNext()
{
	string sTemp( "" );

	if ( !m_sQueue.empty() )
	{
		sTemp = string( m_sQueue.front() );

		m_sQueue.pop();
	}

	return ( const string )sTemp;
}

// +----------------------------------------------------------------------------
// |  GetLast
// +----------------------------------------------------------------------------
// |  Returns the newest string ( as char * ) from the back of the queue. Also
// |  pops the message from the queue.
// |
// |  <OUT> -> The newest message in the queue
// +----------------------------------------------------------------------------
const string CLog::GetLast()
{
	string sTemp( "" );

	if ( !m_sQueue.empty() )
	{
		sTemp = string( m_sQueue.back() );

		while ( !m_sQueue.empty() )
		{
			m_sQueue.pop();
		}
	}
	
	return ( const string )sTemp;
}

// +----------------------------------------------------------------------------
// |  GetLogCount
// +----------------------------------------------------------------------------
// |  <OUT> -> Returns the number of elements in the Q.
// +----------------------------------------------------------------------------
int CLog::GetLogCount()
{
	return int( m_sQueue.size() );
}

// +----------------------------------------------------------------------------
// |  Empty
// +----------------------------------------------------------------------------
// |  Checks if the Q is empty. i.e. if there are any messages in the Q.
// |
// |  <OUT> -> Returns 'true' if the queue is empty; 'false' otherwise.
// +----------------------------------------------------------------------------
bool CLog::Empty()
{
	return m_sQueue.empty();
}

// +----------------------------------------------------------------------------
// |  SelfTest
// +----------------------------------------------------------------------------
// |  Performs a self-test. Output goes to a message box on windows and to the
// |  terminal on linux systems.
// +----------------------------------------------------------------------------
void CLog::SelfTest()
{
	ostringstream oss;

	oss << "Putting 3 controller commands to Q ... ";
	Put( CArcTools::CmdToString( 0x444F4E, 0x2, 0x54444C, 0x112233 ).c_str() );
	Put( CArcTools::CmdToString( 0x455252, 0x2, 0x111111, 0x1, 0x2, 0x3, 0x4 ).c_str() );
	Put( CArcTools::CmdToString( 0x444F4E, 0x2, 0x535450 ).c_str() );
	oss << "done" << endl;

	oss << "Reading back Q: "<< endl;
	while ( !Empty() )
	{
		oss << "\t" << GetNext() << endl;
	}
	oss << "Done reading Q!" << endl;

#ifdef WIN32
	::MessageBoxA( NULL, oss.str().c_str(), "CLog::SelfTest()", MB_OK );
#else
	cout << endl
		 << "+--------------------------------------------------------------+" << endl
		 << "|  CLog::SelfTest()                                            |" << endl
		 << "+--------------------------------------------------------------+" << endl
		 << oss.str()
		 << endl;
#endif
}
