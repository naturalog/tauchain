#ifndef __RDF_DATA__
#define __RDF_DATA__

#include "object.h"
#include <list>
#include <set>

typedef std::map<string, string> ssmap;
typedef std::shared_ptr<ssmap> pssmap;

class node;
typedef std::shared_ptr<node> pnode;
typedef std::map<string, pnode> snmap;
typedef std::shared_ptr<snmap> psnmap;

class node {
public:
	node() {}
	pstring value, datatype, lang;
	enum node_type { LITERAL, IRI, BNODE } _type;
	node ( const node_type& t ) : _type ( t ) { }
	string tostring() const;
	bool operator<(const node& x) const { return tostring() < x.tostring(); }
	bool operator==(const node& x) const { 
		bool r = *value == *x.value && *datatype == *x.datatype && *lang == *x.lang && _type == x._type;
		return r;
	}
};
inline std::wostream& operator<<(std::wostream& o, const node& n) { return o << n.tostring(); }

pnode mkliteral ( pstring value, pstring datatype, pstring language );
pnode mkiri ( pstring iri );
pnode mkbnode ( pstring attribute );

class quad {
	quad ( string subj, string pred, pnode object, string graph );
public:
	pnode subj, pred, object, graph;

	quad ( string subj, string pred, string object, string graph );
	quad ( string subj, string pred, string value, pstring datatype, pstring language, string graph );
	quad ( pnode subj, pnode pred, pnode object, string graph = L"@default" );
	quad(){}
	quad(const quad& q) : subj(q.subj), pred(q.pred), object(q.object), graph(q.graph) {}
	string tostring ( ) const;
//	bool operator<(const quad& q) const { return tostring() < q.tostring(); }
};

typedef std::shared_ptr<quad> pquad;
typedef std::list<pquad> qlist;
typedef std::shared_ptr<qlist> pqlist;
typedef std::map<string, pqlist> qdb;

extern const pnode first;
extern const pnode rest;
extern const pnode nil;

pqlist mk_qlist();

std::wostream& operator<< ( std::wostream& o, const qdb& );
std::wostream& operator<< ( std::wostream& o, const qlist& );
qdb readqdb ( std::wistream& is );

class jsonld_api;
class rdf_db: public qdb {
	ssmap context;
	jsonld_api& api;
public:
	rdf_db ( jsonld_api& );

	string tostring();

	void setNamespace ( string ns, string prefix );
	string getNamespace ( string ns );
	void clearNamespaces();
	ssmap& getNamespaces();
	somap getContext();

	void parse_ctx ( pobj contextLike );

	void graph_to_rdf ( string graph_name, somap& graph );

private:
	pnode obj_to_rdf ( pobj item );
};

typedef std::shared_ptr<rdf_db> prdf_db;
class nqparser {
private:
	wchar_t *t;
	const wchar_t *s;
	std::list<quad> r;
	typedef std::list<pnode> plist;
	int pos;
	std::list<std::tuple<pnode, pnode, pnode>> lists;
	pnode readcurly();
	pnode readlist();
	pnode readany(bool lit = true);
	pnode readiri();
	pnode readlit();
	pnode readvar();
	pnode readbnode();
	static pnode rdfrst, rdffst, rnil;
	void readprefix();
	std::list<std::pair<pnode, plist>> preds;
	std::map<string, pnode> prefixes;
public:
	nqparser();
	~nqparser();
	std::list<quad> operator()(const wchar_t *s, string ctx = L"@default");
};
qlist merge ( const qdb& q );
#endif
