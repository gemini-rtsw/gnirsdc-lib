// +----------------------------------------------------------------------+
// | Log.h : Defines a string message logging class                       |
// +----------------------------------------------------------------------+

#ifndef _ARC_CLOG_H_
#define _ARC_CLOG_H_

#ifdef WIN32
#pragma warning( disable: 4251 )
#endif

#include <queue>
#include <string>
#include <sstream>
#include <cstdarg>
#include "DllMain.h"


namespace arc
{
	class CARCDEVICE_API CLog
	{
	public:
		void  SetMaxSize( int dSize );

		void  Put( const char *szFmt, ... );

		const std::string	GetNext();
		const std::string	GetLast();
		int					GetLogCount();
		bool				Empty();

		void SelfTest();

	private:
		static std::queue<std::string>::size_type Q_MAX;

		std::queue<std::string> m_sQueue;
	};

}	// end namespace


#endif
