// package com.github.jsonldjava.core;

/**
    URI Constants used in the JSON-LD parser.
*/

#ifndef JSON_LS_CONSTS
#define JSON_LS_CONSTS
#include "defs.h"

//class JsonLdConsts {
//public:
	const String RDF_SYNTAX_NS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
	const String RDF_SCHEMA_NS = "http://www.w3.org/2000/01/rdf-schema#";
	const String XSD_NS = "http://www.w3.org/2001/XMLSchema#";

	const String XSD_ANYTYPE = XSD_NS + "anyType";
	const String XSD_BOOLEAN = XSD_NS + "boolean";
	const String XSD_DOUBLE = XSD_NS + "double";
	const String XSD_INTEGER = XSD_NS + "integer";
	const String XSD_FLOAT = XSD_NS + "float";
	const String XSD_DECIMAL = XSD_NS + "decimal";
	const String XSD_ANYURI = XSD_NS + "anyURI";
	const String XSD_STRING = XSD_NS + "string";

	const String RDF_TYPE = RDF_SYNTAX_NS + "type";
	const String RDF_FIRST = RDF_SYNTAX_NS + "first";
	const String RDF_REST = RDF_SYNTAX_NS + "rest";
	const String RDF_NIL = RDF_SYNTAX_NS + "nil";
	const String RDF_PLAIN_LITERAL = RDF_SYNTAX_NS + "PlainLiteral";
	const String RDF_XML_LITERAL = RDF_SYNTAX_NS + "XMLLiteral";
	const String RDF_OBJECT = RDF_SYNTAX_NS + "object";
	const String RDF_LANGSTRING = RDF_SYNTAX_NS + "langString";
	const String RDF_LIST = RDF_SYNTAX_NS + "List";
//};
#endif
