#include "prover.h"
#include "jsonld.h"
#include "cli.h"

bool autobt = false, _pause = false, __printkb = false, fnamebase = true, nocolor = false;


#ifdef JSON

#include "json_spirit.h"

jsonld_options opts;

pobj convert ( const json_spirit::mValue& v );
json_spirit::mValue convert ( obj& v );
json_spirit::mValue convert ( pobj v );

pobj cmd_t::load_json ( string fname, bool print ) {
	json_spirit::mValue v;
	if ( fname == "" ) json_spirit::read_stream ( std::wcin, v );
	else {
		std::ifstream is ( fname );
		if (!is.is_open()) throw std::runtime_error("couldnt open file");
		if (!json_spirit::read_stream ( is, v )) throw std::runtime_error("couldnt load json");
	}
	pobj r =  ::convert ( v );
	if ( !r ) throw runtime_error ( "Couldn't read input." );
	if ( print ) dout << r->toString() << std::endl;
	return r;
}

pobj cmd_t::load_json ( const strings& args ) {
	return load_json ( args.size() > 2 ? args[2] : "" );
}

pobj cmd_t::nodemap ( const strings& args ) {
	return nodemap ( load_json ( args[2] ) );
}

pobj cmd_t::nodemap ( pobj o ) {
	psomap nodeMap = make_shared<somap>();
	( *nodeMap ) [str_default] = mk_somap_obj();
	jsonld_api a ( opts );
	a.gen_node_map ( o, nodeMap );
	return mk_somap_obj ( nodeMap );
}

qdb cmd_t::toquads ( const strings& args ) {
	return toquads ( load_json ( args ) );
}

qdb cmd_t::toquads ( pobj o ) {
	jsonld_api a ( opts );
	rdf_db r ( a );
	auto nodeMap = o;
	std::map<string, pnode> lists;
	for ( auto g : *nodeMap->MAP() ) {
		if ( is_rel_iri ( g.first ) ) continue;
		if ( !g.second || !g.second->MAP() ) throw runtime_error ( "Expected map in nodemap." );
		r.graph_to_rdf ( g.first, *g.second->MAP() );
	}
	return r;
}

qdb cmd_t::convert ( pobj o ) {
	return toquads ( nodemap ( expand ( o, opts ) ) );
}

qdb cmd_t::convert ( const string& s ) {
	if ( fnamebase ) opts.base = pstr ( string ( "file://" ) + s + "#" );
	qdb r = convert ( load_json ( s ) );
	return r;
}

#endif

