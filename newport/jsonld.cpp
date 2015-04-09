#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <utility>
#include <memory>
#include <list>
#include <boost/variant.hpp>
#include "json_spirit_reader.h"
#include "json_spirit_writer.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/easy.h>
#include <curl/curlbuild.h>
#include <sstream>
#include <iostream>

using namespace std;
using namespace std::string_literals;
using namespace boost;

#include "err.h"
#include "util.h"

class null {};

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
};

#define OBJ_IMPL(type, getter) \
class type##_obj : public obj { \
	std::shared_ptr<type> data; \
public: \
	type##_obj(const type& o = type()) { data = make_shared<type>(); *data = o; } \
	type##_obj(const std::shared_ptr<type> o) : data(o) { } \
	virtual std::shared_ptr<type> getter() { return data; } \
}

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
typedef map<string, bool> defined_t;
typedef std::shared_ptr<defined_t> pdefined_t;

string resolve ( const string&, const string& );

struct remote_doc_t {
	string url;
	pstring context_url;
	pobj document;
	remote_doc_t ( string url_, const pobj& document_ = 0, pstring context = 0 ) : url ( url_ ), document ( document_ ), context_url ( context ) { }
};

class doc_loader_t {
	void* curl;
public:
	remote_doc_t loadDocument ( const string& url )  {
		pobj p;
		remote_doc_t doc( url, p );
		try {
			doc.setDocument ( fromURL ( /*new URL*/ ( url ) ) );
		} catch ( ... ) {
			throw JsonLdError ( LOADING_REMOTE_CONTEXT_FAILED, url );
		}
		return doc;
	}

	pobj fromURL ( const string& url ) {
		json_spirit::mValue r;
		json_spirit::read ( download ( url ), r );

		return r;
	}

	static size_t write_data ( void *ptr, size_t size, size_t n, void *stream ) {
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

	doc_loader_t() {
		curl = curl_easy_init();
	}
	virtual ~doc_loader_t() {
		curl_easy_cleanup ( curl );
	}
};

// http://www.w3.org/TR/json-ld-api/#the-jsonldoptions-type
struct options {
    options() {}
    options(string base_) : base(make_shared<string>(base_)){}
    pstring base = 0;
    pbool compactArrays = make_shared<bool>(true);
    pobj expandContext = 0;
    pstring processingMode = "json-ld-1.0";
    doc_loader_t doc_loader;
    pbool embed = 0;
    pbool explicit = 0;
    pbool omitDefault = 0;
    pbool useRdfType = make_shared<bool>(false);
    pbool useNativeTypes = make_shared<bool>(false);
    pbool produceGeneralizedRdf = make_shared<bool>(false);
    pstring format = 0;
    pbool useNamespaces = make_shared<bool>(false);
    pstring outputForm = 0;
}

bool keyword ( const string& key ) {
	return "@base"s == key || "@context"s == key || "@container"s == key
	       || "@default"s == key || "@embed"s == key || "@explicit"s == key
	       || "@graph"s == key || "@id"s == key || "@index"s == key
	       || "@language"s == key || "@list"s == key || "@omitDefault"s == key
	       || "@reverse"s == key || "@preserve"s == key || "@set"s == key
	       || "@type"s == key || "@value"s == key || "@vocab"s == key;
}

bool keyword ( pobj p ) {
	if ( !p || !p->STR() ) return false;
	return keyword ( *p->STR() );
}

bool is_abs_iri ( const string& s ) {
	return s.find ( ':' ) != string::npos;
}
bool is_rel_iri ( const string& s ) {
	return !keyword ( s ) || !is_abs_iri ( s );
}
pobj newMap ( const string& k, pobj v ) {
	pobj r = make_shared<somap_obj>();
	( *r->MAP() ) [k] = v;
	return r;
}

//template<typename obj>
class Context : public somap {
public:
	pobj load ( const string& );
private:
	jsonld_options options;
	psomap term_defs = make_shared<somap>();
public:
	psomap inverse = 0;

	Context();
	Context ( const jsonld_options& );

	//Context Processing Algorithm http://json-ld.org/spec/latest/json-ld-api/#context-processing-algorithms
	Context parse ( pobj localContext, vector<string>& remoteContexts ) {
		Context result ( *this );
		if ( !localContext->LIST() ) localContext = make_shared<olist_obj> ( olist ( 1, localContext ) );
		for ( auto context : *localContext->LIST() ) {
			if ( context->Null() ) result = Context ( options );
			else if ( pstring s = context->STR() ) {
				string uri = resolve ( *result["@base"]->STR(), *s ); // REVISE
				if ( std::find ( remoteContexts.begin(), remoteContexts.end(), uri ) != remoteContexts.end() ) throw RECURSIVE_CONTEXT_INCLUSION + "\t" + uri;
				remoteContexts.push_back ( uri );

				pobj remoteContext = load ( uri );
				somap* p;
				if ( !remoteContext->map_and_has ( "@context" ) ) throw INVALID_REMOTE_CONTEXT + "\t"; // + context;
				context = ( *remoteContext->MAP() ) ["@context"];
				result = result.parse ( context, remoteContexts );
				continue;
			} else if ( !context->MAP() ) throw INVALID_LOCAL_CONTEXT + "\t"; // + context;
			somap& cm = *context->MAP();
			auto it = cm.find ( "@base" );
			if ( !remoteContexts.size() && it != cm.end() ) {
				pobj value = it->second;
				if ( value->Null() ) result.erase ( "@base" );
				else if ( pstring s = value->STR() ) {
					if ( is_abs_iri ( *s ) ) result["@base"] = value;
					else {
						string baseUri = *result["@base"]->STR();
						if ( !is_abs_iri ( baseUri ) ) throw INVALID_BASE_IRI + "\t" + baseUri;
						result["@base"] = make_shared<string_obj> ( resolve ( baseUri, *s ) );
					}
				} else throw INVALID_BASE_IRI + "\t" + "@base must be a string";
			}
			// 3.5
			if ( ( it = cm.find ( "@vocab" ) ) != cm.end() ) {
				pobj value = it->second;
				if ( value->Null() ) result.erase ( it );
				else if ( pstring s = value->STR() ) {
					if ( is_abs_iri ( *s ) ) result["@vocab"] = value;
					else throw INVALID_VOCAB_MAPPING + "\t" + "@value must be an absolute IRI";
				} else throw INVALID_VOCAB_MAPPING + "\t" + "@vocab must be a string or null";
			}
			if ( ( it = cm.find ( "@language" ) ) != cm.end() ) {
				pobj value = it->second;
				if ( value->Null() ) result.erase ( it );
				else if ( pstring s = value->STR() ) result["@language"] = make_shared<string_obj> ( lower ( *s ) );
				else throw INVALID_DEFAULT_LANGUAGE + "\t";// + value;
			}
			pdefined_t defined = make_shared<defined_t>();
			for ( auto it : cm ) {
				if ( is ( it.first, { "@base"s, "@vocab"s, "@language"s } ) ) continue;
				result.createTermDefinition ( context->MAP(), it.first, defined ); // REVISE
			}
		}
		return result;
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
			( *term_defs ) [term] = make_shared<somap_obj> ( defn );
			return;
		}
		defn["@reverse"] = make_shared<bool_obj> ( false );
		size_t colIndex;
		if ( ( it = val.find ( "@id" ) ) != val.end() && it->second->STR() && *it->second->STR() != term ) {
			if ( ! it->second->STR() ) throw INVALID_IRI_MAPPING + "expected value of @id to be a string";
			pstring res = expandIri ( it->second->STR(), false, true, context, pdefined );
			if ( res && keyword ( *res ) || is_abs_iri ( *res ) ) {
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
		} else if ( ( it = find ( "@vocab" ) ) != end() )
			defn ["@id"] = make_shared<string_obj> ( *at ( "@vocab" )->STR() + term );
		else throw INVALID_IRI_MAPPING + "relative term defn without vocab mapping";

		// 16
		bool tmp = ( ( it = val.find ( "@container" ) ) != val.end() ) && it->second->STR() &&
		           is ( *it->second->STR(), { "@list"s, "@set"s, "@index"s, "@language"s }, INVALID_CONTAINER_MAPPING + "@container must be either @list, @set, @index, or @language" ) && ( defn["@container"] = it->second );

		auto i1 = val.find ( "@language" ), i2 = val.find ( "type" );
		pstring lang;
		if ( i1 != val.end() && i2 == val.end() ) {
			if ( !i1->second->Null() || ( lang = i2->second->STR() ) ) defn["@language"] = lang ? make_shared<string_obj> ( lower ( *lang ) ) : 0;
			else throw INVALID_LANGUAGE_MAPPING + "@language must be a string or null";
		}

		( *term_defs ) [term] = make_shared<somap_obj> ( defn );
		( *pdefined ) [term] = true;
	}
	//http://json-ld.org/spec/latest/json-ld-api/#iri-expansion
	pstring expandIri ( const pstring value, bool relative, bool vocab, const psomap context, pdefined_t pdefined ) {
		if ( !value || keyword ( *value ) ) return value;
		const defined_t& defined = *pdefined;
		if ( context && has ( context, value ) && !defined.at ( *value ) ) createTermDefinition ( context, *value, pdefined );
		if ( vocab && has ( term_defs, value ) ) {
			psomap td;
			return ( td = term_defs->at ( *value )->MAP() ) ? td->at ( "@id" )->STR() : 0;
		}
		size_t colIndex = value->find ( ":" );
		if ( colIndex != string::npos ) {
			string prefix = value->substr ( 0, colIndex ), suffix = value->substr ( colIndex + 1 );
			if ( prefix == "_" || startsWith ( suffix, "//" ) ) return value;
			if ( context && has ( context, prefix ) && ( !has ( pdefined, prefix ) || !defined.at ( prefix ) ) )
				createTermDefinition ( context, prefix, pdefined );
			if ( has ( term_defs, prefix ) ) return make_shared<string> ( *term_defs->at ( prefix )->MAP()->at ( "@id" )->STR() + suffix );
			return value;
		}
		if ( vocab && find ( "@vocab" ) != end() ) return make_shared<string> ( *at ( "@vocab" )->STR() + *value );
		if ( relative ) return make_shared<string> ( resolve ( *at ( "@base" )->STR(), *value ) );
		if ( context && is_rel_iri ( *value ) ) throw INVALID_IRI_MAPPING + "not an absolute IRI: " + *value;
		return value;
	}
};


int main() {
	Context c;
	vector<string> v;
	c.parse ( 0, v );
	return 0;
}
