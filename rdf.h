#ifndef __RDF_DATA__
#define __RDF_DATA__

namespace jsonld {

typedef map<string, string> ssmap;
typedef std::shared_ptr<ssmap> pssmap;

class node;
typedef std::shared_ptr<node> pnode;
typedef map<string, pnode> snmap;
typedef std::shared_ptr<snmap> psnmap;

class node {
public:
	string type, value, datatype, lang;
	enum node_type { LITERAL, IRI, BNODE } _type;
	node ( const node_type& t ) : _type ( t ) { }
	string tostring();
};

pnode mkliteral ( string value, pstring datatype, pstring language );
pnode mkiri ( string iri );
pnode mkbnode ( string attribute );

typedef std::tuple<pnode, pnode, pnode, pnode> quad_base;

class quad: public quad_base {
	quad ( string subj, string pred, pnode object, string graph );
public:
	pnode &subj = std::get <0> ( *this ), &pred = std::get <1> ( *this ),
	       &object = std::get <2> ( *this ), &graph = std::get <3> ( *this );

	quad ( string subj, string pred, string object, string graph );
	quad ( string subj, string pred, string value, pstring datatype, pstring language, string graph );
	quad ( pnode subj, pnode pred, pnode object, string graph );
	using quad_base::quad_base;

	string tostring ( );
};

typedef std::shared_ptr<quad> pquad;
typedef list<pquad> qlist;
typedef std::shared_ptr<qlist> pqlist;
typedef map<string, pqlist> qdb;

extern const pnode first;
extern const pnode rest;
extern const pnode nil;

pqlist mk_qlist();

ostream& operator<< ( ostream& o, const qdb& );

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
}
jsonld::qlist merge ( const jsonld::qdb& q );
#endif
