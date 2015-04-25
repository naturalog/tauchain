#ifndef __JSONLD_H__
#define __JSONLD_H__

#include <algorithm>
#include <utility>
#include <memory>
#include <list>
#include <set>
//#include <boost/variant.hpp>
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
#include "rdf_data.h"
using namespace std;

namespace jsonld {

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

string resolve ( pstring, const string& );

inline pstring removeBase ( pobj o, string iri ) {
	return pstr ( "" );
}

inline pstring removeBase ( pobj o, pstring iri ) {
	return removeBase ( o, *iri );
}

inline bool equals ( const obj& a, const obj& b ) {
	return a.equals ( b );
}

inline bool equals ( const pobj& a, const pobj& b ) {
	return a->equals ( *b );
}

inline bool equals ( const pobj& a, const obj& b ) {
	return a->equals ( b );
}

inline bool equals ( const obj& a, const pobj& b ) {
	return a.equals ( *b );
}

inline pobj get ( psomap p, string k ) {
	if ( !p ) return 0;
	auto it = p->find ( k );
	return it == p->end() ? 0 : it->second;
}

// http://www.w3.org/TR/json-ld-api/#the-jsonldoptions-type
struct jsonld_options {
	jsonld_options() {
	}
	jsonld_options ( string base_ ) :
		base ( pstr ( base_ ) ) {
	}
	pstring base = 0;
	pbool compactArrays = make_shared<bool> ( true );
	pobj expandContext = 0;
	pstring processingMode = pstr ( "json-ld-1.0" );
	pbool embed = 0;
	pbool isexplicit = make_shared<bool> ( true );
	pbool omitDefault = 0;
	pbool useRdfType = make_shared<bool> ( true );
	pbool useNativeTypes = make_shared<bool> ( true );
	pbool produceGeneralizedRdf = make_shared<bool> ( true );
	pstring format = 0;
	pbool useNamespaces = make_shared<bool> ( true );
	pstring outputForm = 0;
};

inline bool keyword ( const string& key ) {
	return str_base == key || str_context == key || str_container == key
	       || str_default == key || str_embed == key || str_explicit == key
	       || str_graph == key || str_id == key || str_index == key
	       || str_lang == key || str_list == key || str_omitDefault == key
	       || str_reverse == key || str_preserve == key || str_set == key
	       || str_type == key || str_value == key || str_vocab == key;
}

inline bool keyword ( pstring key ) {
	return key ? keyword ( *key ) : false;
}

#define KW_SHORTCUTS(x) \
const string kw##x = string("@") + #x;\
template<typename T> inline bool has##x(T t) { return has(t,kw##x); } \
inline const pobj& get##x(pobj p) { return p->MAP()->at(kw##x); } \
inline const pobj& get##x(obj& p) { return p.MAP()->at(kw##x); } \
inline const pobj& get##x(psomap p) { return p->at(kw##x); } \
inline const pobj& get##x(somap p) { return p.at(kw##x); } \
inline const pobj sget##x(pobj p) { return has##x(p->MAP()) ? p->MAP()->at(kw##x) : 0; } \
inline const pobj sget##x(obj& p) { return has##x(p.MAP()) ? p.MAP()->at(kw##x) : 0; } \
inline const pobj sget##x(psomap p) { return has##x(p) ? p->at(kw##x) : 0; } \
inline const pobj sget##x(somap p) { return has##x(p) ? p.at(kw##x) : 0; }
KW_SHORTCUTS ( base );
KW_SHORTCUTS ( id );
KW_SHORTCUTS ( index );
KW_SHORTCUTS ( set );
KW_SHORTCUTS ( list );
KW_SHORTCUTS ( type );
KW_SHORTCUTS ( reverse );
KW_SHORTCUTS ( value );
KW_SHORTCUTS ( vocab );
KW_SHORTCUTS ( graph );
KW_SHORTCUTS ( context );
KW_SHORTCUTS ( none );
template<typename T> inline bool haslang ( T t ) {
	return has ( t, str_lang );
}

inline pobj& getlang ( pobj p ) {
	return p->MAP()->at ( str_lang );
}

inline pobj& getlang ( obj& p ) {
	return p.MAP()->at ( str_lang );
}

inline pobj& getlang ( psomap p ) {
	return p->at ( str_lang );
}
inline pobj& getlang ( somap p ) {
	return p.at ( str_lang );
}

inline bool keyword ( pobj p ) {
	if ( !p || !p->STR() )
		return false;
	return keyword ( *p->STR() );
}

inline bool is_abs_iri ( const string& s ) {
	return (s.find(":") != string::npos) || (s.size() && s[0] == '?');
//	return (!s.size()) ? false : ( ( s.find ( "://" ) != string::npos ) || s[0] == '?' || s[0] == '_' /* '_' taken from expandiri algo step 4.2 */ );
}

inline bool is_rel_iri ( const string& s ) {
	return ( ! ( keyword ( s ) || is_abs_iri ( s ) ) );
}

inline pobj newMap ( const string& k, pobj v ) {
	pobj r = mk_somap_obj();
	( *r->MAP() ) [k] = v;
	return r;
}

inline bool isvalue ( pobj v ) {
	return v && v->MAP() && hasvalue ( v->MAP() );
}

inline polist vec2vec ( const vector<string>& x ) {
	polist res = mk_olist();
	for ( auto t : x )
		res->push_back ( make_shared <string_obj> ( t ) );
	return res;
}

inline vector<string> vec2vec ( polist x ) {
	vector<string> res;
	for ( auto t : *x )
		res.push_back ( *t->STR() );
	return res;
}

inline void add_all ( polist l, pobj v ) {
	if ( v->LIST() )
		l->insert ( l->end(), v->LIST()->begin(), v->LIST()->end() );
	else
		l->push_back ( v );
}

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
json_spirit::mValue convert ( obj& v );
json_spirit::mValue convert ( pobj v );
pobj convert ( const json_spirit::mValue& v );

class context_t: public somap_obj {
private:
	jsonld_options options;
	psomap term_defs = make_shared<somap>();
public:
	psomap_obj inverse = 0;

	context_t ( const jsonld_options& o = jsonld_options() ) :
		somap_obj(), options ( o ) {
		if ( options.base )
			( *MAP() ) [ "@base" ] = make_shared <string_obj> ( *options.base );
	}

	pstring getContainer ( string prop ) {
		if ( prop == str_graph )
			return pstr ( str_set );
		if ( keyword ( prop ) )
			return pstr ( prop );
		auto it = term_defs->find ( prop );
		return it == term_defs->end() ? 0 : it->second->STR();
	}

	pstring getContainer ( pstring prop ) {
		return getContainer ( *prop );
	}

	typedef std::shared_ptr<context_t> pcontext;
	//Context Processing Algorithm http://json-ld.org/spec/latest/json-ld-api/#context-processing-algorithms
	pcontext parse ( pobj localContext, vector<string> remoteContexts = vector<string>() );
	void create_term_def ( const psomap context, const string term, pdefined_t pdefined );
	pstring expandIri ( const pstring value, bool relative, bool vocab, const psomap context, pdefined_t defined );

	//http://json-ld.org/spec/latest/json-ld-api/#iri-expansion
	/*	 pstring expandIri ( string value, bool relative, bool vocab, const psomap context, pdefined_t pdefined ) {
	    if ( keyword ( value ) ) return make_shared<string> ( value );
	    const defined_t& defined = *pdefined;
	    if ( context && has ( context, value ) && !defined.at ( value ) ) create_term_def ( context, value, pdefined );
	    if ( vocab && has ( term_defs, value ) ) {
	    psomap td;
	    return ( td = term_defs->at ( value )->MAP() ) ? td->at ( str_id )->STR() : 0;
	    }
	    size_t colIndex = value.find ( ":" );
	    if ( colIndex != string::npos ) {
	    string prefix = value.substr ( 0, colIndex ), suffix = value.substr ( colIndex + 1 );
	    if ( prefix == "_" || startsWith ( suffix, "//" ) ) return make_shared<string> ( value );
	    if ( context && has ( context, prefix ) && ( !has ( pdefined, prefix ) || !defined.at ( prefix ) ) )
	    create_term_def ( context, prefix, pdefined );
	    if ( has ( term_defs, prefix ) ) return pstr ( *term_defs->at ( prefix )->MAP()->at ( str_id )->STR() + suffix );
	    return make_shared<string> ( value );
	    }
	    if ( vocab && MAP()->find ( str_vocab ) != MAP()->end() ) return pstr ( *MAP()->at ( str_vocab )->STR() + value );
	    if ( relative ) {
	    auto t = sgetbase ( *this );
	    return pstr ( resolve ( t ? t->STR() : 0, value ) );
	    }
	    if ( context && is_rel_iri ( value ) ) throw INVALID_IRI_MAPPING + "not an absolute IRI: " + value;
	    return make_shared<string> ( value );
	    }
	*/
	pstring get_type_map ( const string& prop ) {
		auto td = term_defs->find ( prop );
		return td == term_defs->end() || !td->second->MAP() ?
		       0 : gettype ( td->second )->STR();
	}

	pstring get_lang_map ( const string& prop ) {
		auto td = term_defs->find ( prop );
		return td == term_defs->end() || !td->second->MAP() ?
		       0 : getlang ( td->second )->STR();
	}

	psomap get_term_def ( const string& key ) {
		return term_defs->at ( key )->MAP();
	}

	// http://json-ld.org/spec/latest/json-ld-api/#inverse-context-creation
	psomap_obj getInverse();

	int compareShortestLeast ( const string& a, const string& b ) {
		if ( a.length() < b.length() )
			return -1;
		else if ( b.length() < a.length() )
			return 1;
		return a == b ? 0 : a < b ? -1 : 1;
	}
	int compareShortestLeast ( pstring a, pstring b ) {
		return !a && !b ? 0 : a && !b ? 1 :
		       !a && b ? -1 : compareShortestLeast ( *a, *b );
	}
	int compareShortestLeast ( string a, pstring b ) {
		return !b ? 1 : compareShortestLeast ( a, *b );
	}
	int compareShortestLeast ( pstring a, string b ) {
		return !a ? -1 : compareShortestLeast ( *a, b );
	}

	// http://json-ld.org/spec/latest/json-ld-api/#term-selection
	pstring selectTerm ( string iri, vector<string>& containers,
	                     string typeLanguage, vector<string>& preferredValues );

	pstring compactIri ( string iri, bool relativeToVocab ) {
		return compactIri ( iri, 0, relativeToVocab, false );
	}
	pstring compactIri ( pstring iri, bool relativeToVocab ) {
		return !iri ? 0 : compactIri ( *iri, 0, relativeToVocab, false );
	}
	pstring compactIri ( pstring iri, pobj value, bool relativeToVocab,
	                     bool reverse ) {
		return !iri ? 0 : compactIri ( *iri, value, relativeToVocab, reverse );
	}
	// http://json-ld.org/spec/latest/json-ld-api/#iri-compaction
	pstring compactIri ( string iri, pobj value, bool relativeToVocab,
	                     bool reverse );

	// http://json-ld.org/spec/latest/json-ld-api/#value-compaction
	pobj compactValue ( string act_prop, psomap_obj value_ );

	bool isReverseProperty ( string prop ) {
		auto it = term_defs->find ( prop );
		if ( it == term_defs->end() || !it->second->MAP() )
			return false;
		auto r = it->second->MAP()->at ( str_reverse );
		return r && r->BOOL() && *r->BOOL();
	}
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

	jsonld_api ( pobj input, jsonld_options opts ) :
		jsonld_api ( opts ) {
		initialize ( input, 0 );
	}
	jsonld_api ( pobj input, pobj context, jsonld_options opts ) :
		jsonld_api ( opts ) {
		initialize ( input, 0 );
	}
	jsonld_api ( jsonld_options opts_ = jsonld_options ( "" ) ) :
		opts ( opts_ ) {
	}
private:
	void initialize ( pobj input, pobj context_ ) {
		if ( input && ( input->LIST() || input->MAP() ) )
			value = input->clone();
		context = make_shared<context_t>();
		if ( context )
			context = context->parse ( context_ );
	}
public:
	// http://json-ld.org/spec/latest/json-ld-api/#compaction-algorithm
	pobj compact ( pcontext act_ctx, string act_prop, pobj element,
	               bool compactArrays );

	pobj compact ( pcontext act_ctx, string act_prop, pobj element ) {
		return compact ( act_ctx, act_prop, element, true );
	}

	// http://json-ld.org/spec/latest/json-ld-api/#expansion-algorithm
	pobj expand ( pcontext act_ctx, pstring act_prop, pobj element );

	static bool deepCompare ( pobj v1, pobj v2, bool listOrderMatters = false );

	static bool deepContains ( polist values, pobj value ) {
		for ( pobj item : *values ) if ( deepCompare ( item, value, false ) ) return true;
		return false;
	}

	static void mergeValue ( psomap obj, pstring key, pobj value ) {
		if ( obj && key ) mergeValue ( *obj, *key, value );
	}
	static void mergeValue ( psomap obj, string key, pobj value ) {
		if ( obj ) mergeValue ( *obj, key, value );
	}
	static void mergeValue ( somap& obj, pstring key, pobj value ) {
		if ( key ) mergeValue ( obj, *key, value );
	}

	static void mergeValue ( somap& obj, string key, pobj value ) {
		auto x = obj[key];
		polist values = x ? obj[key]->LIST() : 0;
		if ( !values ) obj[key] = mk_olist_obj ( values = mk_olist() );
		if ( key == str_list || ( has ( value->MAP(), str_list ) ) || !deepContains ( values, value ) )
			values->push_back ( value );
	}

	string gen_bnode_id ( string id = "" ) {
		if ( bnode_id_map.find ( id ) != bnode_id_map.end() ) return bnode_id_map[id];
		stringstream ss;
		ss << "_:b" << ( blankNodeCounter++ );
		return bnode_id_map[id] = ss.str();
	}

	void gen_node_map ( pobj element, psomap nodeMap ) {
		gen_node_map ( element, nodeMap, str_default, pobj(), pstring(), psomap() );
	}

	void gen_node_map ( pobj element, psomap nodeMap, string activeGraph ) {
		gen_node_map ( element, nodeMap, activeGraph, pobj(), pstring(), psomap() );
	}

	void gen_node_map ( pobj element, psomap nodeMap, string activeGraph, pobj activeSubject, pstring act_prop, psomap list );

	std::shared_ptr<rdf_db> toRDF();
	std::shared_ptr<rdf_db> toRDF ( pobj input, jsonld_options options );
};

pobj expand ( pobj input, jsonld_options opts = jsonld_options() );
//prdf_db to_rdf ( jsonld_api& a, pobj o ) {
//	//	psomap node_map = make_shared<somap>();
//	//	a.gen_node_map ( o, node_map );
//	return a.toRDF();
//}
}

typedef jsonld::rdf_db rdf_db;
typedef jsonld::quad quad;
typedef jsonld::pquad pquad;
typedef jsonld::node node;
typedef jsonld::pnode pnode;
typedef jsonld::qlist qlist;
typedef jsonld::prdf_db prdf_db;

#endif
