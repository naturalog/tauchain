#ifndef __MISC_H__
#define __MISC_H__

#include <map>
#include <string>
#include <vector>
#include "strings.h"
#include "rdf.h"

//#include <boost/interprocess/containers/list.hpp>
//#include <boost/interprocess/containers/map.hpp>
//#include <boost/bimap.hpp>

#ifdef with_marpa
#define MARPA(x) x
#else
#define MARPA(x)
#endif

enum ParsingResult {FAIL, /* INCOMPLETE, */ COMPLETE};

string indent();
using std::endl;

extern std::set<string> silence;
#ifdef DEBUG
extern bool in_silent_part;
#define TRACE(x) \
  if ((_indent + (int)proc.size() < level) && !(proc.size() > 0 && silence.find(proc.back()) != silence.end()) ) \
  { \
  	in_silent_part = false; \
  	dout << KYEL << indent() << KNRM; \
  	x; \
  } \
  else \
  { \
  	if(!in_silent_part) \
  	{ \
  		dout << "..." << endl; \
	  	in_silent_part = true; \
	} \
  }
#else
#define TRACE(X)
#endif

#define FUN setproc(__FUNCTION__);

#define MSG(x) TRACE(dout << x << std::endl);

extern std::ostream& dout;
extern std::ostream& derr;
extern bool deref, shorten;



#ifdef DEBUG
//logger _logger;
extern bool autobt, _pause;
void bt();
void dopause();
#define trace(x) std::wclog<<__FILE__<<':'<<__LINE__<<tab<<x; if (_pause) dopause()
#else
void bt();
#define trace(x)
#endif



typedef i64 nodeid;

extern std::list<string>& proc;


class bidict {
	std::map<nodeid, node> ip;
	std::map<node, nodeid> pi;
public:	
	std::map<string, pnode> nodes;

	void init();

	nodeid set ( node v );
	nodeid set ( pnode v ) { return set (*v); }
	void set ( const std::vector<node>& v );
	
	bool has ( nodeid k ) const;
	bool has ( node v ) const;

	node operator[] ( nodeid);
	node operator[] ( u64 ) { throw std::runtime_error("called dict[] with wrong type"); }
	nodeid operator[] ( node );
	nodeid operator[] ( pnode v ) { return v ? (*this)[*v] : 0; }
	
	string tostr();
};

extern bidict& dict;



//MISC?
extern nodeid file_contents_iri, marpa_parser_iri, marpa_parse_iri, logequalTo, lognotEqualTo, rdffirst, rdfrest, A, Dot, rdftype, GND, rdfnil, False;

//RDFS
extern nodeid rdfsResource, rdfsdomain, rdfsrange, rdfsClass, rdfssubClassOf, rdfssubPropertyOf, rdfsContainerMembershipProperty, rdfsmember, rdfsDatatype, rdfsLiteral, rdfProperty;
extern nodeid rdfAlt, rdfsContainer, rdfBag, rdfSeq, rdfXMLLiteral, rdfscomment;
extern nodeid rdfList, rdfsisDefinedBy, owlFunctionalProperty;
extern nodeid rdfsseeAlso, rdfslabel, rdfobject, rdfStatement, rdfpredicate, rdfsrange, rdfsubject, rdfvalue;


//, _dlopen, _dlclose, _dlsym, _dlerror, _invoke;




string dstr ( nodeid p, bool json = false );
string maybe_shorten_uri(string s);
string shorten_uri(string s);
string indent();
extern int _indent;





struct _setproc {
	string prev;
	//_setproc(const std::string& p);
	_setproc(const string& p);
	~_setproc();
};
#ifdef DEBUG
#define setproc(x) _setproc __setproc(x)
#else
#define setproc(x)
#endif





string listid();
string list_bnode_name(int item);

#endif
