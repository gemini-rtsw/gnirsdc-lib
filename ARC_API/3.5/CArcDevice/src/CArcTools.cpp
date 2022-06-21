#ifdef WIN32
	#include <windows.h>
#endif

#include <functional>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <cstdarg>
#include <cctype>
#include <ios>
#include "CArcTools.h"
#include "ArcDefs.h"

#if defined( linux ) || defined( __linux )
	#include <cstring>
#endif

using namespace std;
using namespace arc;


// +----------------------------------------------------------------------------
// |  ReplyToString
// +----------------------------------------------------------------------------
// |  Returns the character std::string representation of the specified reply.
// |  If the reply is not a standard value, then the hexadecimal value of the
// |  reply is returned as a character string.
// |
// |  Example:  reply = 0x455252 -> returns 'ERR'
// |
// |  <IN> -> dReply - The reply to be converted to a character std::string.
// +----------------------------------------------------------------------------
std::string CArcTools::ReplyToString( int dReply )
{
	ostringstream oss;

		 if ( dReply == DON  ) { oss << "DON";  }
	else if ( dReply == ERR  ) { oss << "ERR";  }
	else if ( dReply == TOUT ) { oss << "TOUT"; }
	else if ( dReply == ROUT ) { oss << "ROUT"; }
	else if ( dReply == SYR  ) { oss << "SYR";  }
	else if ( dReply == RST  ) { oss << "RST";  }
	else if ( dReply == HERR ) { oss << "HERR"; }
	else
	{
		oss.setf( ios::hex, ios::basefield );
		oss.setf( ios::uppercase );

		oss << "0x" << dReply;
	}

	return oss.str();
}

// +----------------------------------------------------------------------------
// |  CmdToString
// +----------------------------------------------------------------------------
// |  Returns the character std::string representation of the specified command.
// |  If the command fails to be converted, then the hexadecimal value of the
// |  command is returned as a character string.
// |
// |  Example:  value = 0x524343 -> returns 'RCC'
// |
// |  <IN> -> dCmd - The command to be converted to a character std::string.
// +----------------------------------------------------------------------------
std::string CArcTools::CmdToString( int dCmd )
{
	ostringstream oss;
	oss.setf( ios::hex, ios::basefield );
	oss.setf( ios::uppercase );

	if ( isalpha( ( ( dCmd & 0xff0000 ) >> 16 ) ) &&
		 isalpha( ( ( dCmd & 0x00ff00 ) >> 8 ) )  &&
		 isalpha(   ( dCmd & 0x0000ff ) ) )
	{
		oss << ( char )( ( dCmd & 0xff0000 ) >> 16 )
			<< ( char )( ( dCmd & 0x00ff00 ) >> 8 )
			<< ( char )(   dCmd & 0x0000ff );
	}
	else
	{
		oss << "0x" << dCmd;
	}

	return oss.str();
}

// +----------------------------------------------------------------------------
// |  CmdToString
// +----------------------------------------------------------------------------
// |  Method to bundle command values into a string that can be used to throw
// |  a std::runtime_error exception.
// |
// |  <IN> -> dBoardId    : The command header.
// |  <IN> -> dCmd        : The command value.
// |  <IN> -> dArg1       : The 1st argument.
// |  <IN> -> dArg2       : The 2nd argument.
// |  <IN> -> dArg3       : The 3rd argument.
// |  <IN> -> dArg4       : The 4th argument.
// |  <IN> -> dReply      : The received command reply value.
// |  <IN> -> dSysErr     : A system error number, used to get system message.
// +----------------------------------------------------------------------------
string CArcTools::CmdToString( int dReply, int dBoardId, int dCmd, int dArg1,
							   int dArg2, int dArg3, int dArg4, int dSysErr )
{
	ostringstream oss;

	oss.setf( ios::hex, ios::basefield );
	oss.setf( ios::uppercase );

	int dNumOfArgs = 2;
	if ( dArg1 >= 0 ) { dNumOfArgs++; }
	if ( dArg2 >= 0 ) { dNumOfArgs++; }
	if ( dArg3 >= 0 ) { dNumOfArgs++; }
	if ( dArg4 >= 0 ) { dNumOfArgs++; }

	oss << "[ 0x" << ( dBoardId << 8 | dNumOfArgs );	// Header
	oss << " " << CArcTools::CmdToString( dCmd );		// Command

	if ( dArg1 >= 0 ) { oss << " 0x" << dArg1; }		// Arg1
	if ( dArg2 >= 0 ) { oss << " 0x" << dArg2; }		// Arg2
	if ( dArg3 >= 0 ) { oss << " 0x" << dArg3; }		// Arg3
	if ( dArg4 >= 0 ) { oss << " 0x" << dArg4; }		// Arg4

	oss << " -> "
		<< CArcTools::ReplyToString( dReply )
		<< " ]";										// Reply

	if ( dSysErr > 0 )	// System error
	{
		oss << endl
			<< dec
			<< CArcTools::GetSystemMessage( dSysErr );
	}

	return oss.str();
}

// +----------------------------------------------------------------------------
// |  StringToCmd
// +----------------------------------------------------------------------------
// |  Method to convert an ASCII command string, such as 'TDL' to the
// |  equivalent integer value.  Throws std::runtime_error if ASCII command
// |  parameter is not three characters in length.
// |
// |  <IN> -> sCmd : The string command value.
// +----------------------------------------------------------------------------
int CArcTools::StringToCmd( string sCmd )
{
	if ( sCmd.find( '-' ) != string::npos )
	{
		CArcTools::ThrowException( "CArcTools",
								   "StringToCmd",
								   "Negative numbers not allowed!" );
	}

	if ( sCmd.length() < 3 )
	{
		CArcTools::ThrowException( "CArcTools",
								   "StringToCmd",
								   "Invalid parameter length: %d, must be 3!",
								    sCmd.length() );
	}

	string sTemp = CArcTools::StringToUpper( sCmd );

	int dCmd = int( sTemp.at( 0 ) ) << 16 |
			   int( sTemp.at( 1 ) ) << 8  |
			   int( sTemp.at( 2 ) );

	return dCmd;
}

// +----------------------------------------------------------------------------
// |  FormatString
// +----------------------------------------------------------------------------
// |  Method uses printf-style formatting that is then used to throw a "const
// |  char *" exception.
// |
// |  <IN> -> pszFmt : A printf-style format string, followed by variables.
// |
// |				   Acceptable format parameters:
// |				   %d   -> integer
// |				   %f   -> double
// |				   %s   -> char * string
// |				   %e   -> system message
// |				   %x/X -> lower/upper case hexadecimal integer
// +----------------------------------------------------------------------------
std::string CArcTools::FormatString( const char *pszFmt, ... )
{
	ostringstream oss;
	char *ch, *sVal;
	va_list ap;

	va_start( ap, pszFmt );

	for ( ch = ( char * )pszFmt; *ch; ch++ )
	{
		if ( *ch != '%' )
		{
			oss << *ch;
			continue;
		}

		switch ( *++ch )
		{
			case 'd':
			{
				oss << va_arg( ap, int );
			}
			break;

			case 'f':
			{
				oss << va_arg( ap, double );
			}
			break;

			case 's':
			{
				for ( sVal = va_arg( ap, char * ); *sVal; sVal++ )
				{
					oss << *sVal;
				}
			}
			break;

			case 'e':
			{
				oss << GetSystemMessage( va_arg( ap, long ) );
			}
			break;

			case 'X':
			case 'x':
			{
				oss << uppercase << hex << va_arg( ap, int ) << dec;
			}
			break;

			default:
			{
				oss << *ch;
			}
			break;
		}
	}
	va_end( ap );

	oss << ends;

	return oss.str();
}

// +----------------------------------------------------------------------------
// |  StringToUpper
// +----------------------------------------------------------------------------
// |  Function to transform a string into all uppercase letters.
// |
// |  <IN> -> sStr : The std::string to transform.
// +----------------------------------------------------------------------------
const std::string CArcTools::StringToUpper( std::string sStr )
{
	std::transform( sStr.begin(),
					sStr.end(),
					sStr.begin(),
					std::ptr_fun<int,int>( toupper ) );

	return sStr;
}

// +----------------------------------------------------------------------------
// |  GetSystemMessage
// +----------------------------------------------------------------------------
// |  Function used by ThrowException functions to get a formatted message
// |  string from the specified system error code.
// |
// |  <IN> -> dCode   : A system error code. errno on linux/solaris.
// +----------------------------------------------------------------------------
std::string CArcTools::GetSystemMessage( int dCode )
{
	ostringstream oss;

#ifdef WIN32

	LPTSTR lpBuffer;

	FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					( DWORD )dCode,
					MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
					( LPTSTR )&lpBuffer,
					0,
					NULL );

	char szBuffer[ 1024 ] = { '\0' };

	int dBytes = WideCharToMultiByte( CP_ACP,
									  0,
									  reinterpret_cast<LPWSTR>( lpBuffer ),
									  -1,
									  szBuffer,
									  sizeof( szBuffer ),
									  NULL,
									  NULL );

	if ( dBytes > 0 )
	{
		oss << szBuffer;
	}

	LocalFree( lpBuffer );

#else

	if ( dCode != -1 )
	{
		oss << "( errno: " << dCode << " ) - " << strerror( dCode );
	}

#endif

	return oss.str();
}

// +----------------------------------------------------------------------------
// |  ConvertWideToAnsi
// +----------------------------------------------------------------------------
// |  Converts the specified wide char string (unicode) to an ansi std::string.
// |
// |  <IN> -> wcharString - Wide character string to be converted to std::string.
// +----------------------------------------------------------------------------
std::string CArcTools::ConvertWideToAnsi( wchar_t wcharString[] )
{
	string newString = "";

#ifdef WIN32
	char buffer[ 1024 ] = { '\0' };

	int bytes = WideCharToMultiByte( CP_ACP,
									 0,
									 ( LPCWSTR )wcharString,
									 -1,
									 buffer,
									 sizeof( buffer ),
									 NULL,
									 NULL );

	if ( bytes > 0 )
	{
		newString = string( buffer );
	}
#endif

	return newString;
}

// +----------------------------------------------------------------------------
// |  ConvertWideToAnsi
// +----------------------------------------------------------------------------
// |  Converts the specified wide string ( unicode ) to an ansi std::string.
// |
// |  <IN> -> wsString - Wide string to be converted to std::string.
// +----------------------------------------------------------------------------
std::string CArcTools::ConvertWideToAnsi( const std::wstring& wsString )
{
	ostringstream oss;

	const ctype<wchar_t>& ctfacet = use_facet< ctype<wchar_t> >( oss.getloc() );

	for( size_t i=0; i<wsString.size(); ++i )
	{
		oss << ctfacet.narrow( wsString[ i ], 0 );
	}

	return oss.str();
}

// +----------------------------------------------------------------------------
// |  ConvertAnsiToWide
// +----------------------------------------------------------------------------
// |  Converts the specified ANSI char string to a unicode std::wstring.
// |
// |  <IN> -> cString - ANSI C character string to be converted to std::wstring.
// +----------------------------------------------------------------------------
std::wstring CArcTools::ConvertAnsiToWide( const char *szString )
{
	wostringstream wostr;
	wostr << szString << ends;
	return wostr.str();
}

// +----------------------------------------------------------------------------
// |  ConvertIntToString
// +----------------------------------------------------------------------------
// |  Converts the specified integer value to a std::string. This is a
// |  convenience method.
// |
// |  <IN> -> dNumber - The integer to convert to std::string.
// +----------------------------------------------------------------------------
std::string CArcTools::ConvertIntToString( int dNumber )
{
	ostringstream oss;
	oss << dNumber << ends;
	return oss.str();
}

// +----------------------------------------------------------------------------
// |  StringToHex
// +----------------------------------------------------------------------------
// |  Converts the specified std::string to a long integer value. This is a
// |  convenience method.
// |
// |  <IN> -> sStr - The std::string to convert.
// +----------------------------------------------------------------------------
long CArcTools::StringToHex( std::string sStr )
{
	long lVal = -1;

	stringstream ss( sStr );
	ss >> hex >> lVal;

	return lVal;
}

// +----------------------------------------------------------------------------
// |  StringToChar
// +----------------------------------------------------------------------------
// |  Converts the specified std::string, which represents a single character,
// |  to a C char value. This is a convenience method.
// |
// |  <IN> -> sStr - The std::string to convert to a char.
// +----------------------------------------------------------------------------
char CArcTools::StringToChar( std::string sStr )
{
	char cVal = ( char )' ';

	stringstream ss( sStr );
	ss >> cVal;

	return cVal;
}

// +---------------------------------------------------------------------------+
// |  ThrowException                                                           
// +---------------------------------------------------------------------------+
// |  Throws a std::runtime_error based on the supplied cfitsion status value. 
// |                                                                           
// |  <IN> -> sClassName  : Name of the class where the exception occurred.    
// |  <IN> -> sMethodName : Name of the method where the exception occurred.   
// |  <IN> -> sMsg        : The exception message.                             
// +---------------------------------------------------------------------------+
void CArcTools::ThrowException( string sClassName, string sMethodName, string sMsg )
{
	ostringstream oss;

	oss << "( "
		<< ( sClassName.empty() ? "?Class?" : sClassName )
		<< "::"
		<< ( sMethodName.empty() ? "?Method?" : sMethodName )
		<< "() ): "
		<< sMsg
		<< ends;

	throw std::runtime_error( ( const std::string )oss.str() );
}

// +----------------------------------------------------------------------------
// |  ThrowException
// +----------------------------------------------------------------------------
// |  Method uses printf-style formatting that is then used to throw a
// |  runtime_error exception.
// |
// |  <IN> -> sClassName  : Name of the class where the exception occurred.
// |  <IN> -> sMethodName : Name of the method where the exception occurred.
// |  <IN> -> pszFmt : A printf-style format string, followed by variables.
// |				   Acceptable format parameters:
// |				   %d   -> integer
// |				   %f   -> double
// |				   %s   -> char * string
// |				   %e   -> system message
// |				   %x/X -> lower/upper case hexadecimal integer
// +----------------------------------------------------------------------------
void CArcTools::ThrowException( string sClassName, string sMethodName, const char *pszFmt, ... )
{
	ostringstream oss;
	char *ch, *sVal;
	va_list ap;

	va_start( ap, pszFmt );

	for ( ch = ( char * )pszFmt; *ch; ch++ )
	{
		if ( *ch != '%' )
		{
			oss << *ch;
			continue;
		}

		switch ( *++ch )
		{
			case 'd':
			{
				oss << va_arg( ap, int );
			}
			break;

			case 'f':
			{
				oss << va_arg( ap, double );
			}
			break;

			case 's':
			{
				for ( sVal = va_arg( ap, char * ); *sVal; sVal++ )
				{
					oss << *sVal;
				}
			}
			break;

			case 'e':
			{
				oss << GetSystemMessage( va_arg( ap, long ) );
			}
			break;

			case 'X':
			case 'x':
			{
				oss << uppercase << hex << va_arg( ap, int ) << dec;
			}
			break;

			default:
			{
				oss << *ch;
			}
			break;
		}
	}
	va_end( ap );

	oss << ends;

	ThrowException( sClassName, sMethodName, oss.str() );
}
