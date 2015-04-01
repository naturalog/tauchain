// package com.github.jsonldjava.core;

/**
    URI Constants used in the JSON-LD parser.
*/
public: const class JsonLdConsts {

public: static const String RDF_SYNTAX_NS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
public: static const String RDF_SCHEMA_NS = "http://www.w3.org/2000/01/rdf-schema#";
public: static const String XSD_NS = "http://www.w3.org/2001/XMLSchema#";

public: static const String XSD_ANYTYPE = XSD_NS + "anyType";
public: static const String XSD_BOOLEAN = XSD_NS + "boolean";
public: static const String XSD_DOUBLE = XSD_NS + "double";
public: static const String XSD_INTEGER = XSD_NS + "integer";
public: static const String XSD_FLOAT = XSD_NS + "float";
public: static const String XSD_DECIMAL = XSD_NS + "decimal";
public: static const String XSD_ANYURI = XSD_NS + "anyURI";
public: static const String XSD_STRING = XSD_NS + "string";

public: static const String RDF_TYPE = RDF_SYNTAX_NS + "type";
public: static const String RDF_FIRST = RDF_SYNTAX_NS + "first";
public: static const String RDF_REST = RDF_SYNTAX_NS + "rest";
public: static const String RDF_NIL = RDF_SYNTAX_NS + "nil";
public: static const String RDF_PLAIN_LITERAL = RDF_SYNTAX_NS + "PlainLiteral";
public: static const String RDF_XML_LITERAL = RDF_SYNTAX_NS + "XMLLiteral";
public: static const String RDF_OBJECT = RDF_SYNTAX_NS + "object";
public: static const String RDF_LANGSTRING = RDF_SYNTAX_NS + "langString";
public: static const String RDF_LIST = RDF_SYNTAX_NS + "List";
}
;
