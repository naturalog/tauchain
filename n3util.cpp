#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <streambuf>
using namespace std;

string trim ( const string& str ) {
	static const string delims = " \t\r\n";
	size_t first = str.find_first_not_of ( delims ), last = str.find_last_not_of ( delim );
	return str.substr ( first, ( last - first + 1 ) );
}

int main() {
	std::string doc ( ( std::istreambuf_iterator<char> ( cin ) ),
	                  std::istreambuf_iterator<char>() );
	string line;
	map<string, string> prefixes;
	while ( getline ( cin, line ) ) {
		line = trim ( line );
		if ( line.startsWith ( "@prefix" ) )
		}
	char s[255], p[255], o[255];

	sscanf ( "%s %s %s .\n", s, p, o );
	[ { "@id": "c", "@graph": [{ "@id": "s", "p": {"@id": "o"} }] }]
}

