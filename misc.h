#include <map>
#include <string>
#include <vector>
using namespace std;

class bidict {
	map<int, string> m1;
	map<string, int> m2;
public:
	int set ( const string& v );
	void set ( const vector<string>& v );
	const string operator[] ( const int& k );
	int operator[] ( const string& v );
	bool has ( int k ) const;
	bool has ( const string& v ) const;
	string tostr();
};
