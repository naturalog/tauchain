#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <utility>
#include <memory>
#include <list>
#include <set>
#include <boost/variant.hpp>
#include "json_spirit.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/easy.h>
#include <curl/curlbuild.h>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;
using namespace std::string_literals;
using namespace boost;



namespace jsonld {

const string LOADING_DOCUMENT_FAILED = "loading document failed";
const string LIST_OF_LISTS = "list of lists";
const string INVALID_INDEX_VALUE = "invalid @index value";
const string CONFLICTING_INDEXES = "conflicting indexes";
const string INVALID_ID_VALUE = "invalid @id value";
const string INVALID_LOCAL_CONTEXT = "invalid local context";
const string MULTIPLE_CONTEXT_LINK_HEADERS = "multiple context link headers";
const string LOADING_REMOTE_CONTEXT_FAILED = "loading remote context failed";
const string INVALID_REMOTE_CONTEXT = "invalid remote context";
const string RECURSIVE_CONTEXT_INCLUSION = "recursive context inclusion";
const string INVALID_BASE_IRI = "invalid base IRI";
const string INVALID_VOCAB_MAPPING = "invalid vocab mapping";
const string INVALID_DEFAULT_LANGUAGE = "invalid default language";
const string KEYWORD_REDEFINITION = "keyword redefinition";
const string INVALID_TERM_DEFINITION = "invalid term definition";
const string INVALID_REVERSE_PROPERTY = "invalid reverse property";
const string INVALID_IRI_MAPPING = "invalid IRI mapping";
const string CYCLIC_IRI_MAPPING = "cyclic IRI mapping";
const string INVALID_KEYWORD_ALIAS = "invalid keyword alias";
const string INVALID_TYPE_MAPPING = "invalid type mapping";
const string INVALID_LANGUAGE_MAPPING = "invalid language mapping";
const string COLLIDING_KEYWORDS = "colliding keywords";
const string INVALID_CONTAINER_MAPPING = "invalid container mapping";
const string INVALID_TYPE_VALUE = "invalid type value";
const string INVALID_VALUE_OBJECT = "invalid value object";
const string INVALID_VALUE_OBJECT_VALUE = "invalid value object value";
const string INVALID_LANGUAGE_TAGGED_STRING = "invalid language-tagged const String";
const string INVALID_LANGUAGE_TAGGED_VALUE = "invalid language-tagged value";
const string INVALID_TYPED_VALUE = "invalid typed value";
const string INVALID_SET_OR_LIST_OBJECT = "invalid set or list object";
const string INVALID_LANGUAGE_MAP_VALUE = "invalid language map value";
const string COMPACTION_TO_LIST_OF_LISTS = "compaction to list of lists";
const string INVALID_REVERSE_PROPERTY_MAP = "invalid reverse property map";
const string INVALID_REVERSE_VALUE = "invalid @reverse value";
const string INVALID_REVERSE_PROPERTY_VALUE = "invalid reverse property value";
const string SYNTAX_ERROR = "syntax error";
const string NOT_IMPLEMENTED = "not implemnted";
const string UNKNOWN_FORMAT = "unknown format";
const string INVALID_INPUT = "invalid input";
const string PARSE_ERROR = "parse error";
const string UNKNOWN_ERROR = "unknown error";

template<typename M, typename K, typename E> inline bool throw_if_contains ( const M& m, const K& k, const E& err ) {
	if ( m.find ( k ) == m.end() ) throw err;
	return true;
}
template<typename M, typename K, typename E> inline bool throw_if_not_contains ( const M& m, const K& k, const E& err ) {
	if ( m.find ( k ) != m.end() ) throw err;
	return true;
}

template<typename T>
inline bool is ( const T& s, const std::vector<T>& v, std::string exception = std::string() ) {
	bool rc = std::find ( v.begin(), v.end(), s ) != v.end();
	if ( exception.size() && !rc ) throw exception;
	return rc;
}

template<typename T> inline bool is ( const std::shared_ptr<T>& s, const std::vector<T>& v, std::string exception = std::string() ) {
	return is<T> ( *s, v, exception );
}


inline string lower ( const string& s_ ) {
	string s = s_;
	std::transform ( s.begin(), s.end(), s.begin(), ::tolower );
	return s;
}

inline bool endsWith ( const string& x, const string& y ) {
	return x.size() >= y.size() && x.substr ( x.size() - y.size(), y.size() ) == y;
}
inline bool startsWith ( const string& x, const string& y ) {
	return x.size() >= y.size() && x.substr ( 0, y.size() ) == y;
}

template<typename charT>
inline vector<string> split ( const string& s, charT c ) {
	vector<string> v;
	for ( string::size_type i = 0,  j = s.find ( c ); j != string::npos; ) {
		v.push_back ( s.substr ( i, j - i ) );
		j = s.find ( c, i = ++j );
		if ( j == string::npos ) v.push_back ( s.substr ( i, s.length() ) );
	}
	return v;
}

template<typename C, typename K> bool has ( const C& c, const K& k ) {
	return /*std::find(c.begin(), c.end(), k)*/c.find ( k ) != c.end();
}
template<typename C, typename K> bool has ( std::shared_ptr<C> c, const K& k ) {
	return c && has<C, K> ( *c, k );
}
template<typename C, typename K> bool has ( std::shared_ptr<C> c, std::shared_ptr<K> k ) {
	return k && has<C, K> ( c, *k );
}

template<typename T> string tostr ( T t ) {
	stringstream s;
	s << t;
	return s.str();
}


typedef nullptr_t null;

class obj {
protected:
	obj() {}
public:
	typedef std::shared_ptr<obj> pobj;
	typedef map<string, pobj> somap;
	typedef vector<pobj> olist;
	typedef std::shared_ptr<somap> psomap;

	virtual std::shared_ptr<uint64_t> UINT() {
		return 0;
	}
	virtual std::shared_ptr<int64_t> INT() {
		return 0;
	}
	virtual std::shared_ptr<bool> BOOL() {
		return 0;
	}
	virtual std::shared_ptr<string> STR() {
		return 0;
	}
	virtual std::shared_ptr<somap> MAP() {
		return 0;
	}
	virtual std::shared_ptr<olist> LIST() {
		return 0;
	}
	virtual std::shared_ptr<double> DOUBLE() {
		return 0;
	}
	virtual std::shared_ptr<null> Null() {
		return 0;
	}

	bool map_and_has ( const string& k ) {
		psomap m = MAP();
		return m && m->find ( k ) != m->end();
	}
	bool map_and_has_null ( const string& k ) {
		psomap m = MAP();
		if ( !m ) return false;
		auto it = m->find ( k );
		return it != m->end() && it->second->Null();
	}
	virtual string type_str() const = 0;
	virtual bool equals ( const obj& o ) const = 0;
	virtual pobj clone() const = 0;
	string toString() {
		stringstream ss;
		if ( MAP() ) for ( auto x : *MAP() ) ss << x.first << ',' << x.second->toString() << endl;
		else if ( LIST() ) for ( auto x : *LIST() ) ss << x << endl;
		else if ( BOOL() ) ss << *BOOL();
		else if ( DOUBLE() ) ss << *DOUBLE();
		else if ( Null() ) ss << "(null)";
		else if ( STR() ) ss << *STR();
		else if ( INT() ) ss << *INT();
		else if ( UINT() ) ss << *UINT();
		return ss.str();
	}
};

#define OBJ_IMPL(type, getter) \
class type##_obj : public obj { \
	std::shared_ptr<type> data; \
public: \
	type##_obj(const type& o = type()) { data = make_shared<type>(); *data = o; } \
	type##_obj(const std::shared_ptr<type> o) : data(o) { } \
	virtual std::shared_ptr<type> getter() { return data; } \
	virtual string type_str() const { return #type; } \
	virtual bool equals(const obj& o) const { \
		if ( type_str() != o.type_str() ) return false; \
		auto od = ((const type##_obj&)o).data; \
		if ( !data || !od) return data == od; \
		return *data == *od; \
	}\
	virtual pobj clone() const { return make_shared<type##_obj>(*data);  }\
};typedef std::shared_ptr<type##_obj> p##type##_obj

OBJ_IMPL ( int64_t, INT );
OBJ_IMPL ( uint64_t, UINT );
OBJ_IMPL ( bool, BOOL );
OBJ_IMPL ( double, DOUBLE );
OBJ_IMPL ( string, STR );
OBJ_IMPL ( somap, MAP );
OBJ_IMPL ( olist, LIST );
OBJ_IMPL ( null, Null );


typedef obj::pobj pobj;
typedef obj::somap somap;
typedef obj::olist olist;
typedef std::shared_ptr<string> pstring;
typedef std::shared_ptr<somap> psomap;
typedef std::shared_ptr<olist> polist;
typedef std::shared_ptr<bool> pbool;
typedef map<string, bool> defined_t;
typedef std::shared_ptr<defined_t> pdefined_t;

template<typename T> pstring_obj mk_str_obj ( T t ) {
	return make_shared<string_obj> ( t );
}
pstring_obj mk_str_obj() {
	return make_shared<string_obj>();
}

template<typename T> psomap_obj mk_somap_obj ( T t ) {
	return make_shared<somap_obj> ( t );
}
psomap_obj mk_somap_obj() {
	return make_shared<somap_obj>();
}
template<typename T> polist_obj mk_olist_obj ( T t ) {
	return make_shared<olist_obj> ( t );
}
polist_obj mk_olist_obj() {
	return make_shared<olist_obj>();
}
template<typename T> polist mk_olist ( T t ) {
	return make_shared<olist> ( t );
}
polist mk_olist() {
	return make_shared<olist>();
}

string resolve ( const string&, const string& ) {
	return "";
}

inline pstring pstr ( const string& s ) {
	return make_shared<string> ( s );
}

pstring removeBase ( pobj o, string iri ) {
	return pstr ( "" );
}
pstring removeBase ( pobj o, pstring iri ) {
	return removeBase ( o, *iri );
}

bool equals ( const obj& a, const obj& b ) {
	return a.equals ( b );
}
bool equals ( const pobj& a, const pobj& b ) {
	return a->equals ( *b );
}
bool equals ( const pobj& a, const obj& b ) {
	return a->equals ( b );
}
bool equals ( const obj& a, const pobj& b ) {
	return a.equals ( *b );
}
pobj get ( psomap p, string k ) {
	if ( !p ) return 0;
	auto it = p->find ( k );
	if ( it == p->end() ) return 0;
	return it->second;
}

// http://www.w3.org/TR/json-ld-api/#the-jsonldoptions-type
struct jsonld_options {
	jsonld_options() {}
	jsonld_options ( string base_ ) : base ( pstr ( base_ ) ) {}
	pstring base = 0;
	pbool compactArrays = make_shared<bool> ( true );
	//obj expandContext = 0;
	pstring processingMode = pstr ( "json-ld-1.0" );
	pbool embed = 0;
	pbool isexplicit = 0;
	pbool omitDefault = 0;
	pbool useRdfType = make_shared<bool> ( false );
	pbool useNativeTypes = make_shared<bool> ( false );
	pbool produceGeneralizedRdf = make_shared<bool> ( false );
	pstring format = 0;
	pbool useNamespaces = make_shared<bool> ( false );
	pstring outputForm = 0;
};

bool keyword ( const string& key ) {
	return "@base"s == key || "@context"s == key || "@container"s == key
	       || "@default"s == key || "@embed"s == key || "@explicit"s == key
	       || "@graph"s == key || "@id"s == key || "@index"s == key
	       || "@language"s == key || "@list"s == key || "@omitDefault"s == key
	       || "@reverse"s == key || "@preserve"s == key || "@set"s == key
	       || "@type"s == key || "@value"s == key || "@vocab"s == key;
}

#define KW_SHORTCUTS(x) \
const string kw##x = "@"s + #x;\
template<typename T> bool has##x(T t) { return has(t,kw##x); } \
const pobj& get##x(pobj p) { return p->MAP()->at(kw##x); } \
const pobj& get##x(obj& p) { return p.MAP()->at(kw##x); } \
const pobj& get##x(psomap p) { return p->at(kw##x); } \
const pobj& get##x(somap p) { return p.at(kw##x); }
KW_SHORTCUTS ( base );
KW_SHORTCUTS ( id );
KW_SHORTCUTS ( index );
KW_SHORTCUTS ( set );
KW_SHORTCUTS ( list );
KW_SHORTCUTS ( type );
KW_SHORTCUTS ( reverse );
KW_SHORTCUTS ( value );
KW_SHORTCUTS ( vocab );
KW_SHORTCUTS ( none );
template<typename T> bool haslang ( T t ) {
	return has ( t, "@language" );
}
pobj& getlang ( pobj p ) {
	return p->MAP()->at ( "@language" );
}
pobj& getlang ( obj& p ) {
	return p.MAP()->at ( "@language" );
}
pobj& getlang ( psomap p ) {
	return p->at ( "@language" );
}
pobj& getlang ( somap p ) {
	return p.at ( "@language" );
}

bool keyword ( pobj p ) {
	if ( !p || !p->STR() ) return false;
	return keyword ( *p->STR() );
}

bool is_abs_iri ( const string& s ) {
	return s.find ( ':' ) != string::npos;
}
bool is_rel_iri ( const string& s ) {
	return ! ( keyword ( s ) || is_abs_iri ( s ) );
}
pobj newMap ( const string& k, pobj v ) {
	pobj r = mk_somap_obj();
	( *r->MAP() ) [k] = v;
	return r;
}

bool isvalue ( pobj v ) {
	return v && v->MAP() && hasvalue ( v->MAP() );
}

polist vec2vec ( const vector<string>& x ) {
	polist res = mk_olist();
	for ( auto t : x ) res->push_back ( make_shared<string_obj> ( t ) );
	return res;
}

vector<string> vec2vec ( polist x ) {
	vector<string> res;
	for ( auto t : *x ) res.push_back ( *t->STR() );
	return res;
}

void make_list_if_not ( pobj& o ) {
	if ( o->LIST() ) return;
	pobj t = o->clone();
	o = mk_olist_obj ( olist ( 1, t ) );
}

void add_all ( polist l, pobj v ) {
	if ( v->LIST() ) l->insert ( l->end(), v->LIST()->begin(), v->LIST()->end() );
	else l->push_back ( v );
}

struct remote_doc_t {
	string url;
	pobj document;
	pstring context_url;
	remote_doc_t ( string url_, const pobj& document_ = 0, pstring context = 0 ) : url ( url_ ), document ( document_ ), context_url ( context ) { }
};

void* curl = curl_easy_init();
pobj convert ( const json_spirit::mValue& v ) {
	if ( v.is_uint64() ) return make_shared<uint64_t_obj> ( v.get_uint64() );
	if ( v.isInt() ) make_shared<int64_t_obj> ( v.get_int64() );
	if ( v.isString() ) return make_shared<string_obj> ( v.get_str() );
	if ( v.isDouble() ) return make_shared<double_obj> ( v.get_real() );
	if ( v.isBoolean() ) return make_shared<bool_obj> ( v.get_bool() );
	if ( v.is_null() ) return 0; //make_shared<null_obj>();
	pobj r;
	if ( v.isList() ) {
		r = make_shared<olist_obj>();
		auto a = v.get_array();
		for ( auto x : a ) r->LIST()->push_back ( convert ( x ) );
		return r;
	}
	if ( !v.isMap() ) throw "logic error";
	r = make_shared<somap_obj>();
	auto a = v.get_obj();
	for ( auto x : a ) ( *r->MAP() ) [x.first] = convert ( x.second );
	return r;
}

size_t write_data ( void *ptr, size_t size, size_t n, void *stream ) {
	string data ( ( const char* ) ptr, ( size_t ) size * n );
	* ( ( std::stringstream* ) stream ) << data << std::endl;
	return size * n;
}

string download ( const string& url ) {
	curl_easy_setopt ( curl, CURLOPT_URL, url.c_str() );
	curl_easy_setopt ( curl, CURLOPT_FOLLOWLOCATION, 1L );
	curl_easy_setopt ( curl, CURLOPT_NOSIGNAL, 1 );
	curl_easy_setopt ( curl, CURLOPT_ACCEPT_ENCODING, "deflate" );
	std::stringstream out;
	curl_easy_setopt ( curl, CURLOPT_WRITEFUNCTION, write_data );
	curl_easy_setopt ( curl, CURLOPT_WRITEDATA, &out );
	CURLcode res = curl_easy_perform ( curl );
	if ( res != CURLE_OK ) throw std::runtime_error ( "curl_easy_perform() failed: "s + curl_easy_strerror ( res ) );
	return out.str();
}

pobj fromURL ( const string& url ) {
	json_spirit::mValue r;
	json_spirit::read_string ( download ( url ), r );
	return convert ( r );
}

remote_doc_t load ( const string& url )  {
	pobj p;
	remote_doc_t doc ( url, p );
	try {
		doc.document = fromURL ( url );
	} catch ( ... ) {
		throw LOADING_REMOTE_CONTEXT_FAILED + "\t" + url;
	}
	return doc;
}

//template<typename obj>
class context_t : public somap_obj {
private:
	jsonld_options options;
	psomap term_defs = make_shared<somap>();
public:
	psomap_obj inverse = 0;

	context_t ( const jsonld_options& o = jsonld_options() ) : somap_obj(), options ( o ) {
		if ( options.base ) MAP()->at ( "@base" ) = make_shared<string_obj> ( *options.base );
	}

	pstring getContainer ( string prop ) {
		if ( prop == "@graph" ) return pstr ( "@set" );
		if ( keyword ( prop ) ) return pstr ( prop );
		auto it = term_defs->find ( prop );
		return it == term_defs->end() ? 0 : it->second->STR();
	}
	pstring getContainer ( pstring prop ) {
		return getContainer ( *prop );
	}

	typedef std::shared_ptr<context_t> pcontext;
	//Context Processing Algorithm http://json-ld.org/spec/latest/json-ld-api/#context-processing-algorithms
	pcontext parse ( pobj localContext, vector<string> remoteContexts = vector<string>() ) {
		context_t result ( *this );
		if ( !localContext || !localContext->LIST() ) localContext = mk_olist_obj ( olist ( 1, localContext ) );
		for ( auto context : *localContext->LIST() ) {
			if ( !context || context->Null() ) {
				result = context_t ( options );
				continue;
			} else if ( pstring s = context->STR() ) {
				string uri = resolve ( * ( *result.MAP() ) ["@base"]->STR(), *s ); // REVISE
				if ( std::find ( remoteContexts.begin(), remoteContexts.end(), uri ) != remoteContexts.end() ) throw RECURSIVE_CONTEXT_INCLUSION + "\t" + uri;
				remoteContexts.push_back ( uri );

				pobj remoteContext = fromURL ( uri );
				if ( !remoteContext->map_and_has ( "@context" ) ) throw INVALID_REMOTE_CONTEXT + "\t"; // + context;
				context = ( *remoteContext->MAP() ) ["@context"];
				result = *result.parse ( context, remoteContexts );
				continue;
			} else if ( !context->MAP() ) throw INVALID_LOCAL_CONTEXT + "\t"; // + context;
			somap& cm = *context->MAP();
			auto it = cm.find ( "@base" );
			if ( !remoteContexts.size() && it != cm.end() ) {
				pobj value = it->second;
				if ( value->Null() ) result.MAP()->erase ( "@base" );
				else if ( pstring s = value->STR() ) {
					if ( is_abs_iri ( *s ) ) ( *result.MAP() ) ["@base"] = value;
					else {
						string baseUri = * ( *result.MAP() ) ["@base"]->STR();
						if ( !is_abs_iri ( baseUri ) ) throw INVALID_BASE_IRI + "\t" + baseUri;
						( *result.MAP() ) ["@base"] = make_shared<string_obj> ( resolve ( baseUri, *s ) );
					}
				} else throw INVALID_BASE_IRI + "\t" + "@base must be a string";
			}
			// 3.5
			if ( ( it = cm.find ( "@vocab" ) ) != cm.end() ) {
				pobj value = it->second;
				if ( value->Null() ) result.MAP()->erase ( it );
				else if ( pstring s = value->STR() ) {
					if ( is_abs_iri ( *s ) ) ( *result.MAP() ) ["@vocab"] = value;
					else throw INVALID_VOCAB_MAPPING + "\t" + "@value must be an absolute IRI";
				} else throw INVALID_VOCAB_MAPPING + "\t" + "@vocab must be a string or null";
			}
			if ( ( it = cm.find ( "@language" ) ) != cm.end() ) {
				pobj value = it->second;
				if ( value->Null() ) result.MAP()->erase ( it );
				else if ( pstring s = value->STR() ) getlang ( result ) = make_shared<string_obj> ( lower ( *s ) );
				else throw INVALID_DEFAULT_LANGUAGE + "\t";// + value;
			}
			pdefined_t defined = make_shared<defined_t>();
			for ( auto it : cm ) {
				if ( is ( it.first, { "@base"s, "@vocab"s, "@language"s } ) ) continue;
				result.createTermDefinition ( context->MAP(), it.first, defined ); // REVISE
			}
		}
		return make_shared<context_t> ( result );
	}
	// Create Term Definition Algorithm
	void createTermDefinition ( const psomap context, const string term, pdefined_t pdefined ) {
		defined_t& defined = *pdefined;
		if ( defined.find ( term ) != defined.end() ) {
			if ( defined[term] ) return;
			throw CYCLIC_IRI_MAPPING + "\t" + term;
		}
		defined[term] = false;
		if ( keyword ( term ) ) throw KEYWORD_REDEFINITION + "\t" + term;
		term_defs->erase ( term );
		auto it = context->find ( term );
		psomap m;
		decltype ( m->end() ) _it;
		if ( it == context->end() || it->second->map_and_has_null ( "@id" ) ) {
			( *term_defs ) [term] = make_shared<null_obj>();
			defined[term] = true;
			return;
		}
		pobj& value = it->second;
		if ( value->STR() ) value = newMap ( "@id", value );
		if ( value->MAP() ) throw INVALID_TERM_DEFINITION;
		somap defn, &val = *value->MAP();
		//10
		if ( ( it = val.find ( "@type" ) ) != val.end() ) {
			pstring type = it->second->STR();
			if ( !type ) throw INVALID_TYPE_MAPPING;
			type = expandIri ( type, false, true, context, pdefined );
			if ( is ( *type, {"@id"s, "@vocab"s} ) || ( !startsWith ( *type, "_:" ) && is_abs_iri ( *type ) ) ) defn["@type"] = make_shared<string_obj> ( *type );
			else throw INVALID_TYPE_MAPPING + "\t" + *type;
		}
		// 11
		if ( ( it = val.find ( "@reverse" ) ) != val.end() ) {
			if ( throw_if_not_contains ( val, "@id", INVALID_REVERSE_PROPERTY ) && !it->second->STR() )
				throw INVALID_IRI_MAPPING + "\t" + "Expected String for @reverse value.";
			string reverse = *expandIri ( val.at ( "@reverse" )->STR(), false, true, context, pdefined );
			if ( !is_abs_iri ( reverse ) ) throw INVALID_IRI_MAPPING + "Non-absolute @reverse IRI: " + reverse;
			defn ["@id"] = make_shared<string_obj> ( reverse );
			if ( ( it = val.find ( "@container" ) ) != val.end() && is ( *it->second->STR(), { "@set"s, "@index"s }, INVALID_REVERSE_PROPERTY + "reverse properties only support set- and index-containers" ) )
				defn ["@container"] = it->second;
			defn["@reverse"] = make_shared<bool_obj> ( ( *pdefined ) [term] = true );
			( *term_defs ) [term] = mk_somap_obj ( defn );
			return;
		}
		defn["@reverse"] = make_shared<bool_obj> ( false );
		size_t colIndex;
		if ( ( it = val.find ( "@id" ) ) != val.end() && it->second->STR() && *it->second->STR() != term ) {
			if ( ! it->second->STR() ) throw INVALID_IRI_MAPPING + "expected value of @id to be a string";
			pstring res = expandIri ( it->second->STR(), false, true, context, pdefined );
			if ( res && ( keyword ( *res ) || is_abs_iri ( *res ) ) ) {
				if ( *res == "@context" ) throw INVALID_KEYWORD_ALIAS + "cannot alias @context";
				defn ["@id"] = make_shared<string_obj> ( res );
			} else throw INVALID_IRI_MAPPING + "resulting IRI mapping should be a keyword, absolute IRI or blank node";
		} else if ( ( colIndex = term.find ( ":" ) ) != string::npos ) {
			string prefix = term.substr ( 0, colIndex );
			string suffix = term.substr ( colIndex + 1 );
			if ( context->find ( prefix ) != context->end() ) createTermDefinition ( context, prefix, pdefined );
			if ( ( it = term_defs->find ( prefix ) ) != term_defs->end() )
				defn ["@id"] = make_shared<string_obj> ( *it->second->MAP()->at ( "@id" )->STR() + suffix );
			else defn["@id"] = make_shared<string_obj> ( term );
		} else if ( ( it = MAP()->find ( "@vocab" ) ) != MAP()->end() )
			defn ["@id"] = make_shared<string_obj> ( *MAP()->at ( "@vocab" )->STR() + term );
		else throw INVALID_IRI_MAPPING + "relative term defn without vocab mapping";

		// 16
		( ( it = val.find ( "@container" ) ) != val.end() ) && it->second->STR() &&
		is ( *it->second->STR(), { "@list"s, "@set"s, "@index"s, "@language"s }, INVALID_CONTAINER_MAPPING + "@container must be either @list, @set, @index, or @language" ) && ( defn["@container"] = it->second );

		auto i1 = val.find ( "@language" ), i2 = val.find ( "type" );
		pstring lang;
		if ( i1 != val.end() && i2 == val.end() ) {
			if ( !i1->second->Null() || ( lang = i2->second->STR() ) ) getlang ( defn ) = lang ? make_shared<string_obj> ( lower ( *lang ) ) : 0;
			else throw INVALID_LANGUAGE_MAPPING + "@language must be a string or null";
		}

		( *term_defs ) [term] = mk_somap_obj ( defn );
		( *pdefined ) [term] = true;
	}


	pstring expandIri ( const pstring value, bool relative, bool vocab, const psomap context, pdefined_t pdefined ) {
		return value ? expandIri ( value, relative, vocab, context, pdefined ) : 0;
	}
	//http://json-ld.org/spec/latest/json-ld-api/#iri-expansion
	pstring expandIri ( string value, bool relative, bool vocab, const psomap context, pdefined_t pdefined ) {
		if ( keyword ( value ) ) return make_shared<string> ( value );
		const defined_t& defined = *pdefined;
		if ( context && has ( context, value ) && !defined.at ( value ) ) createTermDefinition ( context, value, pdefined );
		if ( vocab && has ( term_defs, value ) ) {
			psomap td;
			return ( td = term_defs->at ( value )->MAP() ) ? td->at ( "@id" )->STR() : 0;
		}
		size_t colIndex = value.find ( ":" );
		if ( colIndex != string::npos ) {
			string prefix = value.substr ( 0, colIndex ), suffix = value.substr ( colIndex + 1 );
			if ( prefix == "_" || startsWith ( suffix, "//" ) ) return make_shared<string> ( value );
			if ( context && has ( context, prefix ) && ( !has ( pdefined, prefix ) || !defined.at ( prefix ) ) )
				createTermDefinition ( context, prefix, pdefined );
			if ( has ( term_defs, prefix ) ) return pstr ( *term_defs->at ( prefix )->MAP()->at ( "@id" )->STR() + suffix );
			return make_shared<string> ( value );
		}
		if ( vocab && MAP()->find ( "@vocab" ) != MAP()->end() ) return pstr ( *MAP()->at ( "@vocab" )->STR() + value );
		if ( relative ) return pstr ( resolve ( *MAP()->at ( "@base" )->STR(), value ) );
		if ( context && is_rel_iri ( value ) ) throw INVALID_IRI_MAPPING + "not an absolute IRI: " + value;
		return make_shared<string> ( value );
	}

	pstring get_type_map ( const string& prop ) {
		auto td = term_defs->find ( prop );
		return td == term_defs->end() || !td->second->MAP() ? 0 : gettype ( td->second )->STR();
	}

	pstring get_lang_map ( const string& prop ) {
		auto td = term_defs->find ( prop );
		return td == term_defs->end() || !td->second->MAP() ? 0 : getlang ( td->second )->STR();
	}

	psomap get_term_def ( const string& key ) {
		return term_defs->at ( key )->MAP();
	}

	// http://json-ld.org/spec/latest/json-ld-api/#inverse-context-creation
	psomap_obj getInverse() {
		if ( inverse ) return inverse;
		inverse = mk_somap_obj();
		pstring defaultLanguage = getlang ( MAP() )->STR();
		if ( !defaultLanguage ) ( *MAP() ) ["@language"] = mk_str_obj ( defaultLanguage = pstr ( "@none" ) );

		for ( auto x : *term_defs ) {
			string term = x.first;
			auto it = term_defs->find ( term );
			psomap definition = it == term_defs->end() || !it->second ? 0 : it->second->MAP();
			if ( !definition ) continue;
			pstring container = ( ( it = definition->find ( "@container" ) ) == definition->end() || !it->second ) ? 0 : it->second->STR();
			if ( !container ) container = pstr ( "@none" );
			pstring iri = ( ( it = definition->find ( "@id" ) ) == definition->end() ) || !it->second ? 0 : it->second->STR();

			psomap_obj containerMap = mk_somap_obj ( iri ? inverse->MAP()->at ( *iri )->MAP() : 0 );
			if ( !containerMap ) {
				containerMap = mk_somap_obj();
				inverse->MAP()->at ( *iri ) = containerMap;
			}
			psomap_obj type_lang_map = mk_somap_obj ( container ? containerMap->MAP()->at ( *container )->MAP() : 0 );
			if ( !type_lang_map ) {
				type_lang_map = mk_somap_obj();
				( *type_lang_map->MAP() ) ["@language"] = mk_somap_obj();
				( *type_lang_map->MAP() ) ["@type"] = mk_somap_obj();
				( *containerMap->MAP() ) [ *container ] = type_lang_map;
			}
			if ( definition->at ( "@reverse" )->BOOL() ) {
				psomap typeMap = type_lang_map->MAP()->at ( "@type" )->MAP();
				if ( !hasreverse ( typeMap ) ) ( * typeMap ) ["@reverse"] = make_shared<string_obj> ( term );
			} else if ( hastype ( definition ) ) {
				psomap typeMap = gettype ( type_lang_map )->MAP();
				if ( !has ( typeMap, gettype ( definition )->STR() ) ) ( *typeMap ) [ *gettype ( definition )->STR() ] = make_shared<string_obj> ( term );
			} else if ( haslang ( definition ) ) {
				psomap lang_map = gettype ( type_lang_map )->MAP();
				pstring language = getlang ( definition )->STR();
				if ( !language ) ( *definition ) ["@language"] = mk_str_obj ( language = pstr ( "@null" ) );
				if ( !has ( lang_map, language ) ) ( *lang_map ) [ *language ] = make_shared<string_obj> ( term );
			} else {
				psomap lang_map = getlang ( type_lang_map )->MAP();
				if ( !haslang ( lang_map ) ) ( * lang_map ) ["@language"] = make_shared<string_obj> ( term );
				if ( !hasnone ( lang_map ) ) ( * lang_map ) ["@none"] = make_shared<string_obj> ( term );
				psomap typeMap = gettype ( type_lang_map )->MAP();
				if ( !hasnone ( typeMap ) ) ( *typeMap ) ["@none"] = make_shared<string_obj> ( term );
			}
		}
		return inverse;
	}


	int compareShortestLeast ( const string& a, const string& b ) {
		if ( a.length() < b.length() ) return -1;
		else if ( b.length() < a.length() ) return 1;
		return a == b ? 0 : a < b ? -1 : 1;
	}
	int compareShortestLeast ( pstring a, pstring b ) {
		return !a && !b ? 0 : a && !b ? 1 : !a && b ? -1 : compareShortestLeast ( *a, *b );
	}
	int compareShortestLeast ( string a, pstring b ) {
		return !b ? 1 : compareShortestLeast ( a, *b );
	}
	int compareShortestLeast ( pstring a, string b ) {
		return !a ? -1 : compareShortestLeast ( *a, b );
	}

	// http://json-ld.org/spec/latest/json-ld-api/#term-selection
	pstring selectTerm ( string iri, vector<string>& containers, string typeLanguage, vector<string>& preferredValues ) {
		auto inv = getInverse();
		auto containerMap = inv->MAP()->at ( iri )->MAP();
		for ( string container : containers ) {
			if ( !has ( containerMap, container ) ) continue;
			auto type_lang_map = containerMap->at ( container )->MAP();
			auto valueMap = type_lang_map ->at ( typeLanguage )->MAP();
			for ( string item : preferredValues ) {
				if ( !has ( valueMap, item ) ) continue;
				return valueMap->at ( item )->STR();
			}
		}
		return 0;
	}

	pstring compactIri ( string iri, bool relativeToVocab ) {
		return compactIri ( iri, 0, relativeToVocab, false );
	}
	pstring compactIri ( pstring iri, bool relativeToVocab ) {
		return !iri ? 0 : compactIri ( *iri, 0, relativeToVocab, false );
	}
	pstring compactIri ( pstring iri, pobj value, bool relativeToVocab, bool reverse ) {
		return !iri ? 0 : compactIri ( *iri, value, relativeToVocab, reverse );
	}
	// http://json-ld.org/spec/latest/json-ld-api/#iri-compaction
	pstring compactIri ( string iri, pobj value, bool relativeToVocab, bool reverse ) {
		if ( relativeToVocab && has ( inverse->MAP(), iri ) ) {
			auto it = MAP()->find ( "@language" );
			pstring defaultLanguage = 0;
			if ( it != MAP()->end() && it->second ) defaultLanguage = it->second->STR();
			if ( !defaultLanguage ) defaultLanguage = pstr ( "@none" );
			vector<string> containers;
			pstring type_lang = pstr ( "@language" ), type_lang_val = pstr ( "@null" );
			if ( value->MAP() && has ( value->MAP(), "@index" ) ) containers.push_back ( "@index" );
			if ( reverse ) {
				type_lang = pstr ( "@type" );
				type_lang_val = pstr ( "@reverse" );
				containers.push_back ( "@set" );
			} else if ( value->MAP() && has ( value->MAP(),  "@list" ) ) {
				if ( ! has ( value->MAP(),  "@index" ) ) containers.push_back ( "@list" );
				polist list = value->MAP( )->at ( "@list" )->LIST();
				pstring common_lang = ( list->size() == 0 ) ? defaultLanguage : 0, common_type = 0;
				// 2.6.4)
				for ( pobj item : *list ) {
					pstring itemLanguage = pstr ( "@none" ), itemType = pstr ( "@none" );
					if ( isvalue ( item ) ) {
						if ( ( it = item->MAP()->find ( "@language" ) ) != item->MAP()->end() ) itemLanguage = it->second->STR();
						else if (  ( it = item->MAP()->find ( "@type" ) ) != item->MAP()->end()  ) itemType = it->second->STR();
						else itemLanguage = pstr ( "@null" );
					} else itemType = pstr ( "@id" );
					if ( !common_lang ) common_lang = itemLanguage;
					else if ( common_lang != itemLanguage && isvalue ( item ) ) common_lang = pstr ( "@none" );
					if ( !common_type ) common_type = itemType;
					else if ( common_type != itemType  ) common_type = pstr ( "@none" );
					if ( "@none"s == *common_lang  && "@none"s == * common_type  ) break;
				}
				common_lang =  common_lang  ? common_lang : pstr ( "@none" );
				common_type =  common_type  ? common_type : pstr ( "@none" );
				if ( "@none"s != *common_type )  {
					type_lang = pstr ( "@type" );
					type_lang_val = common_type;
				} else type_lang_val = common_lang;
			} else {
				if ( value->MAP() && has ( value->MAP(),  "@value" ) ) {
					if ( hasvalue ( value->MAP() )
					        && ! hasindex ( value->MAP() ) ) {
						containers.push_back ( "@language" );
						type_lang_val = getlang ( value )->STR();
					} else if ( hastype ( value->MAP() ) ) {
						type_lang = pstr ( "@type" );
						type_lang_val = gettype ( value )->STR();
					}
				} else {
					type_lang = pstr ( "@type" );
					type_lang_val = pstr ( "@id" );
				}
				containers.push_back ( "@set" );
			}

			containers.push_back ( "@none" );
			if ( !type_lang_val ) type_lang_val = pstr ( "@null" );
			vector<string> preferredValues;
			if ( "@reverse"s ==  *type_lang_val  ) preferredValues.push_back ( "@reverse" );
			if ( ( "@reverse"s ==  *type_lang_val  || "@id"s ==  *type_lang_val  )
			        && ( value->MAP() ) && has ( value->MAP(),  "@id" ) ) {
				pstring result = compactIri (  value->MAP( )->at ( "@id" )->STR(), 0, true, true );
				auto it = term_defs->find ( *result );
				if ( it != term_defs->end()
				        && has ( it->second->MAP(), "@id" )
				        && jsonld::equals ( value->MAP( )->at ( "@id" ),
				                      term_defs->at ( *result )->MAP( )->at ( "@id" ) ) ) {
					preferredValues.push_back ( "@vocab" );
					preferredValues.push_back ( "@id" );
				} else {
					preferredValues.push_back ( "@id" );
					preferredValues.push_back ( "@vocab" );
				}
			} else preferredValues.push_back ( *type_lang_val );
			preferredValues.push_back ( "@none" );
			pstring term = selectTerm ( iri, containers, *type_lang, preferredValues );
			if ( term  ) return term;
		}
		if ( relativeToVocab && hasvocab ( MAP() ) ) {
			pstring vocab = getvocab ( MAP() )->STR();
			if ( vocab && iri.find ( *vocab ) == 0 && iri != *vocab ) {
				string suffix = iri.substr ( vocab->length() );
				if ( !has ( term_defs, suffix ) ) return pstr ( suffix );
			}
		}
		pstring compactIRI = 0;
		for ( auto x : *term_defs ) {
			string term = x.first;
			psomap term_def = x.second->MAP();
			if ( term.find ( ":" ) != string::npos ) continue;
			if ( !term_def// || iri == term_def->at ( "@id" )->STR()
			        || !startsWith ( iri,  *getid ( term_def )->STR() ) )
				continue;

			string candidate = term + ":" + iri.substr ( getid ( term_def )->STR()->length() );
			// TODO: verify && and ||
			if ( ( !compactIRI || compareShortestLeast ( candidate, compactIRI ) < 0 )
			        && ( !has ( term_defs, candidate ) || ( iri == *getid ( term_defs->at ( candidate ) )->STR() ) && !value ) )
				compactIRI = pstr ( candidate );
		}
		if ( !compactIRI  ) return compactIRI;
		if ( !relativeToVocab ) return removeBase ( getbase ( MAP() ), iri );
		return make_shared<string> ( iri );
	}

	// http://json-ld.org/spec/latest/json-ld-api/#value-compaction
	pobj compactValue ( string activeProperty, psomap_obj value_ ) {
		psomap value = value_->MAP();
		int nvals = value->size();
		auto p = getContainer ( activeProperty );
		if ( value->find ( "@index" ) != value->end() && p && *p == "@index" ) nvals--;
		if ( nvals > 2 ) return value_;//mk_somap_obj ( value_ );
		pstring type_map = get_type_map ( activeProperty ), lang_map = get_lang_map ( activeProperty );
		auto it = value->find ( "@id" );
		if ( it != value->end() && nvals == 1 )
			if ( type_map && *type_map == "@id" && nvals == 1 )
				return make_shared<string_obj> ( compactIri ( value->at ( "@id" )->STR(), 0, false, false ) );
			else {
				if ( type_map && *type_map == "@vocab" && nvals == 1 )
					return make_shared<string_obj> ( compactIri ( value->at ( "@id" )->STR(), 0, true, false ) );
				else return  mk_somap_obj ( value );
			}
		pobj valval = value->at ( "@value" );
		it = value->find ( "@type" );
		if ( it != value->end() &&  *it->second->STR() == *type_map  ) return valval;
		if ( ( it = value->find ( "@language" ) ) != value->end() ) // TODO: SPEC: doesn't specify to check default language as well
			if ( *it->second->STR() == * lang_map  || jsonld::equals ( it->second, getlang ( MAP() ) ) )
				return valval;
		if ( nvals == 1
		        && ( !valval->STR() || haslang ( MAP() ) || ( term_defs->find ( activeProperty ) == term_defs->end()
		                && haslang ( get_term_def ( activeProperty ) ) && !lang_map ) ) )
			return valval;
		return value_;
	}

	bool isReverseProperty ( string prop ) {
		auto it = term_defs->find ( prop );
		if ( it == term_defs->end() || !it->second->MAP() ) return false;
		auto r = it->second->MAP()->at ( "@reverse" );
		return r && r->BOOL() && *r->BOOL();
	}
public:
	pobj expandValue ( string activeProperty, pobj value )  {
		somap rval;
		psomap td = term_defs->at ( activeProperty )->MAP();
		if ( td && *gettype ( td )->STR() == "@id" ) {
			rval[ "@id" ] = make_shared<string_obj> ( expandIri ( value->toString(), true, false, 0, 0 ) );
			return mk_somap_obj ( rval );
		}
		if ( td && *gettype ( td )->STR() == "@vocab" ) {
			rval[ "@id" ] = make_shared<string_obj> ( expandIri ( value->toString(), true, true, 0, 0 ) );
			return mk_somap_obj ( rval );
		}
		rval[ "@value" ] = value;
		if ( td && hastype ( td ) ) rval[ "@type" ] = gettype ( td );
		else if ( value->STR() ) {
			if ( td && haslang ( td ) ) {
				pstring lang = getlang ( td )->STR();
				if ( lang ) rval[ "@language"] = make_shared<string_obj> ( lang );
			} else if ( haslang ( MAP() ) ) rval[ "@language" ] = getlang ( MAP() );
		}
		return mk_somap_obj ( rval );
	}
	map<string, string> getPrefixes ( bool onlyCommonPrefixes ) {
		map<string, string> prefixes;
		for ( auto x : *term_defs ) {
			string term = x.first;
			if ( term.find ( ":" ) != string::npos ) continue;
			psomap td = term_defs->at ( term )->MAP();
			if ( !td ) continue;
			pstring id = td->at ( "@id" )->STR();
			if ( !id ) continue;
			if ( startsWith ( term, "@" ) || startsWith ( *id, "@" ) ) continue;
			if ( !onlyCommonPrefixes || endsWith ( *id, "/" ) || endsWith ( *id, "#" ) ) prefixes[term] = *id;
		}
		return prefixes;
	}
};

typedef std::shared_ptr<context_t> pcontext;
typedef map<string, string> ssmap;
typedef std::shared_ptr<ssmap> pssmap;

class node;
typedef std::shared_ptr<node> pnode;
typedef map<string, pnode> snmap;
typedef std::shared_ptr<snmap> psnmap;

const string RDF_SYNTAX_NS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#", RDF_SCHEMA_NS = "http://www.w3.org/2000/01/rdf-schema#" , XSD_NS = "http://www.w3.org/2001/XMLSchema#" , XSD_ANYTYPE = XSD_NS + "anyType" , XSD_BOOLEAN = XSD_NS + "boolean" , XSD_DOUBLE = XSD_NS + "double" , XSD_INTEGER = XSD_NS + "integer", XSD_FLOAT = XSD_NS + "float", XSD_DECIMAL = XSD_NS + "decimal", XSD_ANYURI = XSD_NS + "anyURI", XSD_STRING = XSD_NS + "string", RDF_TYPE = RDF_SYNTAX_NS + "type", RDF_FIRST = RDF_SYNTAX_NS + "first", RDF_REST = RDF_SYNTAX_NS + "rest", RDF_NIL = RDF_SYNTAX_NS + "nil", RDF_PLAIN_LITERAL = RDF_SYNTAX_NS + "PlainLiteral", RDF_XML_LITERAL = RDF_SYNTAX_NS + "XMLLiteral", RDF_OBJECT = RDF_SYNTAX_NS + "object", RDF_LANGSTRING = RDF_SYNTAX_NS + "langString", RDF_LIST = RDF_SYNTAX_NS + "List";

class node {
public:
	string type, value, datatype, lang;
	enum node_type { LITERAL, IRI, BNODE };
	const node_type _type;
	node ( const node_type& t ) : _type ( t ) {}
};
/*
	somap node::toObject ( bool useNativeTypes ) {
		if ( type==IRI || type==BNODE ) { somap r; r["@id"]=mk_str_obj(at("value"));return r; }
		somap rval;
		rval[ "@value" ] = mk_str_obj(at ( "value" ));
		auto it = find ( "language" );
		if ( it != end() ) rval[ "@language" ] = mk_str_obj(it->second);
		else {
			string type = at ( "datatype" ), value = at ( "value" );
			if ( useNativeTypes ) {
				if ( XSD_STRING == type ) {
				} else if ( XSD_BOOLEAN == type  ) {
					if ( value == "true"  ) rval[ "@value"] = make_shared<bool_obj> ( true );
					else if ( value == "false" ) rval[ "@value"] = make_shared<bool_obj> ( false );
					else rval[ "@type"] = mk_str_obj(type);
				} else if ( ( XSD_INTEGER == type && is_int ( value ) ) ( XSD_DOUBLE == type && is_double ( value ) ) ) {
					double d = std::stod ( value );
					if ( !::isnan ( d ) && !::isinf ( d ) ) {
						if ( XSD_INTEGER == type ) {
							int64_t i = d;
							if ( tostr ( i ) == value ) rval[ "@value"] = make_shared<uint64_t_obj> ( i );
						} else if ( XSD_DOUBLE == type )
							rval[ "@value"] = make_shared<double_obj> ( d );
						else throw "This should never happen as we checked the type was either integer or double";
					}
				} else rval["@type"] = mk_str_obj ( type );
			} else if ( XSD_STRING != type  ) rval["@type"] = mk_str_obj ( type );
		}
		return rval;
	}
*/

pnode mkliteral ( string value, pstring datatype, pstring language ) {
	pnode r = make_shared<node> ( node::LITERAL );
	r->type = "literal" ;
	r ->value = value ;
	r-> datatype = datatype ? *datatype : XSD_STRING;
	if ( language ) r->lang = *language;
	return r;
}
pnode mkiri ( string iri ) {
	pnode r = make_shared<node> ( node::IRI );
	r ->type = "IRI";
	r->value = iri;
	return r;
}
pnode mkbnode ( string attribute ) {
	pnode r = make_shared<node> ( node::BNODE );
	r->type = "blank node" ;
	r->value = attribute ;
	return r;
}

typedef std::tuple<pnode, pnode, pnode, pnode>  quad_base;

class quad : public quad_base { //map<string, pnode> {
	quad ( string subj, string pred, pnode object, pstring graph ) :
		quad ( startsWith ( subj, "_:" ) ? mkbnode ( subj ) : mkiri ( subj ), mkiri ( pred ), object, graph ) {}
public:
	pnode &subj = std::get<0> ( *this ), &pred = std::get<1> ( *this ), &object = std::get<2> ( *this ), &graph = std::get<3> ( *this );

	quad ( string subj, string pred, string object, pstring graph ) :
		quad ( subj, pred, startsWith ( object, "_:" ) ?  mkbnode ( object ) : mkiri ( object ), graph ) {}
	quad ( string subj, string pred, string value, pstring datatype, pstring language, pstring graph ) :
		quad ( subj, pred, mkliteral ( value, datatype, language ), graph ) {}
	quad ( pnode subj, pnode pred, pnode object, pstring graph ) :
		quad_base ( subj, pred, object, graph && *graph == "@default" ? startsWith ( *graph, "_:" ) ? mkbnode ( *graph ) : mkiri ( *graph ) : 0 ) { }

	string tostring ( string ctx ) {
		return "< "s + ctx + " > : < " + subj->value + " > <" + pred->value + " > < " + object->value + " > .";
	}
};

typedef std::shared_ptr<quad> pquad;
typedef list<pquad> qlist;
typedef std::shared_ptr<qlist> pqlist;
typedef map<string, pqlist> qdb;

const pnode first = mkiri ( RDF_FIRST );
const  pnode rest = mkiri ( RDF_REST );
const pnode nil = mkiri ( RDF_NIL );

pqlist mk_qlist() {
	return make_shared<qlist>();
}
class rdf_db;
class jsonld_api {
public:
	jsonld_options opts;
	pobj value = 0;
	pcontext context = 0;

	jsonld_api ( pobj input, jsonld_options opts ) : jsonld_api ( opts )  {
		initialize ( input, 0 );
	}
	jsonld_api ( pobj input, pobj context, jsonld_options opts ) : jsonld_api ( opts )  {
		initialize ( input, 0 );
	}
	jsonld_api ( jsonld_options opts_ = jsonld_options ( "" ) ) : opts ( opts_ ) {}
private:
	void initialize ( pobj input, pobj context_ ) {
		if ( input->LIST() || input->MAP() ) value = input->clone();
		context = make_shared<context_t>();
		if ( context ) context = context->parse ( context_ );
	}
public:
	// http://json-ld.org/spec/latest/json-ld-api/#compaction-algorithm
	pobj compact ( pcontext activeCtx, string activeProperty, pobj element, bool compactArrays ) {
		if ( element->LIST() ) {
			polist result = mk_olist();
			for ( pobj item : *element->LIST() ) {
				pobj compactedItem = compact ( activeCtx, activeProperty, item, compactArrays );
				if ( compactedItem ) result->push_back ( compactedItem );
			}
			return ( compactArrays && result->size() == 1 && !activeCtx->getContainer ( activeProperty ) ) ? result->at ( 0 ) : mk_olist_obj ( result );
		}
		if ( !element->MAP() ) return element;

		psomap elem = element->MAP();
		if ( has ( elem, "@value" ) || has ( elem, "@id" ) )
			// TODO: spec tells to pass also inverse to compactValue
			if ( pobj compacted_val = activeCtx->compactValue ( activeProperty, mk_somap_obj ( elem ) ) )
				if ( ! ( compacted_val->MAP() || compacted_val->LIST() ) )
					return compacted_val;

		bool insideReverse = activeProperty == "@reverse";
		psomap result = make_shared<somap>();
		for ( auto x : *elem ) { // 7
			string exp_prop = x.first;
			pobj exp_val = x.second;
			if ( is ( exp_prop, { "@id"s, "@type"s} ) ) {
				pobj compacted_val;
				// TODO: spec tells to pass also inverse to compactIri
				if ( exp_val->STR() ) compacted_val = make_shared<string_obj> ( activeCtx->compactIri ( exp_val->STR(), exp_prop == "@type" ) );
				else {
					vector<string> types;
					for ( auto expandedType : *exp_val->LIST() ) types.push_back ( *activeCtx->compactIri ( expandedType->STR(), true ) );
					if ( types.size() == 1 ) compacted_val = make_shared<string_obj> ( types[0] );
					else compacted_val = mk_olist_obj ( vec2vec ( types ) );
				}
				pstring alias = activeCtx->compactIri ( exp_prop, true );
				result->at ( *alias ) = compacted_val;
				continue;
			}
			if ( exp_prop == "@reverse" ) {
				psomap compacted_val = compact ( activeCtx, "@reverse", exp_val, compactArrays )->MAP();
				// Must create a new set to avoid modifying the set we are iterating over
				for ( auto y : somap ( *compacted_val ) ) {
					string property = y.first;
					pobj value = y.second;
					if ( activeCtx->isReverseProperty ( property ) ) {
						if ( ( *activeCtx->getContainer ( property ) == "@set" || !compactArrays ) && !value->LIST() )
							value = mk_olist_obj ( olist ( 1, value ) );
						if ( !has ( result, property ) ) ( *result ) [ property ] = value;
						else {
							make_list_if_not ( result->at ( property ) );
							add_all ( result->at ( property )->LIST(), value );
						}
						compacted_val->erase ( property );
					}
				}
				if ( compacted_val->size() ) result->at ( *activeCtx->compactIri ( "@reverse", true ) ) = mk_somap_obj ( compacted_val );
				continue;
			}
			if ( exp_prop == "@index" && *activeCtx->getContainer ( activeProperty ) == "@index" ) continue;
			if ( is ( exp_prop, {"@index"s, "@value"s, "@language"s} ) ) {
				result->at ( *activeCtx->compactIri ( exp_prop, true ) ) = exp_val;
				continue;
			}
			if ( !exp_val->LIST()->size() ) {
				string itemActiveProperty = *activeCtx->compactIri ( exp_prop, exp_val, true, insideReverse );
				auto it = result->find ( itemActiveProperty );
				if ( it == result->end() ) result->at ( itemActiveProperty ) = mk_olist_obj();
				else make_list_if_not ( it->second );

			}
			for ( pobj exp_item : *exp_val->LIST() ) {
				string itemActiveProperty = *activeCtx->compactIri ( exp_prop, exp_item, true, insideReverse );
				string container = *activeCtx->getContainer ( itemActiveProperty );
				bool isList = has ( exp_item->MAP(), "@list" );
				pobj list = isList ? exp_item->MAP()->at ( "@list" ) : 0;
				pobj compactedItem = compact ( activeCtx, itemActiveProperty, isList ? list : exp_item, compactArrays );
				if ( isList ) {
					make_list_if_not ( compactedItem );
					if ( container != "@list" ) {
						psomap wrapper = make_shared<somap>();
						wrapper->at ( *activeCtx->compactIri ( "@list", true ) ) = compactedItem;
						compactedItem = mk_somap_obj ( wrapper );
						if ( has ( exp_item->MAP(), "@index" ) ) {
							compactedItem->MAP()->at (
							    // TODO: SPEC: no mention of vocab =
							    // true
							    *activeCtx->compactIri ( "@index", true ) ) = exp_item->MAP()->at ( "@index" ) ;
						}
					} else if ( has ( result, itemActiveProperty ) ) throw COMPACTION_TO_LIST_OF_LISTS + "\t" + "There cannot be two list objects associated with an active property that has a container mapping";
				}
				if ( is ( container, {"@language"s, "@index"s} ) ) {
					psomap_obj mapObject;
					if ( has ( result, itemActiveProperty ) ) mapObject = mk_somap_obj ( result->at ( itemActiveProperty )->MAP() );
					else result->at ( itemActiveProperty ) = mapObject = mk_somap_obj();
					if ( container == "@language" && has ( compactedItem->MAP(), "@value" ) )
						compactedItem = compactedItem->MAP()->at ( "@value" );
					string mapKey = *exp_item->MAP() ->at ( container )->STR();
					if ( !has ( mapObject->MAP(), mapKey ) ) ( *mapObject->MAP() ) [ mapKey ] = compactedItem;
					else {
						make_list_if_not ( mapObject->MAP()->at ( mapKey ) );
						mapObject->MAP()->at ( mapKey )->LIST()->push_back ( compactedItem );
					}
				}
				else {
					bool check = ( !compactArrays || is ( container, {"@set"s, "@list"} ) || is ( exp_prop, {"@list"s, "@graph"s} ) ) && ( !compactedItem->LIST() );
					if ( check ) compactedItem = mk_olist_obj ( olist ( 1, compactedItem ) );
					if ( !has ( result, itemActiveProperty ) )  ( *result ) [ itemActiveProperty ] = compactedItem;
					else {
						make_list_if_not ( result->at ( itemActiveProperty ) );
						add_all ( result->at ( itemActiveProperty )->LIST(), compactedItem );
					}
				}
			}
		}
		return mk_somap_obj ( result );
	}

	pobj compact ( pcontext activeCtx, string activeProperty, pobj element ) {
		return compact ( activeCtx, activeProperty, element, true );
	}

	// http://json-ld.org/spec/latest/json-ld-api/#expansion-algorithm
	pobj expand ( pcontext& activeCtx, const string& activeProperty, pobj& element ) {
		if ( !element )  return 0;
		if ( element->LIST() ) {
			polist_obj result = mk_olist_obj();
			for ( pobj item : *element->LIST() ) {
				pobj v = expand ( activeCtx, activeProperty, item );
				if ( ( activeProperty == "@list" || *activeCtx->getContainer ( activeProperty ) == "@list" )
				        && ( v->LIST() || ( v->MAP() && has ( v->MAP(), "@list" ) ) ) )
					throw LIST_OF_LISTS + "\t"s + "lists of lists are not permitted.";
				if ( v ) add_all ( result->LIST(), v );

			}
			return result;
		} else if ( element->MAP() ) {
			psomap elem = element->MAP();
			if ( has ( elem, "@context" ) ) activeCtx = activeCtx->parse ( elem->at ( "@context" ) );
			psomap result = make_shared<somap>();
			for ( auto x : *elem ) {
				string key = x.first;
				pobj value = x.second;
				if ( key == "@context" ) continue;
				pstring exp_prop = activeCtx->expandIri ( key, false, true, 0, 0 );
				pobj exp_val = 0;
				if ( !exp_prop || ( exp_prop->find ( ":" ) == string::npos && !keyword ( *exp_prop ) ) ) continue;
				if ( keyword ( *exp_prop ) ) {
					if ( activeProperty == "@reverse" ) throw INVALID_REVERSE_PROPERTY_MAP + "\t"s + "a keyword cannot be used as a @reverse propery";
					if ( has ( result, exp_prop ) ) throw COLLIDING_KEYWORDS + "\t" + *exp_prop + " already exists in result";
					if ( *exp_prop == "@id" ) {
						if ( !value->STR() ) throw INVALID_ID_VALUE + "\t" + "value of @id must be a string";
						exp_val = make_shared<string_obj> ( activeCtx->expandIri ( value->STR(), true, false, 0, 0 ) );
					} else if ( *exp_prop == "@type" ) {
						if ( value->LIST() ) {
							exp_val = mk_olist_obj();
							for ( pobj v :  *value->LIST() ) {
								if ( !v->STR() ) throw INVALID_TYPE_VALUE + "\t" + "@type value must be a string or array of strings" ;
								exp_val->LIST()->push_back ( make_shared<string_obj> ( activeCtx->expandIri ( v->STR(), true, true, 0, 0 ) ) );
							}
						} else if ( value->STR() ) exp_val = make_shared<string_obj> ( activeCtx->expandIri ( value->STR(), true, true, 0, 0 ) );
						else if ( value->MAP() ) {
							if ( value->MAP()->size() ) throw INVALID_TYPE_VALUE + "\t" + "@type value must be a an empty object for framing";
							exp_val = value;
						} else
							throw INVALID_TYPE_VALUE + "\t" + "@type value must be a string or array of strings";
					} else if ( *exp_prop == "@graph" ) exp_val = expand ( activeCtx, "@graph", value );
					else if ( *exp_prop == "@value" ) {
						if ( value && ( value->MAP() || value->LIST() ) ) throw INVALID_VALUE_OBJECT_VALUE + "\t"s + "value of " + *exp_prop + " must be a scalar or null";
						if ( ! ( exp_val = value ) ) {
							result->at ( "@value" ) = 0;
							continue;
						}
					} else if ( *exp_prop == "@language" ) {
						if ( !value->STR() ) throw INVALID_LANGUAGE_TAGGED_STRING + "\t" + "Value of "s + *exp_prop + " must be a string";
						exp_val = make_shared<string_obj> ( lower ( *value->STR() ) );
					} else if ( *exp_prop == "@index" ) {
						if ( !value->STR() ) throw INVALID_INDEX_VALUE + "\t" + "Value of "s + *exp_prop + " must be a string";
						exp_val = value;
					} else if ( *exp_prop == "@list" ) {
						if ( activeProperty == "@graph" ) continue;
						exp_val = expand ( activeCtx, activeProperty, value );
						if ( !exp_val->LIST() ) exp_val = mk_olist_obj ( olist ( 1, exp_val ) );
						for ( auto o : *exp_val->LIST() ) if ( o->MAP() && has ( o->MAP(), "@list" ) ) throw LIST_OF_LISTS + "\t" + "A list may not contain another list";
					} else if ( *exp_prop == "@set" ) exp_val = expand ( activeCtx, activeProperty, value );
					else if ( *exp_prop == "@reverse" ) {
						if ( !value->MAP() ) throw INVALID_REVERSE_VALUE + "\t" + "@reverse value must be an object";
						exp_val = expand ( activeCtx, "@reverse", value );
						if ( has ( exp_val->MAP(), "@reverse" ) ) {
							psomap reverse = exp_val->MAP()->at ( "@reverse" )->MAP();
							for ( auto z : *reverse ) {
								string property = z.first;
								pobj item = z.second;
								if ( !has ( result, property ) ) ( *result ) [ property ] = mk_olist_obj();
								add_all ( result->at ( property )->LIST(), item );
							}
						}
						if ( exp_val->MAP()->size() > ( has ( exp_val->MAP(), "@reverse" ) ? 1 : 0 ) ) {
							if ( !has ( result, "@reverse" ) )  ( *result ) [ "@reverse" ] = mk_somap_obj();
							psomap reverseMap = result->at ( "@reverse" )->MAP();
							for ( auto t : *exp_val->MAP() ) {
								string property = t.first;
								if ( property == "@reverse" ) continue;
								polist items = exp_val->MAP()->at ( property )->LIST();
								for ( pobj item : *items ) {
									if ( has ( item->MAP(), "@value" ) || has ( item->MAP(), "@list" ) ) throw INVALID_REVERSE_PROPERTY_VALUE;
									if ( !has ( reverseMap, property ) ) ( *reverseMap ) [ property ] = mk_olist_obj();
									reverseMap->at ( property )->LIST()->push_back ( item );
								}
							}
						}
						continue;
					} else if ( is ( exp_prop, {"@explicit"s, "@default"s, "@embed"s, "@embedChildren"s, "@omitDefault"s} ) )
						exp_val = expand ( activeCtx, *exp_prop, value );
					if ( exp_val ) result->at ( *exp_prop ) = exp_val;
					continue;
				} else if ( *activeCtx->getContainer ( key ) == "@language" && value->MAP() ) {
					exp_val = mk_olist_obj();
					for ( auto yy : *value->MAP() ) {
						string language = yy.first;
						pobj languageValue = yy.second;
						make_list_if_not ( languageValue );
						for ( pobj item : *languageValue->LIST() ) {
							if ( ! item->STR() ) throw INVALID_LANGUAGE_MAP_VALUE; // + "\t" + "Expected " + item.toString() + " to be a string");
							somap tmp;
							tmp["@value"] = item;
							tmp["@language"] = make_shared<string_obj> ( lower ( language ) );
							exp_val->LIST( )->push_back ( mk_somap_obj ( tmp ) );
						}
					}
				} else if ( *activeCtx->getContainer ( key ) == "@index" && value->MAP() ) {
					exp_val = mk_olist_obj();
					for ( auto xx : *value->MAP() ) {
						pobj indexValue = xx.second;
						make_list_if_not ( indexValue );
						indexValue = expand ( activeCtx, key, indexValue );
						for ( pobj item : *indexValue->LIST() ) {
							if ( !has ( item->MAP(), "@index" ) ) ( *item->MAP() ) [ "@index" ] = make_shared<string_obj> ( xx.first );
							exp_val->LIST()->push_back ( item );
						}
					}
				} else exp_val = expand ( activeCtx, key, value );
				if ( !exp_val ) continue;
				if ( *activeCtx->getContainer ( key ) == "@list" && !has ( exp_val->MAP(), "@list" ) ) {
					auto tmp = exp_val;
					make_list_if_not ( tmp );
					exp_val = mk_somap_obj();
					( *exp_val->MAP( ) ) [ "@list" ] = tmp;
				}
				if ( activeCtx->isReverseProperty ( key ) ) {
					if ( !has ( result, "@reverse" ) ) ( *result ) [ "@reverse" ] = mk_somap_obj();
					psomap reverseMap =  result->at ( "@reverse" )->MAP();
					make_list_if_not ( exp_val );
					for ( pobj item : *exp_val->LIST() ) {
						if ( has ( item->MAP(), "@value" ) && has ( item->MAP(), "@list" ) ) throw INVALID_REVERSE_PROPERTY_VALUE;
						if ( !has ( reverseMap, exp_prop ) ) ( *reverseMap ) [ *exp_prop ] = mk_olist_obj();
						add_all ( reverseMap->at ( *exp_prop )->LIST(), item );
					}
				} else {
					if ( !has ( result, exp_prop ) ) ( *result ) [ *exp_prop ] = mk_olist_obj();
					add_all ( result->at ( *exp_prop )->LIST(), exp_val );
				}
			}
			if ( hasvalue ( result ) ) {
				//				 Set<String> keySet = new HashSet ( result.keySet() );
				somap ks ( *result );
				if ( hasvalue ( ks ) ) ks.erase ( "@value" );
				if ( hasindex ( ks ) ) ks.erase ( "@index" );
				bool langremoved = haslang ( ks );
				bool typeremoved = hastype ( ks );
				if ( langremoved ) ks.erase ( "@language" );
				if ( typeremoved ) ks.erase ( "@type" );
				if ( ( langremoved && typeremoved ) || ks.size() ) throw INVALID_VALUE_OBJECT + "\t" + "value object has unknown keys";
				pobj rval = getvalue ( result );
				if ( !rval ) return 0;
				if ( ! rval->STR() && haslang ( result ) ) throw INVALID_LANGUAGE_TAGGED_VALUE + "\t" + "when @language is used, @value must be a string";
				else if ( hastype ( result ) )
					if ( ! ( gettype ( result )->STR() ) || startsWith ( *gettype ( result )->STR(), "_:" ) || gettype ( result )->STR()->find ( ":" ) == string::npos )
						throw INVALID_TYPED_VALUE + "\t" + "value of @type must be an IRI";
			} else if ( hastype ( result ) ) {
				pobj rtype = gettype ( result );
				if ( !rtype->LIST() ) ( *result ) ["@type"] = mk_olist_obj ( olist ( 1, rtype ) ) ;
			} else if ( hasset ( result ) || haslist ( result ) ) {
				if ( result->size() > ( hasindex ( result ) ? 2 : 1 ) )
					throw INVALID_SET_OR_LIST_OBJECT + "\t" + "@set or @list may only contain @index";
				if ( hasset ( result ) ) return getset ( result );
			}
			if ( ( haslang ( result ) && result->size() == 1 ) ||  ( activeProperty == "@graph" ) && result && ( ( !result->size() || hasvalue ( result ) || haslist ( result ) ) ) ||
			        ( hasid ( result ) && result->size() == 1 ) ) result = 0;
			return mk_somap_obj ( result );
		}
		if ( activeProperty == "@graph" ) return 0;
		return activeCtx->expandValue ( activeProperty, element );
	}

	static bool deepCompare ( pobj v1, pobj v2, bool listOrderMatters = false ) {
		if ( !v1 ) return !v2;
		if ( !v2 ) return !v1;
		if ( v1->MAP() && v2->MAP() ) {
			psomap m1 = v1->MAP(), m2 = v2->MAP();
			if ( m1->size() != m2->size() ) return false;
			for ( auto x : *m1 ) if ( !has ( m2, x.first ) || !deepCompare ( x.second, m2->at ( x.first ), listOrderMatters ) ) return false;
			return true;
		} else if ( v1->LIST() && v2->LIST() ) {
			polist l1 = v1->LIST(), l2 = v2->LIST();
			if ( l1->size() != l2->size() ) return false;
			// used to mark members of l2 that we have already matched to avoid
			// matching the same item twice for lists that have duplicates
			bool *alreadyMatched = new bool[l2->size()];
			for ( size_t i = 0; i < l1->size(); ++i ) {
				pobj o1 = l1->at ( i );
				bool gotmatch = false;
				if ( listOrderMatters ) gotmatch = deepCompare ( o1, l2->at ( i ), listOrderMatters );
				else for ( size_t j = 0; j < l2->size(); j++ )
						if ( !alreadyMatched[j] && deepCompare ( o1, l2->at ( j ), listOrderMatters ) ) {
							alreadyMatched[j] = true;
							gotmatch = true;
							break;
						}
				delete[] alreadyMatched;
				if ( !gotmatch ) return false;
			}
			return true;
		} else return equals ( v1, v2 );
	}

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
	static void mergeValue ( somap obj, pstring key, pobj value ) {
		if ( key ) mergeValue ( obj, *key, value );
	}

	static void mergeValue ( somap obj, string key, pobj value ) {
		auto x = obj[key];
		polist values = x ? obj[key]->LIST() : 0;
		if ( !values ) obj[key] = mk_olist_obj ( values = mk_olist() );
		if ( key == "@list" || ( value->MAP() && has ( value->MAP(), "@list" ) ) || !deepContains ( values, value ) )
			values->push_back ( value );
	}

	void gen_node_map ( pobj element, psomap nodeMap )  {
		gen_node_map ( element, nodeMap, "@default", pobj(), pstring(), psomap() );
	}

	void gen_node_map ( pobj element, psomap nodeMap, string activeGraph ) {
		gen_node_map ( element, nodeMap, activeGraph, pobj(), pstring(), psomap() );
	}

	string gen_bnode_id ( string id = "" ) {
		if ( has ( bnode_id_map, id ) ) return bnode_id_map[id];
		string bnid = "_:b" + blankNodeCounter++;
		return bnode_id_map[id] = bnid;
	}

	size_t blankNodeCounter = 0;
	map<string, string> bnode_id_map;

	void gen_node_map ( pobj element, psomap nodeMap, string activeGraph, pobj activeSubject, pstring activeProperty, psomap list ) {
		if ( element->LIST() ) {
			for ( pobj item : *element->LIST() ) gen_node_map ( item, nodeMap, activeGraph, activeSubject, activeProperty, list );
			return;
		}
		psomap elem = element->MAP();
		if ( !has ( nodeMap, activeGraph ) ) ( *nodeMap ) [ activeGraph ] = mk_somap_obj();
		psomap graph = nodeMap->at ( activeGraph )->MAP(), node = activeSubject ? graph->at ( *activeSubject->STR() )->MAP() : 0;
		if ( hastype ( elem ) ) {
			vector<string> oldTypes, newTypes;
			if ( gettype ( elem )->LIST() ) oldTypes = vec2vec ( gettype ( elem )->LIST() );
			else {
				oldTypes = vector<string>();//mk_olist_obj();
				oldTypes.push_back ( *elem->at ( "@type" )->STR() );
			}
			for ( string item : oldTypes ) {
				if ( startsWith ( item, "_:" ) ) newTypes.push_back ( gen_bnode_id ( item ) );
				else newTypes.push_back ( item );
			}
			polist el = gettype ( elem )->LIST() ;
			if ( el ) ( *elem ) ["@type"] = mk_olist_obj ( vec2vec ( newTypes ) );
			else ( *elem ) ["@type"] = make_shared<string_obj> ( newTypes[0] );
		}
		if ( hasvalue ( elem ) ) {
			if ( !list ) mergeValue ( node, activeProperty, element );
			else mergeValue ( list, "@list", element );
		} else if ( haslist ( elem ) ) {
			psomap result = make_shared<somap>();
			( *result ) [ "@list"] = mk_olist_obj();
			gen_node_map ( getlist ( elem ), nodeMap, activeGraph, activeSubject, activeProperty, result );
			mergeValue ( node, activeProperty, mk_somap_obj ( result ) );
		} else {
			string id;
			if ( hasid ( elem ) && getid ( elem )->STR() ) {
				/*string*/ id = *elem->at ( "@id" )->STR();
				elem->erase ( "@id" );
				if ( startsWith ( id, "_:" ) ) id = gen_bnode_id ( id );
			} else id = gen_bnode_id ( );
			if ( !has ( graph, id ) ) {
				somap tmp;
				tmp[ "@id"] = make_shared<string_obj> ( id );
				( *graph ) [ id ] = mk_somap_obj ( tmp );
			}
			if ( activeSubject && activeSubject->MAP() ) mergeValue ( get ( graph, id )->MAP(), activeProperty, activeSubject );
			else if ( activeProperty ) {
				somap ref;
				ref[ "@id"] = make_shared<string_obj> ( id );
				if ( !list ) mergeValue ( node, activeProperty, mk_somap_obj ( ref ) );
				else mergeValue ( list, "@list", mk_somap_obj ( ref ) );
			}
			node = graph->at ( id )->MAP();
			if ( hastype ( elem ) ) {
				for ( pobj type : *gettype ( elem )->LIST() ) if ( type ) mergeValue ( node, "@type", type );
				elem->erase ( "@type" );
			}
			if ( hasindex ( elem ) ) {
				pobj elemIndex  = getindex ( elem );
				elem->erase ( "@index" );
				if ( hasindex ( node ) ) {
					if ( !deepCompare ( getindex ( node ), elemIndex ) ) throw CONFLICTING_INDEXES;
				} else ( *node ) ["@index"] = elemIndex ;
			}
			if ( hasreverse ( elem ) ) {
				psomap refnode = make_shared<somap>(), revmap = elem->at ( "@reverse" )->MAP();
				( *refnode ) ["@id"] = make_shared<string_obj> ( id );
				elem->erase ( "@reverse" );
				for ( auto x : *revmap ) {
					string prop = x.first;
					polist values = revmap->at ( prop )->LIST();
					for ( pobj value : *values ) gen_node_map ( value, nodeMap, activeGraph, mk_somap_obj ( refnode ), make_shared<string> ( prop ), 0 );
				}
			}
			if ( has ( elem, "@graph" ) ) {
				gen_node_map ( elem->at ( "@graph" ), nodeMap, id, 0, 0, 0 );
				elem->erase ( "@graph" );
			}
			//			final List<String> keys = new ArrayList<String> ( elem.keySet() );
			//			Collections.sort ( keys );
			for ( auto z : *elem ) {
				string property = z.first;
				pobj value = z.second;
				if ( startsWith ( property, "_:" ) ) property = gen_bnode_id ( property );
				if ( !has ( node, property ) ) ( *node ) [ property ] = mk_olist_obj();
				gen_node_map ( value, nodeMap, activeGraph, make_shared<string_obj> ( id ), make_shared<string> ( property ), 0 );
			}
		}
	}

	std::shared_ptr<rdf_db> toRDF();
};

class rdf_db : public qdb {
	//	static Pattern PATTERN_INTEGER = Pattern.compile ( "^[\\-+]?[0-9]+$" );
	//	static Pattern PATTERN_DOUBLE = Pattern .compile ( "^(\\+|-)?([0-9]+(\\.[0-9]*)?|\\.[0-9]+)([Ee](\\+|-)?[0-9]+)?$" );
	ssmap context;
	jsonld_api api;
public:
	rdf_db ( jsonld_api api_ = jsonld_api() ) : qdb(), api ( api_ ) {
		( *this ) [ "@default" ] = mk_qlist();
	}

	string tostring() {
		string s;
		stringstream o;
		for ( auto x : *this ) for ( pquad q : *x.second ) o << q->tostring ( x.first ) << endl;
		return o.str();
	}

	void setNamespace ( string ns, string prefix ) {
		context[ns] = prefix ;
	}
	string getNamespace ( string ns ) {
		return context[ ns ];
	}
	void clearNamespaces() {
		context.clear();
	}
	ssmap& getNamespaces() {
		return context;
	}
	somap getContext() {
		somap rval;
		for ( auto x : context ) rval[x.first] = mk_str_obj ( x.second );
		if ( has ( rval, "" ) ) {
			rval[ "@vocab"] = rval[""];
			rval.erase ( "" );
		}
		return rval;
	}

	void parseContext ( pobj contextLike ) {
		pcontext context;
		context = context->parse ( contextLike );
		ssmap prefixes = context->getPrefixes ( true );

		for ( auto x : prefixes ) {
			const string &key = x.first, &val = x.second;
			if ( key == "@vocab" ) setNamespace ( "", val );
			else if ( !keyword ( key ) ) setNamespace ( key, val );
		}
	}

	void addTriple ( string subj, string pred, string value, pstring datatype, pstring language ) {
		addquad ( subj, pred, value, datatype, language, pstr ( "@default" ) );
	}

	void addquad ( string s, string p, string value, pstring datatype, pstring language, pstring graph ) {
		if ( !graph ) graph = pstr ( "@default" );
		if ( find ( *graph ) == end() ) ( *this ) [ *graph ] = mk_qlist();
		at ( *graph )->push_back ( make_shared<quad> ( s, p, value, datatype, language, graph ) );
	}

	void addTriple ( string subj, string pred, string object ) {
		addquad ( subj, pred, object, pstr ( "@default" ) );
	}

	void addquad ( string subj, string pred, string object, pstring graph ) {
		if ( !graph ) graph = pstr ( "@default" );
		if ( !has ( *this, *graph ) ) ( *this ) [ *graph ] = mk_qlist();
		at ( *graph )->push_back ( make_shared<quad> ( subj, pred, object, graph ) );
	}

	void graphToRDF ( string graph_name, somap& graph ) {
		qlist triples;
		for ( auto y : graph ) {
			string id = y.first;
			if ( is_rel_iri ( id ) ) continue;
			psomap node = y.second->MAP();
			for ( auto x : *node ) {
				string property = x.first;
				polist values;
				if ( property == "@type" ) {
					values =  gettype ( node )->LIST();
					property = RDF_TYPE;
				} else if ( keyword ( property ) ) continue;
				else if ( startsWith ( property, "_:" ) && !api.opts.produceGeneralizedRdf ) continue;
				else if ( is_rel_iri ( property ) ) continue;
				else values = node->at ( property )->LIST();

				pnode subj = id.find ( "_:" ) ? mkiri ( id ) : mkbnode ( id );
				pnode pred = startsWith ( property, "_:" ) ? mkbnode ( property ) :  mkiri ( property );

				for ( auto item : *values ) {
					// convert @list to triples
					if ( item->MAP() && haslist ( item->MAP() ) ) {
						polist list =  item->MAP( )->at ( "@list" )->LIST();
						pnode last = 0;
						pnode firstBnode = nil;
						if ( list->size() ) {
							last = objectToRDF ( *list->rbegin() );
							firstBnode = mkbnode ( api.gen_bnode_id () );
						}
						triples.push_back ( make_shared<quad> ( subj, pred, firstBnode, pstr ( graph_name ) ) );
						for ( size_t i = 0; i < list->size() - 1; i++ ) {
							pnode object = objectToRDF ( list->at ( i ) );
							triples.push_back ( make_shared<quad> ( firstBnode, first, object, pstr ( graph_name ) ) );
							pnode restBnode = mkbnode ( api.gen_bnode_id() );
							triples.push_back ( make_shared<quad> ( firstBnode, rest, restBnode, pstr ( graph_name ) ) );
							firstBnode = restBnode;
						}
						if ( last ) {
							triples.push_back ( make_shared<quad> ( firstBnode, first, last, pstr ( graph_name ) ) );
							triples.push_back ( make_shared<quad> ( firstBnode, rest, nil, pstr ( graph_name ) ) );
						}
					} else if ( pnode object = objectToRDF ( item ) ) triples.push_back ( make_shared<quad> ( subj, pred, object, pstr ( graph_name ) ) );
				}
			}
		}
		( *this ) [ graph_name ] = make_shared<qlist> ( triples );
	}

private:
	pnode objectToRDF ( pobj item ) {
		if ( isvalue ( item ) ) {
			pobj value = item->MAP( )->at ( "@value" ), datatype = hastype ( item->MAP() ) ? item->MAP( )->at ( "@type" ) : pobj ( 0 );
			if ( value->BOOL() || value->INT() || value->UINT() || value->DOUBLE() ) {
				if ( value->BOOL() ) return mkliteral ( *value->BOOL() ? "(true)" : "(false)", pstr ( datatype ? *datatype->STR() : XSD_BOOLEAN ),  0 );
				else if ( value->DOUBLE() || XSD_DOUBLE == *datatype->STR() ) {
					///					DecimalFormat df = new DecimalFormat ( "0.0###############E0" );
					//					df.setDecimalFormatSymbols ( DecimalFormatSymbols.getInstance ( Locale.US ) );
					return mkliteral ( tostr ( *value->DOUBLE() ), pstr ( datatype ? *datatype->STR() : XSD_DOUBLE ), 0 );
				} else {
					//DecimalFormat df = new DecimalFormat ( "0" );
					return mkliteral ( value->INT() ? tostr ( *value->INT() ) : tostr ( *value->UINT() ), pstr ( datatype ? *datatype->STR() : XSD_INTEGER ), 0 );
				}
			} else if ( haslang ( item->MAP() ) )
				return mkliteral ( *value->STR(), pstr ( datatype ? *datatype->STR() : RDF_LANGSTRING ), getlang ( item )->STR() );
			else return mkliteral ( *value->STR(), pstr ( datatype ? *datatype->STR() : XSD_STRING ), 0 );
		}
		// convert string/node object to RDF
		else {
			string id;
			if (  item->MAP( ) ) {
				id = * getid ( item )->STR();
				if ( is_rel_iri ( id ) ) return 0;
			} else id = * item->STR();
			if ( id.find ( "_:" ) == 0 ) return mkbnode ( id );
			else return mkiri ( id );
		}
	}

public:
	/*	 set<string> graph_names() {
			return keySet();
		}
		vector<quad> getquads ( string graph_name ) {
			return ( List<quad> ) get ( graph_name );
		}*/
};

std::shared_ptr<rdf_db> jsonld_api::toRDF() {
	psomap nodeMap = make_shared<somap>();
	( *nodeMap ) ["@default"] = mk_somap_obj();
	gen_node_map ( value, nodeMap );
	rdf_db r;
	for ( auto g : *nodeMap ) {
		if ( is_rel_iri ( g.first ) ) continue;
		r.graphToRDF ( g.first, *g.second->MAP() );
	}
	return make_shared<rdf_db> ( r );
}


std::shared_ptr<rdf_db> to_rdf ( jsonld_api& a, pobj o ) {
	//	psomap node_map = make_shared<somap>();
	//	a.gen_node_map ( o, node_map );
	return a.toRDF();
}

qdb load_jsonld( string fname ) {
	jsonld_options o;
	/*pstring*/ o.base = 0;
	/*pbool*/ o.compactArrays = make_shared<bool> ( true );
	//	/*pobj*/ o.expandContext = 0;
	/*pstring*/ o.processingMode = pstr ( "json-ld-1.0" );
	/*pbool*/ o.embed = 0;
	/*pbool*/ o.isexplicit = 0;
	/*pbool*/ o.omitDefault = 0;
	/*pbool*/ o.useRdfType = make_shared<bool> ( true );
	/*pbool*/ o.useNativeTypes = make_shared<bool> ( true );
	/*pbool*/ o.produceGeneralizedRdf = make_shared<bool> ( true );
	/*pstring*/ o.format = 0;
	/*pbool*/ o.useNamespaces = make_shared<bool> ( true );
	/*pstring*/ o.outputForm = 0;

	json_spirit::mValue v;
	ifstream ifs ( fname );
	json_spirit::read_stream ( ifs, v );
	auto c = convert ( v );
	jsonld_api a ( c, o );
	auto r = *a.toRDF ( );
	cout<<"Loaded graphs:"<<endl;
	for (auto x : r) cout<<x.first<<endl;
//	cout << r.tostring() << endl;
	return r;
}
}
