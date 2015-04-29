#ifndef __STRINGS_H__
#define __STRINGS_H__

#include <string>
using namespace std;

const string str_base = "@base";
const string str_context = "@context";
const string str_embed = "@embed";
const string str_default = "@default";
const string str_container = "@container";
const string str_graph = "@graph";
const string str_id = "@id";
const string str_list = "@list";
const string str_index = "@index";
const string str_lang = "@language";
const string str_reverse = "@reverse";
const string str_type = "@type";
const string str_value = "@value";
const string str_preserve = "@preserve";
const string str_omitDefault = "@omitDefault";
const string str_vocab = "@vocab";
const string str_explicit = "@explicit";
const string str_set = "@set";
const string str_embedChildren = "@embedChildren";
const string tab = "\t";

const string RDF_SYNTAX_NS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
const string RDF_SCHEMA_NS = "http://www.w3.org/2000/01/rdf-schema#";
const string XSD_NS = "http://www.w3.org/2001/XMLSchema#";
const string XSD_ANYTYPE = XSD_NS + "anyType";
const string XSD_BOOLEAN = XSD_NS + "boolean";
const string XSD_DOUBLE = XSD_NS + "double";
const string XSD_INTEGER = XSD_NS + "integer";
const string XSD_FLOAT = XSD_NS + "float";
const string XSD_DECIMAL = XSD_NS + "decimal";
const string XSD_ANYURI = XSD_NS + "anyURIstring";
const string XSD_STRING = XSD_NS + "string";
const string RDF_TYPE = RDF_SYNTAX_NS + "type";
const string RDF_FIRST = RDF_SYNTAX_NS + "first";
const string RDF_REST = RDF_SYNTAX_NS + "rest";
const string RDF_NIL = RDF_SYNTAX_NS + "nil";
const string RDF_PLAIN_LITERAL = RDF_SYNTAX_NS + "PlainLiteral";
const string RDF_XML_LITERAL = RDF_SYNTAX_NS + "XMLLiteral";
const string RDF_OBJECT = RDF_SYNTAX_NS + "object";
const string RDF_LANGSTRING = RDF_SYNTAX_NS + "langString";
const string RDF_LIST = RDF_SYNTAX_NS + "List";

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
const string INVALID_LANGUAGE_TAGGED_STRING =
    "invalid language-tagged const String";
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

const auto Ex1 = std::runtime_error ( INVALID_IRI_MAPPING + string ( "expected value of @id to be a string" ) );
const auto Ex2 = std::runtime_error ( INVALID_KEYWORD_ALIAS + string ( "cannot alias @context" ) );
const auto Ex3 = std::runtime_error ( INVALID_IRI_MAPPING + string ( "resulting IRI mapping should be a keyword, absolute IRI or blank node" ) );
const auto Ex4 = std::runtime_error ( INVALID_IRI_MAPPING + string ( "relative term defn without vocab mapping" ) );
const auto Ex5 = std::runtime_error ( INVALID_IRI_MAPPING + tab + string ( "Expected String for @reverse value." ) );
const auto Ex6 = INVALID_REVERSE_PROPERTY + string ( "reverse properties only support set- and index-containers" );
const auto Ex7 = std::runtime_error ( INVALID_VOCAB_MAPPING + tab + string ( "@value must be an absolute IRI" ) );
const auto Ex8 = std::runtime_error ( INVALID_VOCAB_MAPPING + tab + string ( "@vocab must be a string or null" ) );
const auto Ex9 = std::runtime_error ( INVALID_BASE_IRI + tab + string ( "@base must be a string" ) );
const auto Ex10 = INVALID_CONTAINER_MAPPING + string ( "@container must be either @list, @set, @index, or @language" );
const auto Ex11 = std::runtime_error ( INVALID_LANGUAGE_MAPPING + string ( "@language must be a string or null" ) );
const auto Ex12 = std::runtime_error ( INVALID_REVERSE_PROPERTY_MAP + string ( tab ) + string ( "a keyword cannot be used as a @reverse propery" ) );
const auto Ex13 = std::runtime_error ( INVALID_ID_VALUE + tab + string ( "value of @id must be a string" ) );
const auto Ex14 = std::runtime_error ( INVALID_TYPE_VALUE + tab + string ( "@type value must be a string or array of strings" ) );
const auto Ex15 = std::runtime_error ( INVALID_TYPE_VALUE + tab + string ( "@type value must be a an empty object for framing" ) );
const auto Ex16 = std::runtime_error ( INVALID_TYPE_VALUE + tab + string ( "@type value must be a string or array of strings" ) );
const auto Ex17 = std::runtime_error ( LIST_OF_LISTS + tab + string ( "A list may not contain another list" ) );
const auto Ex18 = std::runtime_error ( INVALID_REVERSE_VALUE + tab + string ( "@reverse value must be an object" ) );
const auto Ex19 = std::runtime_error ( LIST_OF_LISTS + tab + string ( "lists of lists are not permitted." ) );

const string implication = "http://www.w3.org/2000/10/swap/log#implies";

inline bool endsWith ( const string& x, const string& y ) {
	return x.size() >= y.size() && x.substr ( x.size() - y.size(), y.size() ) == y;
}
inline bool startsWith ( const string& x, const string& y ) {
	return x.size() >= y.size() && x.substr ( 0, y.size() ) == y;
}

template<typename T>
inline bool is ( const T& s, const std::vector<T>& v, std::string exception =
                     std::string() ) {
	bool rc = std::find ( v.begin(), v.end(), s ) != v.end();
	if ( exception.size() && !rc )
		throw std::runtime_error ( exception );
	return rc;
}

template<typename T> inline bool is ( const std::shared_ptr<T>& s,
                                      const std::vector<T>& v, std::string exception = std::string() ) {
	return is<T> ( *s, v, exception );
}

inline string lower ( const string& s_ ) {
	string s = s_;
	std::transform ( s.begin(), s.end(), s.begin(), ::tolower );
	return s;
}

template<typename charT> inline vector<string> split ( const string& s, charT c ) {
	vector<string> v;
	for ( string::size_type i = 0, j = s.find ( c ); j != string::npos; ) {
		v.push_back ( s.substr ( i, j - i ) );
		j = s.find ( c, i = ++j );
		if ( j == string::npos )
			v.push_back ( s.substr ( i, s.length() ) );
	}
	return v;
}

template<typename T> string tostr ( T t ) {
	stringstream s;
	s << t;
	return s.str();
}

#endif
