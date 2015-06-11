#ifndef __JSONLD_H__
#define __JSONLD_H__

#include <algorithm>
#include <utility>
#include <memory>
#include <list>
#include <set>
#include "json_spirit.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/easy.h>
#include <curl/curlbuild.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "object.h"
#include "strings.h"
#include "rdf.h"

inline string resolve ( pstring base_, const string& ref ) {
	return base_ ? *base_ + ref : ref;
}

template<typename M, typename K, typename E> inline bool throw_if_contains (
    const M& m, const K& k, const E& err ) {
	if ( m.find ( k ) == m.end() )
		throw err;
	return true;
}
template<typename M, typename K, typename E> inline bool throw_if_not_contains (
    const M& m, const K& k, const E& err ) {
	if ( m.find ( k ) != m.end() )
		throw err;
	return true;
}

pstring removeBase ( pobj, string );
pstring removeBase ( pobj o, pstring iri );
bool equals ( const obj& a, const obj& b );
bool equals ( const pobj& a, const pobj& b );
bool equals ( const pobj& a, const obj& b );
bool equals ( const obj& a, const pobj& b );
pobj get ( psomap p, string k );
json_spirit::wmValue convert ( obj& v );
json_spirit::wmValue convert ( pobj v );
bool keyword ( const string& key );
bool keyword ( pstring key );

#define KW_SHORTCUTS(x) \
const string kw##x = string(L"@") + ws(#x);\
template<typename T> inline bool has##x(T t) { return 	has(t, kw##x); } \
inline const pobj& get##x(pobj p) { return 		p->MAP()->at(kw##x); } \
inline const pobj& get##x(obj& p) { return		p.MAP()->at(kw##x); } \
inline const pobj& get##x(psomap p) { return 		p->at(kw##x); } \
inline const pobj& get##x(somap p) { return 		p.at(kw##x); } \
inline const pobj sget##x(pobj p) { return 		has##x(p->MAP()) ? p->MAP()->at(kw##x) : 0; } \
inline const pobj sget##x(obj& p) { return 		has##x(p.MAP()) ? p.MAP()->at(kw##x) : 0; } \
inline const pobj sget##x(psomap p) { return 		has##x(p) ? p->at(kw##x) : 0; } \
inline const pobj sget##x(somap p) { return 		has##x(p) ? p.at(kw##x) : 0; }
KW_SHORTCUTS ( base )
KW_SHORTCUTS ( id )
KW_SHORTCUTS ( index )
KW_SHORTCUTS ( set )
KW_SHORTCUTS ( list )
KW_SHORTCUTS ( type )
KW_SHORTCUTS ( reverse )
KW_SHORTCUTS ( value )
KW_SHORTCUTS ( vocab )
KW_SHORTCUTS ( graph )
KW_SHORTCUTS ( context )
KW_SHORTCUTS ( none )

template<typename T> inline bool haslang ( T t ) {
	return has ( t, str_lang );
}

pobj& getlang ( pobj p );
pobj& getlang ( obj& p );
pobj& getlang ( psomap p );
pobj& getlang ( somap p );
bool keyword ( pobj p );
bool is_abs_iri ( const string& s );
bool is_rel_iri ( const string& s );
inline bool is_rel_iri ( pstring s ) { return is_rel_iri(*s); }
pobj newMap ( const string& k, pobj v );
bool isvalue ( pobj v );
polist vec2vec ( const std::vector<string>& x );
std::vector<string> vec2vec ( polist x );
void add_all ( polist l, pobj v );

struct remote_doc_t {
	string url;
	pobj document;
	pstring context_url;
	remote_doc_t ( string url_, const pobj& document_ = 0, pstring context = 0 ) :
		url ( url_ ), document ( document_ ), context_url ( context ) {
	}
};

extern void* curl;

size_t write_data ( void *ptr, size_t size, size_t n, void *stream );
string download ( const string& url );
pobj fromURL ( const string& url );
remote_doc_t load ( const string& url );
pobj convert ( const json_spirit::wmValue& v );

class context_t: public somap_obj {
private:
	jsonld_options options;
	psomap term_defs = make_shared<somap>();
public:
	psomap_obj inverse = 0;

	context_t ( const jsonld_options& o = jsonld_options() );
	pstring getContainer ( string prop );
	pstring getContainer ( pstring prop );

	typedef std::shared_ptr<context_t> pcontext;
	//Context Processing Algorithm http://json-ld.org/spec/latest/json-ld-api/#context-processing-algorithms
	pcontext parse ( pobj localContext, std::vector<string> remoteContexts = std::vector<string>() );
	void create_term_def ( const psomap context, const string term, pdefined_t pdefined );
	pstring expand_iri ( const pstring value, bool relative, bool vocab, const psomap context, pdefined_t defined );

	pstring get_type_map ( const string& prop );
	pstring get_lang_map ( const string& prop );
	psomap get_term_def ( const string& key );

	// http://json-ld.org/spec/latest/json-ld-api/#inverse-context-creation
	psomap_obj getInverse();
	int compareShortestLeast ( const string& a, const string& b );
	int compareShortestLeast ( pstring a, pstring b );
	int compareShortestLeast ( string a, pstring b );
	int compareShortestLeast ( pstring a, string b );
	// http://json-ld.org/spec/latest/json-ld-api/#term-selection
	pstring selectTerm ( string iri, std::vector<string>& containers, string typeLanguage, std::vector<string>& preferredValues );
	pstring compactIri ( string iri, bool relativeToVocab );
	pstring compactIri ( pstring iri, bool relativeToVocab );
	pstring compactIri ( pstring iri, pobj value, bool relativeToVocab, bool reverse );
	// http://json-ld.org/spec/latest/json-ld-api/#iri-compaction
	pstring compactIri ( string iri, pobj value, bool relativeToVocab, bool reverse );
	// http://json-ld.org/spec/latest/json-ld-api/#value-compaction
	pobj compactValue ( string act_prop, psomap_obj value_ );
	bool isReverseProperty ( string prop );
	map<string, string> getPrefixes ( bool onlyCommonPrefixes );
	pobj expandValue ( pstring act_prop, pobj value );
};

typedef std::shared_ptr<context_t> pcontext;

class rdf_db;
class jsonld_api {
public:
	jsonld_options opts;
	pobj value = 0;
	pcontext context = 0;
	static size_t blankNodeCounter;
	static map<string, string> bnode_id_map;

	jsonld_api ( pobj input, jsonld_options opts );
	jsonld_api ( jsonld_options opts_ = jsonld_options ( L"" ) ) :
		opts ( opts_ ) {
	}
private:
	void initialize ( pobj input, pobj context_ );
	static void mergeValue ( psomap obj, pstring key, pobj value );
	static void mergeValue ( psomap obj, string key, pobj value );
	static void mergeValue ( somap& obj, pstring key, pobj value );
	static void mergeValue ( somap& obj, string key, pobj value );
public:
	pobj compact ( pcontext act_ctx, string act_prop, pobj element, bool compactArrays );
	pobj compact ( pcontext act_ctx, string act_prop, pobj element );
	pobj expand ( pcontext act_ctx, pstring act_prop, pobj element );
	static bool deepCompare ( pobj v1, pobj v2, bool listOrderMatters = false );
	static bool deepContains ( polist values, pobj value );
	static pstring gen_bnode_id ( string id = L"" );
	void gen_node_map ( pobj element, psomap nodeMap );
	void gen_node_map ( pobj element, psomap nodeMap, string activeGraph );
	void gen_node_map ( pobj element, psomap nodeMap, string activeGraph, pobj activeSubject, pstring act_prop, psomap list );

	std::shared_ptr<rdf_db> toRDF();
	std::shared_ptr<rdf_db> toRDF ( pobj input, jsonld_options options );
};

pobj expand ( pobj input, jsonld_options opts = jsonld_options() );

#endif
