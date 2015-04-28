#include "cli.h"

#ifdef DEBUG
auto dummy = []() {
	return ( bool ) std::cin.tie ( &std::clog );
}();
bool autobt = false, _pause = false;
#endif

class expand_cmd : public cmd_t {
public:
	virtual string desc() const {
		return "Run expansion algorithm http://www.w3.org/TR/json-ld-api/#expansion-algorithms including all dependant algorithms.";
	}
	virtual string help() const {
		stringstream ss ( "Usage:" );
		ss << endl << "\ttau expand [JSON-LD input filename]";
		ss << endl << "\ttau expand [JSON-LD input filename] [JSON-LD output to compare to]";
		ss << endl << "If input filename is unspecified, reads from stdin." << endl;
		return ss.str();
	}
	virtual int operator() ( const strings& args ) {
		if ( ( args.size() == 3 && args[1] == "help" ) || args.size() > 4 ) {
			cout << help();
			return 1;
		}
		pobj e;
		cout << ( e = jsonld::expand ( load_json ( args ) ) )->toString() << endl;
		if ( args.size() == 3 ) return 0;
		string f1 = tmpnam ( 0 ), f2 = tmpnam ( 0 );
		ofstream os1 ( f1 ), os2 ( f2 );
		os1 << json_spirit::write_string ( jsonld::convert ( e ), json_spirit::pretty_print | json_spirit::single_line_arrays ) << std::endl;
		os2 << json_spirit::write_string ( jsonld::convert ( load_json ( args[3] ) ), json_spirit::pretty_print | json_spirit::single_line_arrays ) << std::endl;
		os1.close();
		os2.close();
		string c = string ( "diff " ) + f1 + string ( " " ) + f2;
		system ( c.c_str() );

		return 0;
	}
};

class convert_cmd : public cmd_t {
public:
	virtual string desc() const {
		return "Convert JSON-LD to quads including all dependent algorithms.";
	}
	virtual string help() const {
		stringstream ss ( "Usage: tau expand [JSON-LD input filename]" );
		ss << endl << "If input filename is unspecified, reads from stdin." << endl;
		return ss.str();
	}
	virtual int operator() ( const strings& args ) {
		if ( ( args.size() == 3 && args[1] == "help" ) || args.size() > 3 ) {
			cout << help();
			return 1;
		}
		try {
			cout << convert ( load_json ( args ) ) << endl;
			return 0;
		} catch ( exception& ex ) {
			std::cerr << ex.what() << endl;
			return 1;
		}
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
		if ( ( args.size() == 3 && args[1] == "help" ) || args.size() > 3 ) {
			cout << help();
			return 1;
		}
		try {
			cout << toquads ( args ) << endl;
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
		if ( ( args.size() == 3 && args[1] == "help" ) || args.size() > 3 ) {
			cout << help();
			return 1;
		}
		try {
			cout << nodemap ( args )->toString() << endl;
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

class prove_cmd : public cmd_t {
public:
	virtual string desc() const {
		return "Run a query against a knowledgebase.";
	}
	virtual string help() const {
		stringstream ss ( "Usage:" );
		ss << endl << "\ttau prove\tRun socrates unit test";
		ss << endl << "\ttau prove [JSON-LD kb filename] [JSON-LD query filename]" << tab << "Does nothing but list all available graphs.";
		ss << endl << "\ttau prove [JSON-LD kb filename] [Graph name in kb] [JSON-LD query filename] [Graph name in query]" << tab << "Resolves the query." << endl;
		return ss.str();
	}
	virtual int operator() ( const strings& args ) {
		if ( ( args.size() == 3 && args[1] == "help" ) || ( args.size() != 2 && args.size() != 4 && args.size() != 6 ) ) {
			cout << help();
			return 1;
		}
		try {
			if ( args.size() == 2 ) cout << ( test_reasoner() ? "pass" : "fail" ) << endl;
			else if ( args.size() == 4 ) {
				qdb rkb, rq;
				pobj kb = nodemap ( jsonld::expand ( load_json ( args[2] ) ) );
				pobj q = nodemap ( jsonld::expand ( load_json ( args[3] ) ) );
				if ( kb && kb->MAP() ) {
					cout << "Contexts in kb:" << endl;
					rkb = toquads ( kb );
					//for ( auto x : rkb ) cout << x.first << '\t' << x.second->size() << " quads." << endl;
				} else cerr << "Cannot parse kb or empty kb." << endl;
				if ( q && q->MAP() ) {
					cout << "Contexts in query:" << endl;
					rq = toquads ( q );
					//for ( auto x : rq ) cout << x.first << '\t' << x.second->size() << " quads." << endl;
				} else {
					cerr << "Cannot parse query or empty query." << endl;
					return 1;
				}
				prove ( merge ( rkb ), merge ( rq ) );
				return 0;
			} else {
				auto _kb = convert ( load_json ( args[2] ) );
				auto _q = convert ( load_json ( args[4] ) );
				auto kb = _kb [args[3]];
				auto q = _q [args[5]];
				if ( !kb ) {
					cerr << "Fatal: kb selected graph is null." << endl;
					return 1;
				}
				if ( !q ) {
					cerr << "Fatal: query selected graph is null." << endl;
					return 1;
				}
				cout << prove ( *kb, *q );
			}
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
	r["convert"] = new convert_cmd;
	r["prove"] = new prove_cmd;
	return r;
}();

int main ( int argc, char** argv ) {
	strings args;
	for ( int n = 0; n < argc; ++n ) args.push_back ( argv[n] );
	if ( argc == 1 || ( cmds.find ( argv[1] ) == cmds.end() && args[1] != "help" ) ) {
		print_usage ( cmds );
		return 1;
	}
	if ( args[1] == "help" && argc == 3 ) {
		cout << cmds[string ( argv[2] )]->help();
		return 0;
	}
	if ( args[1] == "help" && argc == 2 ) {
		print_usage ( cmds );
		return 0;
	}
	return ( *cmds[argv[1]] ) ( args );

	return 0;
}
