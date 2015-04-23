#include "reasoner.h"

typedef vector<string> strings;

class cmd_t {
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
		pobj r =  convert ( v );
		if ( print ) cout << r->toString() << endl;
		return r;
	}

	prdf_db to_quads ( pobj c, bool print = false ) {
		using namespace jsonld;
		jsonld_options o;
		jsonld_api a ( c, o );
		auto r = a.toRDF ( c, o );
		if ( print ) {
			cout << "Loaded graphs:" << endl;
			for ( auto x : *r ) cout << x.first << endl;
		}
		cout << r->tostring() << endl;
		return r;
	}

	prdf_db to_quads ( const strings& args, bool print = false ) {
		return to_quads ( load_json ( args ), print );
	}
	pobj load_json ( const strings& args ) {
		return load_json ( args.size() > 2 ? args[2] : "" );
	}
};

class expand_cmd : public cmd_t {
public:
	virtual string desc() const {
		return "Run expansion algorithm http://www.w3.org/TR/json-ld-api/#expansion-algorithms including all dependant algorithms.";
	}
	virtual string help() const {
		stringstream ss ( "Usage: tau expand [JSON-LD input filename]" );
		ss << endl << "If input filename is unspecified, reads from stdin." << endl;
		return ss.str();
	}
	virtual int operator() ( const strings& args ) {
		cout << jsonld::expand ( load_json ( args ) )->toString() << endl;
		return 1;
	}
};

class toquads_cmd : public cmd_t {
public:
	virtual string desc() const {
		return "Run JSON-LD->RDF algorithm http://www.w3.org/TR/json-ld-api/#deserialize-json-ld-to-rdf-algorithm of already-expanded input.";
	}
	virtual string help() const {
		stringstream ss ( "Usage: tau toquads [JSON-LD input filename]" );
		ss << endl << "If input filename is unspecified, reads from stdin." << endl;
		ss << "Note that input has to be expanded first, so you might want to pipe it with 'tau expand' command." << endl;
		return ss.str();
	}
	virtual int operator() ( const strings& args ) {
		try {
			to_quads ( args , true );
		} catch ( string& ex ) {
			cerr << ex << endl;
			return 1;
		} catch ( exception& ex ) {
			cerr << ex.what() << endl;
			return 1;
		}
		return 0;
	}
};

class nodemap_cmd : public cmd_t {
public:
	virtual string desc() const {
		return "Run JSON-LD node map generation algorithm.";
	}
	virtual string help() const {
		stringstream ss ( "Usage: tau toquads [JSON-LD input filename]" );
		ss << endl << "If input filename is unspecified, reads from stdin." << endl;
		return ss.str();
	}
	virtual int operator() ( const strings& args ) {
		try {
			psomap nodeMap = make_shared<somap>();
			( *nodeMap ) [str_default] = mk_somap_obj();
			jsonld::jsonld_api a;
			a.gen_node_map ( load_json ( args ), nodeMap );
			cout << mk_somap_obj ( nodeMap )->toString() << endl;
		} catch ( string& ex ) {
			cerr << ex << endl;
			return 1;
		} catch ( exception& ex ) {
			cerr << ex.what() << endl;
			return 1;
		}
		return 0;
	}
};

map<string, cmd_t*> cmds = []() {
	map<string, cmd_t*> r;
	r["expand"] = new expand_cmd;
	r["toquads"] = new toquads_cmd;
	r["nodemap"] = new nodemap_cmd;
	return r;
}();

int main ( int argc, char** argv ) {
	strings args;
	for ( int n = 0; n < argc; ++n ) args.push_back ( argv[n] );
	if ( argc == 1 || ( cmds.find ( argv[1] ) == cmds.end() && args[1] != "help" ) ) {
		cout << endl << "Tau-Chain by http://idni.org" << endl;
		cout << endl << "Usage:" << endl;
		cout << "\ttau help <command>\t\tPrints usage of <command>." << endl;
		cout << "\ttau <command> [<args>]\t\tRun <command> with <args>." << endl;
		cout << endl << "Availiable commands:" << endl << endl;
		for ( auto c : cmds ) cout << '\t' << c.first << '\t' << c.second->desc() << endl;
		return 1;
	}
	if ( args[1] == "help" && argc == 3 ) {
		cout << cmds[string ( argv[2] )]->help();
		return 0;
	}
	return ( *cmds[argv[1]] ) ( args );

	//	print_evidence(prove ( *kb["@default"], *query["@default"] ));

	return 0;
}
