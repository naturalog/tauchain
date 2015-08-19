#ifndef __MISC_H__
#define __MISC_H__

#include <map>
#include <string>
#include <vector>
//#include <boost/bimap.hpp>
#include "strings.h"
//#include <boost/interprocess/containers/list.hpp>
//#include <boost/interprocess/containers/map.hpp>
#include "rdf.h"

#ifdef with_marpa
#define MARPA(x) x
#else
#define MARPA(x)
#endif

typedef i64 nodeid;

extern std::list<string>& proc;
class bidict {
	std::map<nodeid, node> ip;
	std::map<node, nodeid> pi;
public:
	void init();
	nodeid set ( node v );
	nodeid set ( pnode v ) { return set (*v); }
	void set ( const std::vector<node>& v );
	node operator[] ( nodeid);
	node operator[] ( u64 ) { throw std::runtime_error("called dict[] with wrong type"); }
	nodeid operator[] ( node );
	nodeid operator[] ( pnode v ) { return v ? (*this)[*v] : 0; }
	bool has ( nodeid k ) const;
	bool has ( node v ) const;
	string tostr();
	std::map<string, pnode> nodes;
};

extern bidict& dict;
extern nodeid file_contents_iri, marpa_parser_iri, marpa_parse_iri, logequalTo, lognotEqualTo, rdffirst, rdfrest, A, Dot, rdfsType, GND, rdfssubClassOf, False, rdfnil, rdfsResource, rdfsdomain;
//extern nodeid rdfList, _dlopen, _dlclose, _dlsym, _dlerror, _invoke;
string dstr ( nodeid p, bool json = false );
string maybe_shorten_uri(string s);
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
