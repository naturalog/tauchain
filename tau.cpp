#include "cli.h"

#ifdef DEBUG
auto dummy = []() {
	return ( bool ) std::cin.tie ( &std::clog );
}();
bool autobt = false, _pause = false;
#endif

void menu();

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
		} catch ( exception& ex ) {
			cerr << ex.what() << endl;
			return 1;
		}
		return 0;
	}
};

class prove_cmd : public cmd_t {
	reasoner r;
public:
	virtual string desc() const {
		return "Run a query against a knowledgebase.";
	}
	virtual string help() const {
		stringstream ss ( "Usage:" );
		ss << endl << "\ttau prove\tRun socrates unit test";
		ss << endl << "\ttau prove [JSON-LD kb filename] [JSON-LD query filename]" << tab << "Resolves the query." << endl;
		return ss.str();
	}
	virtual int operator() ( const strings& args ) {
		if ( ( args.size() == 3 && args[1] == "help" ) || ( args.size() != 2 && args.size() != 4 ) ) {
			cout << help();
			return 1;
		}
		if ( args.size() == 2 )
			cout << ( r.test_reasoner() ? "pass" : "fail" ) << endl;
		else try {
			cout << "evidence: " << endl << r ( convert ( args[2] ), merge ( convert ( args[3] ) ) );
		//	menu();
	//		cout << "dict: " << endl << dict.tostr()<<endl;
			return 0;
		} catch ( exception& ex ) {
			cerr << ex.what() << endl;
			return 1;
		}
		return 0;
	}
};

map<string, cmd_t*> cmds;

int main ( int argc, char** argv ) {
	cmds = {
	{ string("expand") , new expand_cmd },
	{ string("toquads") , new toquads_cmd },
	{ string("nodemap") , new nodemap_cmd },
	{ string("convert") , new convert_cmd },
	{ string("prove") , new prove_cmd }};
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
