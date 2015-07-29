#ifdef IRC
#include "pstream.h"
#endif
#include "prover.h"
#include "jsonld.h"
#include "cli.h"
#include "json_spirit.h"

bool autobt = false, _pause = false, __printkb = false, fnamebase = true, quad_in = false, nocolor = false, deref = true, shorten = false;

jsonld_options opts;


//FILE-IO & CONVERSION
pobj  convert ( const json_spirit::wmValue& v );
json_spirit::wmValue convert ( obj& v );
json_spirit::wmValue convert ( pobj v );

pqdb cmd_t::load_quads ( string fname, bool print ) {
	qdb q;
	try {
		qdb r;
		std::wistream* pis = &std::wcin;
		if (fname != L"")
			pis = new std::wifstream(ws(fname));
		std::wistream& is = *pis;
		return std::make_shared<qdb>(readqdb(is));
	} catch (std::exception& ex) {
		derr << L"Error reading quads: " << ex.what() << std::endl;
	}
	if ( print ) 
		dout << q << std::endl;

	return std::make_shared<qdb>(q);
}



pobj cmd_t::load_json ( string fname, bool print ) {
	json_spirit::wmValue v;
	if ( fname == L"" ) json_spirit::read_stream ( std::wcin, v );
	else {
		std::wifstream is ( ws(fname) );
		if (!is.is_open()) throw std::runtime_error("couldnt open file");
		if (!json_spirit::read_stream ( is, v )) throw std::runtime_error("couldnt load json");
	}
	pobj r =  ::convert ( v );
	if ( !r ) throw wruntime_error ( L"Couldn't read input." );
	if ( print ) dout << r->toString() << std::endl;
	return r;
}

pobj cmd_t::load_json ( const strings& args ) {
	return load_json ( args.size() > 2 ? args[2] : L"" );
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
		if ( !g.second || !g.second->MAP() ) throw wruntime_error ( L"Expected map in nodemap." );
		r.graph_to_rdf ( g.first, *g.second->MAP() );
	}
	return r;
}

qdb cmd_t::convert ( pobj o ) {
	return toquads ( nodemap ( expand ( o, opts ) ) );
}

qdb cmd_t::convert ( const string& s ) {
	if ( fnamebase ) opts.base = pstr ( string ( L"file://" ) + s + L"#" );
	qdb r = convert ( load_json ( s ) );
	return r;
}



