#ifndef __MISC_H__
#define __MISC_H__

#include <map>
#include <string>
#include <vector>
#include <boost/bimap.hpp>
#include "strings.h"
#include <boost/interprocess/containers/list.hpp>
//using namespace std;

extern boost::container::list<string> proc;
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
extern int logequalTo, lognotEqualTo, rdffirst, rdfrest, A, rdfsResource, rdfList, Dot, GND, rdfsType, rdfssubClassOf;
/*
template<typename T> string print ( T t ) {
	wstringstream ss;
	ss << t;
	return ss.str();
}*/
string dstr ( int p );
string indent();
struct _setproc {
	string prev;
	_setproc(const string& p);
	~_setproc();
};
#endif
