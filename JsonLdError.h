#ifndef __JSONLDERROR_H__
#define __JSONLDERROR_H__
const String LOADING_DOCUMENT_FAILED = "loading document failed";
const String LIST_OF_LISTS = "list of lists";
const String INVALID_INDEX_VALUE = "invalid @index value";
const String CONFLICTING_INDEXES = "conflicting indexes";
const String INVALID_ID_VALUE = "invalid @id value";
const String INVALID_LOCAL_CONTEXT = "invalid local context";
const String MULTIPLE_CONTEXT_LINK_HEADERS = "multiple context link headers";
const String LOADING_REMOTE_CONTEXT_FAILED = "loading remote context failed";
const String INVALID_REMOTE_CONTEXT = "invalid remote context";
const String RECURSIVE_CONTEXT_INCLUSION = "recursive context inclusion";
const String INVALID_BASE_IRI = "invalid base IRI";
const String INVALID_VOCAB_MAPPING = "invalid vocab mapping";
const String INVALID_DEFAULT_LANGUAGE = "invalid default language";
const String KEYWORD_REDEFINITION = "keyword redefinition";
const String INVALID_TERM_DEFINITION = "invalid term definition";
const String INVALID_REVERSE_PROPERTY = "invalid reverse property";
const String INVALID_IRI_MAPPING = "invalid IRI mapping";
const String CYCLIC_IRI_MAPPING = "cyclic IRI mapping";
const String INVALID_KEYWORD_ALIAS = "invalid keyword alias";
const String INVALID_TYPE_MAPPING = "invalid type mapping";
const String INVALID_LANGUAGE_MAPPING = "invalid language mapping";
const String COLLIDING_KEYWORDS = "colliding keywords";
const String INVALID_CONTAINER_MAPPING = "invalid container mapping";
const String INVALID_TYPE_VALUE = "invalid type value";
const String INVALID_VALUE_OBJECT = "invalid value object";
const String INVALID_VALUE_OBJECT_VALUE = "invalid value object value";
const String INVALID_LANGUAGE_TAGGED_STRING = "invalid language-tagged const String";
const String INVALID_LANGUAGE_TAGGED_VALUE = "invalid language-tagged value";
const String INVALID_TYPED_VALUE = "invalid typed value";
const String INVALID_SET_OR_LIST_OBJECT = "invalid set or list object";
const String INVALID_LANGUAGE_MAP_VALUE = "invalid language map value";
const String COMPACTION_TO_LIST_OF_LISTS = "compaction to list of lists";
const String INVALID_REVERSE_PROPERTY_MAP = "invalid reverse property map";
const String INVALID_REVERSE_VALUE = "invalid @reverse value";
const String INVALID_REVERSE_PROPERTY_VALUE = "invalid reverse property value";
const String SYNTAX_ERROR = "syntax error";
const String NOT_IMPLEMENTED = "not implemnted";
const String UNKNOWN_FORMAT = "unknown format";
const String INVALID_INPUT = "invalid input";
const String PARSE_ERROR = "parse error";
const String UNKNOWN_ERROR = "unknown error";

class JsonLdError : public std::exception {
public:
	typedef String Error;
private:
	//	Map<String, Object> details;
	String type;
	const String error;
public:
	// TODO: Complete class
	//	JsonLdError ( Error type_, Object detail ) : type ( type_ ) {} //, details(detail) {}
	JsonLdError ( String type_ ) : type ( type_ ) {}
	JsonLdError ( String type_, String detail ) : error ( detail ) {}

	virtual String toString() const {
		return error;
	}

	JsonLdError& setType ( Error error ) {
		type = error;
		return *this;
	};

	Error getType() const {
		return type;
	}

	//	Map<String, Object> getDetails() const {
	//		return details;
	//	}

	virtual String getMessage() const {
		//		const String msg = super.getMessage();
		//		if ( msg != null && !"".equals ( msg ) )
		//			return type.toString() + ": " + msg;
		return type;//.toString();
	}
};
#endif
