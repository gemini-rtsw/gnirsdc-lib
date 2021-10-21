#ifndef _ARC_CSTRINGLIST_H_
#define _ARC_CSTRINGLIST_H_

#ifdef WIN32
#pragma warning( disable: 4251 )
#endif

#include <vector>
#include <string>


namespace arc
{

	class CStringList
	{
		public:
			void Add( std::string const &sElem );
			void Clear();
			bool Empty();

			std::string  ToString();
			std::string& At( int dIndex );
			int Length();

			CStringList& operator<<( std::string const &sElem );
			CStringList& operator+=( CStringList& anotherList );

		private:
			std::vector<std::string> m_vList;
	};

}	// end namespace

#endif
