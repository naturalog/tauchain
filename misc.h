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

typedef i64 resid;

extern std::list<string> proc;
class bidict {
	std::map<resid, node> ip;
	std::map<node, resid> pi;
public:
	void init();
	resid set ( node v );
	resid set ( pnode v ) { return set (*v); }
	void set ( const std::vector<node>& v );
	node operator[] ( resid );
	node operator[] ( u64 ) { throw std::runtime_error("called dict[] with wrong type"); }
	resid operator[] ( node );
	resid operator[] ( pnode v ) { return v ? (*this)[*v] : 0; }
	bool has ( resid k ) const;
	bool has ( node v ) const;
	string tostr();
	std::map<string, pnode> nodes;
};

extern bidict& dict;
extern resid file_contents_iri, marpa_parser_iri, marpa_parse_iri, logequalTo, lognotEqualTo, rdffirst, rdfrest, A, rdfsResource, rdfList, Dot, GND, rdfsType, rdfssubClassOf, _dlopen, _dlclose, _dlsym, _dlerror, _invoke, rdfnil, False;
string dstr ( resid p, bool json = false );
string indent();

struct _setproc {
	string prev;
	_setproc(const string& p);
	~_setproc();
};
#ifdef DEBUG
#define setproc(x) _setproc __setproc(x)
#else
#define setproc(x)
#endif

extern int _indent;
#endif
