#ifndef __STRINGS_H__
#define __STRINGS_H__

#include <string>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <memory>
//using namespace std;

typedef std::wstring string;
using std::map;
using std::stringstream;
using std::shared_ptr;
using std::make_shared;
using std::istream;
typedef std::shared_ptr<string> pstring;
pstring pstr ( const string& s );
inline pstring pstr ( const wchar_t* s ) { return s ? pstr ( string ( s ) ) : 0; }

#ifdef OPENCL
#define CL(x) x
#else
#define CL(x)
#endif

class wruntime_error : public std::exception {
	string msg;
public:
	using std::exception::exception;
	wruntime_error(string s) : msg(s){}
	virtual const char* what() const noexcept {
		return std::string(msg.begin(), msg.end()).c_str();
	}
};

const string str_base = L"@base";
const string str_context = L"@context";
const string str_embed = L"@embed";
const string str_default = L"@default";
const string str_container = L"@container";
const string str_graph = L"@graph";
const string str_id = L"@id";
const string str_list = L"@list";
const string str_index = L"@index";
const string str_lang = L"@language";
const string str_reverse = L"@reverse";
const string str_type = L"@type";
const string str_value = L"@value";
const string str_preserve = L"@preserve";
const string str_omitDefault = L"@omitDefault";
const string str_vocab = L"@vocab";
const string str_explicit = L"@explicit";
const string str_set = L"@set";
const string str_embedChildren = L"@embedChildren";
const string tab = L"\t";

const string RDF_SYNTAX_NS = L"http://www.w3.org/1999/02/22-rdf-syntax-ns#";
const string RDF_SCHEMA_NS = L"http://www.w3.org/2000/01/rdf-schema#";
const string XSD_NS = L"http://www.w3.org/2001/XMLSchema#";
const pstring XSD_ANYTYPE = pstr(XSD_NS + L"anyType");
const pstring XSD_BOOLEAN = pstr(XSD_NS + L"boolean");
const pstring XSD_DOUBLE = pstr(XSD_NS + L"double");
const pstring XSD_INTEGER = pstr(XSD_NS + L"integer");
const pstring XSD_FLOAT = pstr(XSD_NS + L"float");
const pstring XSD_DECIMAL = pstr(XSD_NS + L"decimal");
const pstring XSD_ANYURI = pstr(XSD_NS + L"anyURIstring");
const pstring XSD_STRING = pstr(XSD_NS + L"string");
const pstring XSD_PTR = pstr(XSD_NS + L"pointer");
const pstring RDF_TYPE = pstr(RDF_SYNTAX_NS + L"type");
const pstring RDF_FIRST = pstr(RDF_SYNTAX_NS + L"first");
const pstring RDF_REST = pstr(RDF_SYNTAX_NS + L"rest");
const pstring RDF_NIL = pstr(RDF_SYNTAX_NS + L"nil");
const string RDF_PLAIN_LITERAL = RDF_SYNTAX_NS + L"PlainLiteral";
const string RDF_XML_LITERAL = RDF_SYNTAX_NS + L"XMLLiteral";
const string RDF_OBJECT = RDF_SYNTAX_NS + L"object";
const string RDF_LANGSTRING = RDF_SYNTAX_NS + L"langString";
const string RDF_LIST = RDF_SYNTAX_NS + L"List";

const string LOADING_DOCUMENT_FAILED = L"loading document failed";
const string LIST_OF_LISTS = L"list of lists";
const string INVALID_INDEX_VALUE = L"invalid @index value";
const string CONFLICTING_INDEXES = L"conflicting indexes";
const string INVALID_ID_VALUE = L"invalid @id value";
const string INVALID_LOCAL_CONTEXT = L"invalid local context";
const string MULTIPLE_CONTEXT_LINK_HEADERS = L"multiple context link headers";
const string LOADING_REMOTE_CONTEXT_FAILED = L"loading remote context failed";
const string INVALID_REMOTE_CONTEXT = L"invalid remote context";
const string RECURSIVE_CONTEXT_INCLUSION = L"recursive context inclusion";
const string INVALID_BASE_IRI = L"invalid base IRI";
const string INVALID_VOCAB_MAPPING = L"invalid vocab mapping";
const string INVALID_DEFAULT_LANGUAGE = L"invalid default language";
const string KEYWORD_REDEFINITION = L"keyword redefinition";
const string INVALID_TERM_DEFINITION = L"invalid term definition";
const string INVALID_REVERSE_PROPERTY = L"invalid reverse property";
const string INVALID_IRI_MAPPING = L"invalid IRI mapping";
const string CYCLIC_IRI_MAPPING = L"cyclic IRI mapping";
const string INVALID_KEYWORD_ALIAS = L"invalid keyword alias";
const string INVALID_TYPE_MAPPING = L"invalid type mapping";
const string INVALID_LANGUAGE_MAPPING = L"invalid language mapping";
const string COLLIDING_KEYWORDS = L"colliding keywords";
const string INVALID_CONTAINER_MAPPING = L"invalid container mapping";
const string INVALID_TYPE_VALUE = L"invalid type value";
const string INVALID_VALUE_OBJECT = L"invalid value object";
const string INVALID_VALUE_OBJECT_VALUE = L"invalid value object value";
const string INVALID_LANGUAGE_TAGGED_STRING = L"invalid language-tagged const String";
const string INVALID_LANGUAGE_TAGGED_VALUE = L"invalid language-tagged value";
const string INVALID_TYPED_VALUE = L"invalid typed value";
const string INVALID_SET_OR_LIST_OBJECT = L"invalid set or list object";
const string INVALID_LANGUAGE_MAP_VALUE = L"invalid language map value";
const string COMPACTION_TO_LIST_OF_LISTS = L"compaction to list of lists";
const string INVALID_REVERSE_PROPERTY_MAP = L"invalid reverse property map";
const string INVALID_REVERSE_VALUE = L"invalid @reverse value";
const string INVALID_REVERSE_PROPERTY_VALUE = L"invalid reverse property value";
const string SYNTAX_ERROR = L"syntax error";
const string NOT_IMPLEMENTED = L"not implemnted";
const string UNKNOWN_FORMAT = L"unknown format";
const string INVALID_INPUT = L"invalid input";
const string PARSE_ERROR = L"parse error";
const string UNKNOWN_ERROR = L"unknown error";

const auto Ex1 = wruntime_error ( INVALID_IRI_MAPPING + string ( L"expected value of @id to be a string" ) );
const auto Ex2 = wruntime_error ( INVALID_KEYWORD_ALIAS + string ( L"cannot alias @context" ) );
const auto Ex3 = wruntime_error ( INVALID_IRI_MAPPING + string ( L"resulting IRI mapping should be a keyword, absolute IRI or blank node" ) );
const auto Ex4 = wruntime_error ( INVALID_IRI_MAPPING + string ( L"relative term defn without vocab mapping" ) );
const auto Ex5 = wruntime_error ( INVALID_IRI_MAPPING + tab + string ( L"Expected String for @reverse value." ) );
const auto Ex6 = INVALID_REVERSE_PROPERTY + string ( L"reverse properties only support set- and index-containers" );
const auto Ex7 = wruntime_error ( INVALID_VOCAB_MAPPING + tab + string ( L"@value must be an absolute IRI" ) );
const auto Ex8 = wruntime_error ( INVALID_VOCAB_MAPPING + tab + string ( L"@vocab must be a string or null" ) );
const auto Ex9 = wruntime_error ( INVALID_BASE_IRI + tab + string ( L"@base must be a string" ) );
const auto Ex10 = INVALID_CONTAINER_MAPPING + string ( L"@container must be either @list, @set, @index, or @language" );
const auto Ex11 = wruntime_error ( INVALID_LANGUAGE_MAPPING + string ( L"@language must be a string or null" ) );
const auto Ex12 = wruntime_error ( INVALID_REVERSE_PROPERTY_MAP + string ( tab ) + string ( L"a keyword cannot be used as a @reverse propery" ) );
const auto Ex13 = wruntime_error ( INVALID_ID_VALUE + tab + string ( L"value of @id must be a string" ) );
const auto Ex14 = wruntime_error ( INVALID_TYPE_VALUE + tab + string ( L"@type value must be a string or array of strings" ) );
const auto Ex15 = wruntime_error ( INVALID_TYPE_VALUE + tab + string ( L"@type value must be a an empty object for framing" ) );
const auto Ex16 = wruntime_error ( INVALID_TYPE_VALUE + tab + string ( L"@type value must be a string or array of strings" ) );
const auto Ex17 = wruntime_error ( LIST_OF_LISTS + tab + string ( L"A list may not contain another list" ) );
const auto Ex18 = wruntime_error ( INVALID_REVERSE_VALUE + tab + string ( L"@reverse value must be an object" ) );
const auto Ex19 = wruntime_error ( LIST_OF_LISTS + tab + string ( L"lists of lists are not permitted." ) );

const string implication = L"http://www.w3.org/2000/10/swap/log#implies";
const pstring pimplication = pstr(implication);

template<typename T> inline bool is ( const T& s, const std::vector<T>& v, const string& exception = string() ) {
	bool rc = std::find ( v.begin(), v.end(), s ) != v.end();
	if ( exception.size() && !rc ) throw wruntime_error ( exception );
	return rc;
}

template<typename T> inline bool is ( const std::shared_ptr<T>& s, const std::vector<T>& v, const string& exception = string() ) {
	return is<T> ( *s, v, exception );
}

bool endsWith ( const string& x, const string& y );
bool startsWith ( const string& x, const string& y );
inline bool startsWith ( pstring x, const string& y ) { return startsWith(*x, y); }
string lower ( const string& s_ );

template<typename charT> inline std::vector<string> split ( const string& s, charT c ) {
	std::vector<string> v;
	for ( string::size_type i = 0, j = s.find ( c ); j != string::npos; ) {
		v.push_back ( s.substr ( i, j - i ) );
		j = s.find ( c, i = ++j );
		if ( j == string::npos ) v.push_back ( s.substr ( i, s.length() ) );
	}
	return v;
}

string ws(const std::string& s);
std::string ws(const string& s);
std::string ws(pstring s);
pstring wstrim(const wchar_t* w);
pstring wstrim(const std::string&);
pstring wstrim(string);
pstring pstr ( const string& s );
pstring pstr ( const wchar_t* s );
pstring pstrtrim ( const string& s );
pstring pstrtrim ( const wchar_t* s );
inline pstring pstr(const std::string& s) { return pstr(ws(s)); }
inline pstring pstr(const char* s) { return pstr(std::string(s)); }
template<typename T> pstring tostr ( T t ) {
	std::wstringstream s;
	s << t;
	return pstr(s.str());
}
template<typename T> string _tostr ( T t ) {
	std::wstringstream s;
	s << t;
	return s.str();
}


extern string KNRM, KRED, KGRN, KYEL, KBLU, KMAG, KCYN, KWHT;

extern int level;

#endif
