/*
 * parsers.h
 *
 *  Created on: Apr 22, 2015
 *      Author: troy
 */

#ifndef PARSERS_H_
#define PARSERS_H_

#include "jsonld.h"
using namespace std;

#include <raptor2/raptor2.h>

void list_parser_options(raptor_world *world = raptor_new_world()) {
	for (size_t i = 0; i < raptor_option_get_count(); i++)
		if (raptor_option_description* od = raptor_world_get_option_description(
				world, RAPTOR_DOMAIN_PARSER, (raptor_option) i))
			cout << od->name << '\t' << od->label << endl;
}

pnode mknode(const raptor_term* t) {
	if (t->type == RAPTOR_TERM_TYPE_URI)
		return jsonld::mkiri((const char*) raptor_uri_as_string(t->value.uri));
	if (t->type == RAPTOR_TERM_TYPE_LITERAL) {
		const auto& l = t->value.literal;
		return jsonld::mkliteral((const char*) l.string,
				pstr((const char*) raptor_uri_as_string(l.datatype)),
				pstr((const char*) l.language));
	}
	if (t->type == RAPTOR_TERM_TYPE_BLANK)
		return jsonld::mkbnode((const char*) t->value.blank.string);
	throw runtime_error("unknown node type");
}

void add_quad(void* q, raptor_statement* t) {
	(*(rdf_db*) q)[(const char*) raptor_term_to_string(t->graph)]->push_back(
			make_shared < quad
					> (mknode(t->subject), mknode(t->predicate), mknode(
							t->object), mknode(t->graph)));
}

rdf_db load_nq(string fname) {
//	raptor_uri *uri, *base_uri;

	rdf_db r;
	raptor_world *world = raptor_new_world();
	raptor_parser* parser = raptor_new_parser(world, "nquads");
	raptor_parser_set_statement_handler(parser, (void*) &r, add_quad);
	raptor_uri *uri = raptor_new_uri(world,
			raptor_uri_filename_to_uri_string(fname.c_str()));
//	base_uri = raptor_uri_copy(uri);
	raptor_parser_parse_file(parser, uri, raptor_uri_copy(uri));
	raptor_free_parser (parser);
//	raptor_free_uri (base_uri);
	raptor_free_uri(uri);
//	raptor_free_memory (uri_string);
	raptor_free_world(world);
	return r;
}

#endif /* PARSERS_H_ */
