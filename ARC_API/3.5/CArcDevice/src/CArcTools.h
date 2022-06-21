#ifndef _ARC_CARCTOOLS_H_
#define _ARC_CARCTOOLS_H_

#include <string>
#include <sstream>
#include <iterator>
#include "DllMain.h"



namespace arc
{

	class CARCDEVICE_API CArcTools
	{
		public:
			static std::string  ReplyToString( int dReply );
			static std::string  CmdToString( int dCmd );

			static std::string  CmdToString( int dReply, int dBoardId, int dCmd, int dArg1 = -1,
											 int dArg2 = -1, int dArg3 = -1, int dArg4 = -1,
											 int dSysErr = -1 );

			static int StringToCmd( std::string sCmd );

			static std::string  FormatString( const char *szFmt, ... );
			static const std::string StringToUpper( std::string sStr );

			static std::string  GetSystemMessage( int dCode );
			static std::string  ConvertIntToString( int dNumber );
			static std::string  ConvertWideToAnsi( wchar_t wcharString[] );
			static std::string  ConvertWideToAnsi( const std::wstring& wsString );
			static std::wstring ConvertAnsiToWide( const char *szString );

			static long StringToHex( std::string sStr );
			static char StringToChar( std::string sStr );

			static void ThrowException( std::string sClassName, std::string sMethodName, std::string sMsg );
			static void ThrowException( std::string sClassName, std::string sMethodName, const char *szFmt, ... );


			//
			// String tokenizer that uses stringstream instead of strtok
			//
			class CARCDEVICE_API CTokenizer
			{
				public:
					CTokenizer( void ) { ss = NULL; }

					~CTokenizer( void ) { if ( ss != NULL ) delete ss; }

					void Victim( std::string str )
					{
						if ( ss != NULL ) { delete ss; }
						ss = new std::stringstream( str );
					}

					std::string Next()
					{
						if ( IsEmpty() )
						{
							ThrowException( "CArcTools::CTokenizer", "Next", "No more strings!" );
						}

						std::string sstr;
						*ss >> sstr;
						return sstr;
					}

					bool IsEmpty()
					{
						return ss->eof();
					}

				private:
					std::stringstream *ss;
			};
	};

}	// end namespace


#endif
