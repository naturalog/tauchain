#ifndef __MISC_H__
#define __MISC_H__

#include <map>
#include <string>
#include <vector>
#include <boost/bimap.hpp>
#include "strings.h"
//using namespace std;

class bidict {
	//	map<int, string> m1;
	//	map<string, int> m2;
	typedef boost::bimap<int, string> bm;
	bm m;
public:
	bidict();
	int set ( const string& v );
	void set ( const std::vector<string>& v );
	const string operator[] ( int k );
	int operator[] ( const string& v );
	bool has ( int k ) const;
	bool has ( const string& v ) const;
	string tostr();
};

extern bidict& dict;
extern int logequalTo, lognotEqualTo, rdffirst, rdfrest, A, rdfsResource, rdfList, Dot, GND, rdfsType;

template<typename T> string print ( T t ) {
	stringstream ss;
	ss << t;
	return ss.str();
}
string dstr ( int p );
#endif
