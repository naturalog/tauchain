#ifndef __RDF_H__
#define __RDF_H__

#include <list>
#include <set>
#include <map>
#include "strings.h"



#include "json_object.h"



typedef std::map<string, string> ssmap;
typedef std::shared_ptr<ssmap> pssmap;


class node;
typedef std::shared_ptr<node> pnode;
typedef std::map<string, pnode> snmap;
typedef std::shared_ptr<snmap> psnmap;

class node {
public:
	//Definition
	//value = "lexical form"
	//datatype = "datatype IRI"
	//lang = "language tag"

	//a literal will have: 			value, datatype
	//a language-tagged string will have: 	value, datatype, lang
	pstring value, datatype, lang;	

	pnode next = 0;
	
	//The three types of RDF nodes:
	enum node_type { LITERAL, IRI, BNODE } _type;
	
	

	//Constructors
	//Empty node
	node() {}	
	node ( const node_type& t ) : _type ( t ) { }
	
	
	//Serializer
	string tostring() const;


	//Comparison operators
	bool operator<(const node& x) const { return tostring() < x.tostring(); }
	bool operator==(const node& x) const { 
		return tostring()==x.tostring();
		//*value == *x.value && *datatype == *x.datatype && *lang == *x.lang && _type == x._type;
	}
};


//node:: Stream serializer
inline std::ostream& operator<<(std::ostream& o, const node& n) { return o << n.tostring(); }




//Specialized node constructors
/*
LITERAL

A literal in an RDF graph consists of two or three elements:
*a lexical form, being a Unicode [UNICODE] string, which SHOULD be in Normal Form C [NFC],

*a datatype IRI, being an IRI identifying a datatype that determines how the lexical form maps to a literal value, and

*if and only if the datatype IRI is http://www.w3.org/1999/02/22-rdf-syntax-ns#langString, a non-empty language tag as defined by [BCP47]. The language tag MUST be well-formed according to section 2.2.9 of [BCP47].

*/
pnode mkliteral ( pstring value, pstring datatype, pstring language );


/*
IRI
An IRI (Internationalized Resource Identifier) within an RDF graph is a Unicode string [UNICODE] that conforms to the syntax defined in RFC 3987 [RFC3987].

IRIs in the RDF abstract syntax MUST be absolute, and MAY contain a fragment identifier.

IRI equality: Two IRIs are equal if and only if they are equivalent under Simple String Comparison according to section 5.1 of [RFC3987]. Further normalization MUST NOT be performed when comparing IRIs for equality.
*/
pnode mkiri ( pstring iri );

/*
BLANK NODE

Blank nodes are disjoint from IRIs and literals. Otherwise, the set of possible blank nodes is arbitrary. RDF makes no reference to any internal structure of blank nodes.
*/
pnode mkbnode ( pstring attribute );






class quad {
	quad ( string subj, string pred, pnode object, string graph );

public:
	//Definition
	pnode subj, pred, object, graph;



	//Constructors
	//empty quad
	//*the* empty quad
	quad(){}

	//@default graph?
	quad ( string subj, string pred, string object, string graph );
	quad ( string subj, string pred, string value, pstring datatype, pstring language, string graph );
	quad ( pnode subj, pnode pred, pnode object, string graph = "@default" );
	//default graph?
	quad ( pnode subj, pnode pred, pnode object, pnode graph);

	//makes a quad from another quad?
	quad(const quad& q) : subj(q.subj), pred(q.pred), object(q.object), graph(q.graph) {}


	//Serializer
	string tostring ( ) const;

	//No comparison operators though
};



//typedef std::shared_ptr<quad> p_quad;
//typedef std::list<pquad> lp_quad;
//typedef std::shared_ptr<lp_quad> plp_quad;

//typedef std::list<pnode> qdbList;

//typedef string contextName;
//typedef string listBNodeName;

//typedef std::map<contextName, plp_quad> qdbQuads;
//typedef std::map<listBNodeName, qdbList> qdbLists;

//typedef std::pair<qdb_quads,qdb_lists> qdb;

typedef std::shared_ptr<quad> pquad;
typedef std::list<pquad> qlist;
typedef std::shared_ptr<qlist> pqlist;

//First item in the pair holds the non-list quads
//Second item in the pair holds the lists
//std::map<string,pqlist> maps contexts to their quads.
typedef std::pair<std::map<string, pqlist>, std::map<string, std::list<pnode>>> qdb;




qdb merge_qdbs(const std::vector<qdb> qdbs);

extern const pnode first;
extern const pnode rest;
extern const pnode nil;

pqlist mk_qlist();

std::ostream& operator<< ( std::ostream& o, const qdb& );
std::ostream& operator<< ( std::ostream& o, const qlist& );
#ifdef JSON
class jsonld_api;
class rdf_db: public qdb {
	ssmap context;
	jsonld_api& api;
public:
	using qdb::first;
	using qdb::second;
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
#endif

//why isnt this in its own header?
#ifndef NOPARSER
class nqparser {
private:
	typedef std::list<pnode> plist;

	char *t;
	const char *s;
	std::list<quad> r;
	int pos;
	
	std::map<string, pnode> prefixes;
	std::map<string, std::list<pnode>> qlists;
	std::list<std::tuple<pnode, pnode, pnode>> lists;

	pnode readcurly(qdb &kb);
	pnode readlist(qdb &kb);
	pnode readany(qdb &kb, bool lit = true);
	pnode readiri();
	pnode readlit();
	pnode readvar();
	pnode readbnode();
	void readprefix();
	void preprocess(std::istream& is, std::stringstream& ss);
public:
	nqparser();
	~nqparser();
	
	void parse(qdb& kb, std::istream& is);
	void _parse(qdb& kb, string ctx);
};


#endif

qlist merge ( const qdb& q );





bool nodes_same(pnode x, qdb &a, pnode y, qdb &b);
bool qdbs_equal(qdb &a, qdb &b) ;
qdb merge_qdbs(const std::vector<qdb> qdbs);




#endif

