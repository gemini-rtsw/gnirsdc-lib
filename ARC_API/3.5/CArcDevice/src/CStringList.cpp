// +---------------------------------------------------------------------------+
// | CStringList Class                                                         |
// +---------------------------------------------------------------------------+
// | Implements a string list.                                                 |
// +---------------------------------------------------------------------------+
#include <sstream>
#include <stdexcept>
#include "CStringList.h"

using namespace std;
using namespace arc;


// +---------------------------------------------------------------------------+
// | Add                                                                       |
// +---------------------------------------------------------------------------+
// | Adds the specified string to the back of the list.                        |
// |                                                                           |
// | <IN> -> sElem : The string to add to the end of the list.                 |
// +---------------------------------------------------------------------------+
void CStringList::Add( std::string const &sElem )
{
	 m_vList.push_back( sElem );
}


// +---------------------------------------------------------------------------+
// | Clear                                                                     |
// +---------------------------------------------------------------------------+
// | Removes all strings from the list. The list will be empty when done.      |
// +---------------------------------------------------------------------------+
void CStringList::Clear()
{
	  m_vList.clear();
}


// +---------------------------------------------------------------------------+
// | Empty                                                                     |
// +---------------------------------------------------------------------------+
// | Returns 'true' if the list is empty; 'false' otherwise.                   |
// +---------------------------------------------------------------------------+
bool CStringList::Empty()
{
	return m_vList.empty();
}


// +---------------------------------------------------------------------------+
// | ToString                                                                  |
// +---------------------------------------------------------------------------+
// | Returns the entire list as a single string. Each list element is          |
// | seperated by a newline character ( '\n' ).                                |
// +---------------------------------------------------------------------------+
std::string CStringList::ToString()
{
	std::vector<std::string>::iterator it;
	std::string sTemp;

	for ( it = m_vList.begin() ; it != m_vList.end() ; it++ )
	{
		sTemp += std::string( *it );
		sTemp += std::string( "\n" );
	}

	return sTemp;
}


// +---------------------------------------------------------------------------+
// | At                                                                        |
// +---------------------------------------------------------------------------+
// | Returns the string at the specified list index.                           |
// |                                                                           |
// | Throws std::runtime_error if index is out of range.                       |
// |                                                                           |
// | <IN> -> dIndex : The element to be returned.                              |
// +---------------------------------------------------------------------------+
std::string& CStringList::At( int dIndex )
{
	if ( dIndex >= static_cast<int>( m_vList.size() ) )
	{
		ostringstream oss;

		oss << "( CStringList::At() ): The index [ "
			<< dIndex << " ] is out of range [ 0 - "
			<< m_vList.size() << " ]!" << ends;

		throw std::runtime_error( oss.str() );
	}

	return m_vList.at( dIndex );
}


// +---------------------------------------------------------------------------+
// | Length                                                                    |
// +---------------------------------------------------------------------------+
// | Returns the number of elements in the list.                               |
// +---------------------------------------------------------------------------+
int CStringList::Length()
{
	return int( m_vList.size() );
}


// +---------------------------------------------------------------------------+
// | operator <<                                                               |
// +---------------------------------------------------------------------------+
// | Operator to add an element to the back of the list.                       |
// |                                                                           |
// | <IN> -> sElem : The element to be added to the back of the list.          |
// +---------------------------------------------------------------------------+
CStringList& CStringList::operator<<( std::string const &sElem )
{
	m_vList.push_back( sElem );

	return *this;
}


// +---------------------------------------------------------------------------+
// | operator +=                                                               |
// +---------------------------------------------------------------------------+
// | Operator to add two lists together.                                       |
// |                                                                           |
// | <IN> -> anotherList : The list to add to the end of this list.            |
// +---------------------------------------------------------------------------+
CStringList& CStringList::operator+=( CStringList& anotherList )
{
	for ( int i=0; i<anotherList.Length(); i++ )
	{
		m_vList.push_back( anotherList.At( i ) );
	}

	return *this;
}
