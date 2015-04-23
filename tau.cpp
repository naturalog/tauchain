#include "reasoner.h"

typedef list<string> strings;

class cmd_t {
public:
	virtual string desc() const = 0;
	virtual string help() const = 0;
	virtual int operator() ( const strings& args ) = 0;

	pobj load_json ( istream& is ) {
		json_spirit::mValue v;
		json_spirit::read_stream ( is, v );
		pobj o = convert ( v );
		//		clog << "
	}

	rdf_db load_jsonld ( istream is, bool print = false ) {
		using namespace jsonld;
		jsonld_options o;

		trace ( cout << "reading json:" << endl;
		        cout << "-------------" << endl; );
		pobj c = load_json ( is );
		trace ( cout << c->toString() << endl; );
		jsonld_api a ( c, o );
		trace ( cout << "--------------------" << endl;
		        cout << "converting to quads:" << endl;
		        cout << "--------------------" << endl; );
		auto r = *a.toRDF ( c, o );
		if ( print ) {
			cout << "Loaded graphs:" << endl;
			for ( auto x : r ) cout << x.first << endl;
		}
		//	cout << r.tostring() << endl;
		return r;
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
	}
};

class toquads_cmd : public cmd_t {
public:
	virtual string desc() const {
		return "Run JSON-LD->RDF algorithm http://www.w3.org/TR/json-ld-api/#deserialize-json-ld-to-rdf-algorithm including all dependant algorithms.";
	}
	virtual string help() const {
		stringstream ss ( "Usage: tau toquads [JSON-LD input filename]" );
		ss << endl << "If input filename is unspecified, reads from stdin." << endl;
		return ss.str();
	}
	virtual int operator() ( const strings& args ) {
	}
};

map<string, cmd_t*> cmds = []() {
	map<string, cmd_t*> r;
	r["expand"] = new expand_cmd;
	r["toquads"] = new toquads_cmd;
	return r;
}();

int main ( int argc, char** argv ) {
	if ( argc == 1 || ( cmds.find ( argv[2] ) == cmds.end() ) ) {
		cout << endl << "Tau-Chain by http://idni.org" << endl;
		cout << endl << "Usage:" << endl;
		cout << "\ttau <command>\t\tPrints usage of <command>." << endl;
		cout << "\ttau <command> <args>\t\tRun <command> with <args>." << endl;
		cout << endl << "Availiable commands:" << endl << endl;
		for ( auto c : cmds ) cout << '\t' << c.first << '\t' << c.second->desc() << endl;
		return 1;
	}
	if ( argc == 2 ) {
		cout << cmds[string ( argv[1] )]->help();
		return 1;
	}
	strings args;
	for ( int n = 0; n < argc; ++n ) args.push_back ( argv[n] );
	return ( *cmds[argv[1]] ) ( args );

	//	print_evidence(prove ( *kb["@default"], *query["@default"] ));

	return 0;
}
