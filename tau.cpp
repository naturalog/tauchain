#include <stdio.h>
#include "cli.h"
#include <sstream>
#include "parsers.h"
#include "prover.h"
using namespace std;

#ifdef DEBUG
auto dummy = []() {
	return ( bool ) std::cin.tie ( &std::clog );
}();
#endif
bool autobt = false, _pause = false, __printkb = false, fnamebase = true, quad_in = false;
jsonld::jsonld_options opts;

void menu();
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#ifdef IRC
ostream& dout = *new ofstream(string("/tmp/irc.freenode.net/#zennet/in"));
ostream& derr = *new ofstream(string("/tmp/irc.freenode.net/#zennet/in"));
string chan;
#else
ostream& dout = cout;
ostream& derr = cerr;
#endif

class listen_cmd : public cmd_t {
	virtual string desc() const { return "Listen to incoming connections and answer queries."; }
	virtual string help() const { return "Behave as a server. Usage: tau listen <port>"; }
	virtual int operator() ( const strings& args ) {
		if (args.size() != 3) { derr<<help()<<endl; return 1; }
		using namespace boost::asio;
		using boost::asio::ip::tcp;
		io_service ios;
		ip::tcp::endpoint endpoint(tcp::v4(), stoi(args[2]));
		ip::tcp::acceptor acceptor(ios, endpoint);
		for (;;) {
			ip::tcp::iostream& stream = *new ip::tcp::iostream;
			acceptor.accept(*stream.rdbuf());
			stream.expires_from_now(boost::posix_time::seconds(60));
			threads.push_back(new thread([&stream](){
				string line;
				while (stream) {
					stream >> line;
					dout << line << endl;
				}
			}));
		}
		return 0;
	}
};

#define ever ;;

class prove_cmd : public cmd_t {
public:
	reasoner r;
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
			dout << help();
			return 1;
		}
		if ( args.size() == 2 )
			dout << ( r.test_reasoner() ? "QED! \npass" : "fail" ) << endl;
		else try {
#ifdef IRC
				prover::initmem(true);
				for(ever) { try {
#endif
				qdb kb = !quad_in ? convert ( args[2] ) : load_quads(args[2]);
				opts.base = pstr ( string ( "file://" ) + args[2] + "#" );
				qdb query = !quad_in ? convert ( load_json(args[3]) ) : load_quads(args[3]);
				auto e = r.prove ( kb, merge ( query ) );
				dout << "evidence: " << endl << e << endl;
#ifdef IRC
				} catch (exception& ex) { derr<<ex.what()<<endl; } }
#endif
				return 0;
			} catch ( exception& ex ) {
				derr << ex.what() << endl;
				return 1;
			}
		return 0;
	}
};

int main ( int argc, char** argv ) {
#ifdef IRC	
	chan = argv[1];
	argc--;
#endif	
	prover::initmem();
	cmds_t cmds = { {
			{ string ( "expand" ) , new expand_cmd },
			{ string ( "toquads" ) , new toquads_cmd },
			{ string ( "nodemap" ) , new nodemap_cmd },
			{ string ( "convert" ) , new convert_cmd },
			{ string ( "prove" ) , new prove_cmd },
			{ string ( "listen" ) , new listen_cmd }
		}, {
			{ { "--no-deref", "show integers only instead of strings" }, &deref },
			{ { "--pause", "pause on each trace and offer showing the backtrace. available under -DDEBUG only." }, &_pause },
			{ { "--shorten", "on IRIs containig # show only what after #" }, &shorten },
		//	{ { "--printkb", "print predicates, rules and frames at the end of prove command" }, &__printkb },
			{ { "--base", "set file://<filename> as base in JsonLDOptions" }, &fnamebase },
			{ { "--quads", "set input format for prove command as quads" }, &quad_in }
		}
	};
	strings args;
	for ( int n = 0; n < argc; ++n ) args.push_back ( argv[n] );
	process_flags ( cmds, args );
	if ( argc == 1 ) {
		dout << endl << "Input kb as quads, then query." << endl << "After finished inserting kb, write a line \"fin.\" in order to move to query." << endl<< "Then after query is inputted type aother \"fin.\" or Ctrl+D in order to start reasoning."<<"Syntax is \"s p o c.\" or \"s p o.\" for triples in @default graph." << endl << endl ;
		prove_cmd p;
		quad_in = true;
		return p({"","","",""});
/*		qdb kb = p.load_quads("");
		qdb query = p.load_quads("");
		auto e = p.r.prove ( kb, merge ( query ) );
		dout << "evidence: " << endl << e << endl;*/
	}
	if (( cmds.first.find ( argv[1] ) == cmds.first.end() && args[1] != "help" ) ) {
		print_usage ( cmds );
		return 1;
	}
	if ( args[1] == "help" && argc == 3 ) {
		dout << cmds.first[string ( argv[2] )]->help();
		return 0;
	}
	if ( args[1] == "help" && argc == 2 ) {
		print_usage ( cmds );
		return 0;
	}

	int rval = ( *cmds.first[argv[1]] ) ( args );

	for (auto x : threads) { x->join(); delete x; } 
/*
	ofstream o("proof.dot");
	o<<"digraph Proof {"<<endl;
	for (auto x : proofs) o<<x->dot()<<endl;
	o<<"}";
	ofstream o1("rules.dot");
	o1<<"digraph Predicates {"<<endl;
	for (size_t n = 0; n < npredicates; ++n) o1<<predicates[n].dot()<<endl;
	o1<<"}";
	dout << "Written proof.dot and rules.dot. Use 'dot -Tpng proof.dot > proof.png' etc to visualize." << endl;
*/
	return rval;
}
