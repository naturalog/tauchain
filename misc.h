#ifndef __MISC_H__
#define __MISC_H__

#include <map>
#include <string>
#include <vector>
//#include <boost/bimap.hpp>
#include "strings.h"
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/map.hpp>
#include "rdf.h"

extern boost::container::list<string> proc;
class bidict {
	boost::container::map<int, node> ip;
	boost::container::map<node, int> pi;
public:
	void init();
	int set ( node v );
	int set ( pnode v ) { return set (*v); }
	void set ( const std::vector<node>& v );
	node operator[] ( int k );
	int operator[] ( node v );
	int operator[] ( pnode v ) { return v ? (*this)[*v] : 0; }
	bool has ( int k ) const;
	bool has ( node v ) const;
	string tostr();
	boost::container::map<string, pnode> nodes;
};

extern bidict& dict;
extern int logequalTo, lognotEqualTo, rdffirst, rdfrest, A, rdfsResource, rdfList, Dot, GND, rdfsType, rdfssubClassOf, _dlopen, _dlclose, _dlsym, _dlerror, _invoke;
string dstr ( int p );
string indent();

struct _setproc {
	string prev;
	_setproc(const string& p);
	~_setproc();
};
#endif
