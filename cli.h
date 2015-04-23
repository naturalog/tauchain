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
		pobj r =  ::convert ( v );
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
	pobj nodemap ( const strings& args ) {
		return nodemap ( load_json ( args ) );
	}
	pobj nodemap ( pobj o ) {
		psomap nodeMap = make_shared<somap>();
		( *nodeMap ) [str_default] = mk_somap_obj();
		jsonld::jsonld_api a;
		a.gen_node_map ( o, nodeMap );
		return mk_somap_obj ( nodeMap );
	}
	rdf_db toquads ( const strings& args ) {
		return toquads ( load_json ( args ) );
	}
	rdf_db toquads ( pobj o ) {
		rdf_db r;
		auto nodeMap = o;
		for ( auto g : *nodeMap->MAP() ) {
			if ( jsonld::is_rel_iri ( g.first ) ) continue;
			if ( !g.second || !g.second->MAP() ) throw 0;
			r.graph_to_rdf ( g.first, *g.second->MAP() );
		}
		return r;
	}
	rdf_db convert ( pobj o ) {
		return toquads ( nodemap ( jsonld::expand ( o ) ) );
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
		if ( args.size() > 3 ) {
			cout << help();
			return 1;
		}
		cout << jsonld::expand ( load_json ( args ) )->toString() << endl;
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
		if ( args.size() > 3 ) {
			cout << help();
			return 1;
		}
		cout << convert ( load_json ( args ) ).tostring() << endl;
		return 0;
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
		if ( args.size() > 3 ) {
			cout << help();
			return 1;
		}
		try {
			cout << toquads ( args ).tostring() << endl;
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
		if ( args.size() > 3 ) {
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
		ss << endl << "\ttau prove [JSON-LD kb filename] [JSON-LD query filename]" << tab << "Does nothing but list all availiable graphs." << endl;
		ss << endl << "\ttau prove [JSON-LD kb filename] [Graph name in kb] [JSON-LD query filename] [Graph name in query]" << tab << "(Hopefully) Answers the query." << endl;
		return ss.str();
	}
	virtual int operator() ( const strings& args ) {
		if ( args.size() != 4 && args.size() != 6 ) {
			cout << help();
			return 1;
		}
		try {
			if ( args.size() == 4 ) {
				pobj kb = nodemap ( jsonld::expand ( load_json ( args[2] ) ) );
				pobj q = nodemap ( jsonld::expand ( load_json ( args[args.size() == 4 ? 3 : 4] ) ) );
				if ( kb && kb->MAP() ) {
					cout << "Contexts in kb:" << endl;
					for ( auto x : *kb->MAP() ) cout << x.first << endl;
				} else cout << "Cannot parse query or empty query." << endl;
				if ( q && q->MAP() ) {
					cout << "Contexts in query:" << endl;
					for ( auto x : *q->MAP() ) cout << x.first << endl;
				} else cout << "Cannot parse query or empty query." << endl;
				return 0;
			}
			print_evidence ( prove ( *convert ( load_json ( args[2] ) ) [args[3]], *convert ( load_json ( args[args.size() == 4 ? 3 : 4] ) ) [args[5]] ) );
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
