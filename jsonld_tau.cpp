#include "prover.h"
#include "jsonld.h"
#include "jsonld_tau.h"
#include "misc.h"


bool autobt = false, _pause = false, __printkb = false;


#ifdef JSON

#include "json_spirit.h"

jsonld_options opts;

pobj convert ( const json_spirit::mValue& v );
json_spirit::mValue convert ( obj& v );
json_spirit::mValue convert ( pobj v );

pobj load_json ( std::istream &is ) {
	json_spirit::mValue v;
	json_spirit::read_stream_or_throw( is, v );
	pobj r =  ::convert ( v );
	if ( !r ) throw runtime_error ( "Couldn't make sense of input." );
	return r;
}

qdb toquads ( pobj o ) {
	jsonld_api a ( opts );
	rdf_db r ( a );
	auto nodeMap = o;
	std::map<string, pnode> lists;
	if (!nodeMap->MAP())
		throw runtime_error ( "Expected an object at top level i guess?" );
	for ( auto g : *nodeMap->MAP() ) {
		dout << g.first << std::endl;
		if ( is_rel_iri ( g.first ) ) continue;
		if ( !g.second )
			throw runtime_error ( "key without value?" );
		if ( !g.second->MAP() ) throw runtime_error ( "Expected map in nodemap, got "+g.second->type_str() );
		r.graph_to_rdf ( g.first, *g.second->MAP() );
	}
	return r;
}

ParsingResult load_jsonld ( qdb &kb, std::istream &is )
{
		kb = toquads(load_json(is));
		return COMPLETE;
}

ParsingResult parse_jsonld(qdb &kb, std::istream &f)
{
	try{
		load_jsonld(kb, f);
		return COMPLETE;
	}
	catch (runtime_error ex) { derr << "[json]" << ex.what() << std::endl; }
	catch (json_spirit::Error_position ex) { derr << "[json]" << ex.reason_ << std::endl; }
	catch (...) { derr << "[jsonld]Unknown exception" << std::endl; }
	return FAIL;
}


#endif

