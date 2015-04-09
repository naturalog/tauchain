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
#include "loader.h"

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

// http://www.w3.org/TR/json-ld-api/#the-jsonldoptions-type
struct jsonld_options {
	jsonld_options() {}
	jsonld_options ( string base_ ) : base ( pstr ( base_ ) ) {}
	pstring base = 0;
	pbool compactArrays = make_shared<bool> ( true );
	pobj expandContext = 0;
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

polist vec2vec ( const vector<string>& x ) {
	polist res = make_shared<olist>();
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
	o = make_shared<olist_obj> ( olist ( 1, t ) );
}

void add_all ( polist l, pobj v ) {
	if ( v->LIST() ) l->insert ( l->end(), v->LIST()->begin(), v->LIST()->end() );
	else l->push_back ( v );
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
		if ( !localContext->LIST() ) localContext = make_shared<olist_obj> ( olist ( 1, localContext ) );
		for ( auto context : *localContext->LIST() ) {
			if ( context->Null() ) result = context_t ( options );
			else if ( pstring s = context->STR() ) {
				string uri = resolve ( * ( *result.MAP() ) ["@base"]->STR(), *s ); // REVISE
				if ( std::find ( remoteContexts.begin(), remoteContexts.end(), uri ) != remoteContexts.end() ) throw RECURSIVE_CONTEXT_INCLUSION + "\t" + uri;
				remoteContexts.push_back ( uri );

				pobj remoteContext = fromURL ( uri );
				somap* p;
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
				else if ( pstring s = value->STR() ) ( *result.MAP() ) ["@language"] = make_shared<string_obj> ( lower ( *s ) );
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
			( *term_defs ) [term] = make_shared<somap_obj> ( defn );
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
		return td == term_defs->end() || !td->second->MAP() ? 0 : td->second->MAP()->at ( "@type" )->STR();
	}

	pstring get_lang_map ( const string& prop ) {
		auto td = term_defs->find ( prop );
		return td == term_defs->end() || !td->second->MAP() ? 0 : td->second->MAP()->at ( "@language" )->STR();
	}

	psomap get_term_def ( const string& key ) {
		return term_defs->at ( key )->MAP();
	}

	// http://json-ld.org/spec/latest/json-ld-api/#inverse-context-creation
	psomap_obj getInverse() {
		if ( inverse ) return inverse;
		inverse = make_shared<somap_obj>();
		pstring defaultLanguage = MAP()->at ( "@language" )->STR();
		if ( !defaultLanguage ) defaultLanguage = pstr ( "@none" );

		for ( auto x : *term_defs ) {
			string term = x.first;
			auto it = term_defs->find ( term );
			psomap definition = it == term_defs->end() || !it->second ? 0 : it->second->MAP();
			if ( !definition ) continue;
			pstring container = ( ( it = definition->find ( "@container" ) ) == definition->end() || !it->second ) ? 0 : it->second->STR();
			if ( !container ) container = pstr ( "@none" );
			pstring iri = ( ( it = definition->find ( "@id" ) ) == definition->end() ) || !it->second ? 0 : it->second->STR();

			psomap_obj containerMap = make_shared<somap_obj> ( iri ? inverse->MAP()->at ( *iri )->MAP() : 0 );
			if ( !containerMap ) {
				containerMap = make_shared<somap_obj>();
				inverse->MAP()->at ( *iri ) = containerMap;
			}
			psomap_obj typeLanguageMap = make_shared<somap_obj> ( container ? containerMap->MAP()->at ( *container )->MAP() : 0 );
			if ( !typeLanguageMap ) {
				typeLanguageMap = make_shared<somap_obj>();
				typeLanguageMap->MAP()->at ( "@language" ) = make_shared<somap_obj>();
				typeLanguageMap->MAP()->at ( "@type" ) = make_shared<somap_obj>();
				containerMap->MAP()->at ( *container ) = typeLanguageMap;
			}
			if ( definition->at ( "@reverse" )->BOOL() ) {
				psomap typeMap = typeLanguageMap->MAP()->at ( "@type" )->MAP();
				if ( !has ( typeMap, "@reverse" ) ) typeMap->at ( "@reverse" ) = make_shared<string_obj> ( term );
			} else if ( has ( definition, "@type" ) ) {
				psomap typeMap = typeLanguageMap->MAP()->at ( "@type" )->MAP();
				if ( !has ( typeMap, definition->at ( "@type" )->STR() ) ) typeMap->at ( *definition->at ( "@type" )->STR() ) = make_shared<string_obj> ( term );
			} else if ( has ( definition, "@language" ) ) {
				psomap languageMap = typeLanguageMap->MAP()->at ( "@type" )->MAP();
				pstring language = definition->at ( "@language" )->STR();
				if ( !language ) language = pstr ( "@null" );
				if ( !has ( languageMap, language ) ) languageMap->at ( *language ) = make_shared<string_obj> ( term );
			} else {
				psomap languageMap = typeLanguageMap->MAP()->at ( "@language" )->MAP();
				if ( !has ( languageMap, "@language" ) ) languageMap->at ( "@language" ) = make_shared<string_obj> ( term );
				if ( !has ( languageMap, "@none" ) ) languageMap->at ( "@none" ) = make_shared<string_obj> ( term );
				psomap typeMap = typeLanguageMap->MAP()->at ( "@type" )->MAP();
				if ( !has ( typeMap, "@none" ) ) typeMap->at ( "@none" ) = make_shared<string_obj> ( term );
			}
		}
		return inverse;
	}

	bool isvalue ( pobj v ) {
		return v && v->MAP() && has ( v->MAP(), "@value" );
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
			auto typeLanguageMap = containerMap->at ( container )->MAP();
			auto valueMap = typeLanguageMap ->at ( typeLanguage )->MAP();
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
					if ( has ( value->MAP(),  "@language" )
					        && ! has ( value->MAP(),  "@index" ) ) {
						containers.push_back ( "@language" );
						type_lang_val = value->MAP( )->at ( "@language" )->STR();
					} else if ( has ( value->MAP(),  "@type" ) ) {
						type_lang = pstr ( "@type" );
						type_lang_val = value->MAP( )->at ( "@type" )->STR();
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
				        && ::equals ( value->MAP( )->at ( "@id" ),
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
		if ( relativeToVocab && has ( MAP(), "@vocab" ) ) {
			pstring vocab = MAP()->at ( "@vocab" )->STR();
			if ( vocab && iri.find ( *vocab ) == 0 && iri != *vocab ) {
				string suffix = iri.substr ( vocab->length() );
				if ( !has ( term_defs, suffix ) ) return pstr ( suffix );
			}
		}
		pstring compactIRI = 0;
		for ( auto x : *term_defs ) {
			string term = x.first;
			psomap termDefinition = x.second->MAP();
			if ( term.find ( ":" ) != string::npos ) continue;
			if ( !termDefinition// || iri == termDefinition->at ( "@id" )->STR()
			        || !startsWith ( iri,  *termDefinition->at ( "@id" )->STR() ) )
				continue;

			string candidate = term + ":" + iri.substr ( termDefinition->at ( "@id" )->STR()->length() );
			// TODO: verify && and ||
			if ( ( !compactIRI || compareShortestLeast ( candidate, compactIRI ) < 0 )
			        && ( !has ( term_defs, candidate ) || ( iri == *term_defs->at ( candidate )->MAP() ->at ( "@id" )->STR() ) && !value ) )
				compactIRI = pstr ( candidate );
		}
		if ( !compactIRI  ) return compactIRI;
		if ( !relativeToVocab ) return removeBase ( MAP()->at ( "@base" ), iri );
		return make_shared<string> ( iri );
	}

	// http://json-ld.org/spec/latest/json-ld-api/#value-compaction
	pobj compactValue ( string activeProperty, psomap_obj value_ ) {
		psomap value = value_->MAP();
		int nvals = value->size();
		auto p = getContainer ( activeProperty );
		if ( value->find ( "@index" ) != value->end() && p && *p == "@index" ) nvals--;
		if ( nvals > 2 ) return value_;//make_shared<somap_obj> ( value_ );
		pstring type_map = get_type_map ( activeProperty ), lang_map = get_lang_map ( activeProperty );
		auto it = value->find ( "@id" );
		if ( it != value->end() && nvals == 1 )
			if ( type_map && *type_map == "@id" && nvals == 1 )
				return make_shared<string_obj> ( compactIri ( value->at ( "@id" )->STR(), 0, false, false ) );
			else {
				if ( type_map && *type_map == "@vocab" && nvals == 1 )
					return make_shared<string_obj> ( compactIri ( value->at ( "@id" )->STR(), 0, true, false ) );
				else return  make_shared<somap_obj> ( value );
			}
		pobj valval = value->at ( "@value" );
		it = value->find ( "@type" );
		if ( it != value->end() &&  *it->second->STR() == *type_map  ) return valval;
		if ( ( it = value->find ( "@language" ) ) != value->end() ) // TODO: SPEC: doesn't specify to check default language as well
			if ( *it->second->STR() == * lang_map  || ::equals ( it->second, MAP()->at ( "@language" ) ) )
				return valval;
		if ( nvals == 1
		        && ( !valval->STR() || has ( MAP(), "@language" ) || ( term_defs->find ( activeProperty ) == term_defs->end()
		                && has ( get_term_def ( activeProperty ), "@language" ) && !lang_map ) ) )
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
		if ( td && *td->at ( "@type" )->STR() == "@id" ) {
			rval[ "@id" ] = make_shared<string_obj> ( expandIri ( value->toString(), true, false, 0, 0 ) );
			return make_shared<somap_obj> ( rval );
		}
		if ( td && *td->at ( "@type" )->STR() == "@vocab" ) {
			rval[ "@id" ] = make_shared<string_obj> ( expandIri ( value->toString(), true, true, 0, 0 ) );
			return make_shared<somap_obj> ( rval );
		}
		rval[ "@value" ] = value;
		if ( td && has ( td, "@type" ) ) rval[ "@type" ] = td->at ( "@type" );
		else if ( value->STR() ) {
			if ( td && has ( td, "@language" ) ) {
				pstring lang = td->at ( "@language" )->STR();
				if ( lang ) rval[ "@language"] = make_shared<string_obj> ( lang );
			} else if ( has ( MAP(), "@language" ) ) rval[ "@language" ] = MAP()->at ( "@language" );
		}
		return make_shared<somap_obj> ( rval );
	}
};

typedef std::shared_ptr<context_t> pcontext;
#include "jsonldapi.h"

int main() {
	context_t c;
	vector<string> v;
	c.parse ( 0, v );
	return 0;
}
