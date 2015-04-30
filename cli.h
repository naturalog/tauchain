#include "reasoner.h"

typedef vector<string> strings;

class cmd_t {
	jsonld::jsonld_options opts;
public:
	virtual string desc() const = 0;
	virtual string help() const = 0;
	virtual int operator() ( const strings& args ) = 0;

	pobj load_json ( string fname = "", bool print = false ) {
		json_spirit::mValue v;
		if ( fname == "" ) json_spirit::read_stream ( cin, v );
		else {
			ifstream is ( fname );
			json_spirit::read_stream ( is, v );
		}
		pobj r =  jsonld::convert ( v );
		if ( print ) cout << r->toString() << endl;
		return r;
	}

	pobj load_json ( const strings& args ) {
		return load_json ( args.size() > 2 ? args[2] : "" );
	}

	pobj nodemap ( const strings& args ) {
		return nodemap ( load_json ( args[2] ), pstr ( string ( "file://" ) + args[2] ) );
	}

	pobj nodemap ( pobj o, pstring base ) {
		psomap nodeMap = make_shared<somap>();
		( *nodeMap ) [str_default] = mk_somap_obj();
		jsonld::jsonld_options opts ( base );
		jsonld::jsonld_api a ( opts );
		a.gen_node_map ( o, nodeMap );
		return mk_somap_obj ( nodeMap );
	}
	qdb toquads ( const strings& args ) {
		return toquads ( load_json ( args ), pstr ( args[2] ) );
	}

	qdb toquads ( pobj o, pstring base ) {
		jsonld::jsonld_options opts ( base );
		jsonld::jsonld_api a ( opts );
		rdf_db r ( a );
		auto nodeMap = o;
		for ( auto g : *nodeMap->MAP() ) {
			if ( jsonld::is_rel_iri ( g.first ) ) continue;
			if ( !g.second || !g.second->MAP() ) throw 0;
			r.graph_to_rdf ( g.first, *g.second->MAP() );
		}
		return r;
	}

	qdb convert ( pobj o, pstring base ) {
		return toquads ( nodemap ( jsonld::expand ( o, jsonld::jsonld_options ( base ) ), base ), base );
	}

	qdb convert ( const string& s ) {
		return convert ( load_json ( s ), pstr ( string ( "file://" ) + s + "#" ) );
	}
};

void print_usage ( const map<string, cmd_t*>& cmds ) {
	cout << endl << "Tau-Chain by http://idni.org" << endl;
	cout << endl << "Usage:" << endl;
	cout << "\ttau help <command>\t\tPrints usage of <command>." << endl;
	cout << "\ttau <command> [<args>]\t\tRun <command> with <args>." << endl;
	cout << endl << "Available commands:" << endl << endl;
	for ( auto c : cmds ) cout << '\t' << c.first << '\t' << c.second->desc() << endl;
	cout << endl;
}
