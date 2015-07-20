#include <stdio.h>
#include "cli.h"
#include <sstream>
#include "prover.h"
#include "match.h"
#ifdef with_marpa
#include "marpa_tau.h"
#endif

#ifdef DEBUG
auto dummy = []() {
	return ( bool ) std::cin.tie ( &std::clog );
}();
#endif
bool autobt = false, _pause = false, __printkb = false, fnamebase = true, quad_in = false, nocolor = false;
jsonld_options opts;

//void menu();
//#include <boost/asio.hpp>
//#include <boost/asio/ip/tcp.hpp>

std::wostream& dout = std::wcout;
std::wostream& derr = std::wcerr;
//#endif
/*
class listen_cmd : public cmd_t {
	virtual std::string desc() const { return "Listen to incoming connections and answer queries."; }
	virtual std::string help() const { return "Behave as a server. Usage: tau listen <port>"; }
	virtual int operator() ( const strings& args ) {
		if (args.size() != 3) { derr<<help()<<std::endl; return 1; }
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
*/
#define ever ;;

class prove_cmd : public cmd_t {
public:
	virtual std::string desc() const {
		return "Run a query against a knowledgebase.";
	}
	virtual std::string help() const {
		std::stringstream ss ( "Usage:" );
		ss << std::endl << "\ttau prove [JSON-LD kb filename] [JSON-LD query filename]" << "\tAnswers the query." << std::endl;
		return ss.str();
	}
	virtual int operator() ( const strings& args ) {
		if ( ( args.size() == 3 && args[1] == L"help" ) || ( args.size() != 2 && args.size() != 4 ) ) {
			dout << ws(help());
			return 1;
		}
		try {
#ifdef IRC
			for(ever) { try {
#endif					
			auto kb = load_quads(L"");
			if (kb) {
				dout << "kb input done." << std::endl;
				auto query = load_quads(L"");
				dout << "query loaded." << std::endl;
				if (query) {
					prover pr( *kb );
					pr ( *query );
					dout << "Ready." << std::endl;
				}
			}
#ifdef IRC
			} catch (std::exception& ex) { dout<<ex.what()<<std::endl; } 
			catch (...) { dout<<"generic exception."<<std::endl; } 
			sleep(1);
			}
#endif
			return 0;
		} catch ( std::exception& ex ) {
			derr << ex.what() << std::endl;
			return 1;
		}
		return 0;
	}
};

int main ( int argc, char** argv ) {
	dict.init();
	cmds_t cmds = { {
			#ifdef with_marpa
			{ string ( L"load_n3" ) , new load_n3_cmd },
			#endif
			{ string ( L"convert" ) , new convert_cmd },
			{ string ( L"prove" ) , new prove_cmd }
		}, {
			{ { L"--no-deref", L"show integers only instead of strings" }, &deref },
			{ { L"--pause", L"pause on each trace and offer showing the backtrace. available under -DDEBUG only." }, &_pause },
			{ { L"--shorten", L"on IRIs containig # show only what after #" }, &shorten },
		//	{ { "--printkb", "print predicates, rules and frames at the end of prove command" }, &__printkb },
			{ { L"--base", L"set file://<filename> as base in JsonLDOptions" }, &fnamebase },
			{ { L"--quads", L"set input format for prove command as quads" }, &quad_in },
			{ { L"--nocolor", L"disable color output" }, &nocolor }
		}
	};
	strings args;
	for ( int n = 0; n < argc; ++n ) args.push_back ( ws(std::string(argv[n])) );
	for ( int n = 0; n < argc; ++n ) 
		if (args[n] == L"--level") {
			if (n + 1 == argc) {
				derr<<"please specify log level."<<std::endl;
				exit(1);
			}
			level = std::stoi(args[n+1]);
			args.erase(args.begin() + n + 1);
			args.erase(args.begin() + n);
			break;
		}
	
	process_flags ( cmds, args );
	if (nocolor)
		KNRM = KRED = KGRN = KYEL = KBLU = KMAG = KCYN = KWHT = L"";
	argc = args.size();
	if ( argc == 1 ) {
		prove_cmd p;
		quad_in = true;
		return p({L"",L"",L"",L""});
	}
	if (( cmds.first.find ( args[1] ) == cmds.first.end() && args[1] != L"help" ) ) {
		print_usage ( cmds );
		return 1;
	}
	if ( args[1] == L"help" && argc == 3 ) {
		dout << ws(cmds.first[string ( args[2] )]->help());
		return 0;
	}
	if ( args[1] == L"help" && argc == 2 ) {
		print_usage ( cmds );
		return 0;
	}

	int rval = ( *cmds.first[args[1]] ) ( args );

	return rval;
}
