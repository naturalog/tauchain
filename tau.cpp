#include <stdio.h>
#include "cli.h"
#include <sstream>
#include "prover.h"
#ifdef with_marpa
#include "marpa_tau.h"
#endif
#include <tclap/CmdLine.h>
/*

Cleaned stuff up here; some removed, some moved to cli.h/cpp

*/




/* Then we have some Ohad stuff */

boost::interprocess::managed_heap_memory* segment;
allocator_t* alloc;

std::wostream& dout = std::wcout;
std::wostream& derr = std::wcerr;

#define ever ;;



/*And here we go...*/


//following?yeah:)coo//its good..dont hesitate to leave plenty comments in...and delete the old stuff:)cool:)heres one more thing
/*So tau.cpp is divided into:

	1) class prove_cmd : public cmd_t, 

	2) int main( int argc, char** argv)
	
*/



/*
(1)
*/

class prove_cmd : public cmd_t {

//Mostly stuff related to command line
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
			for(ever) {try {
#endif
			/*
			//
			//
			*/
			/*
			CORE LOGIC
			Core bit of "main logic" encapsulated by stuff that's mostly just related to parsing command-line args


			*/
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
			/*
			//
			//
			*/

			//And more command-line stuff...
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

//so we have that little bit of core logic sandwiched between stuff that's just commandline
//like i said before, id rather see all the input/output stuff here, the tclap code will be quite clean.. sure :)anyway..this works now?yea assuming i haven't broken anything lets check


/*
(2)
*/


int main ( int argc, char** argv ) {
	dict.init();
	/*
	1. My new TCLAP command line stuff; just handling boolean-type or "SwitchArg" arguments like --nocolor and --no-deref
	*/
	

	try{
		/*
		This is the object that holds all the cli info
		*/
		TCLAP::CmdLine cmd("Tau command line",' ',"0.0");
		
		/*
		Here are the "argument objects", ValueArg's are parameterized by a type, SwitchArgs are boolean
		*/
		TCLAP::ValueArg<std::string> tc_loadn3("3","load_n3","load n3",false,"","",cmd);

		//Compare this with://nice :)
		TCLAP::SwitchArg 
			tc_deref("d","no-deref","show integers only instead of strings",cmd,true),
			tc_pause("P","pause","pause on each trace and offer showing the backtrace. available under -DDEBUG only.",cmd,false),
			tc_shorten("s","shorten","on IRIs containing # show only what's after #",cmd,false),
			tc_base("b","base","set file://<filename> as base in JsonLDOptions",cmd,true),
			tc_quads("q","quads","set input format for prove as quads",cmd,false),
			tc_nocolor("n","nocolor","disable color output",cmd,false);		

		/*
		Now we just run this and it parses our command line into the 'cmd' object
		*/
		cmd.parse(argc,argv);
		

		/*
		Then we can reference the args with .getValue() and do what we want with them. Right now I'm having trouble putting these objects 'tc_deref', 'tc_pause' etc into things like std::list, std::tuple etc. But this works and handles our boolean-type commands like --nocolor and --no-deref.
		*///why would you put them into lists?
		/*
		for something like
		for(x : thisList){
			bool = x.getValue();
		}
		
		*///just a second, i'll show you how Ohad does
		//i just don't like repitition :)
		deref = tc_deref.getValue();
		_pause = tc_pause.getValue();
		shorten = tc_shorten.getValue();
		fnamebase = tc_base.getValue();
		quad_in = tc_quads.getValue();
		nocolor = tc_nocolor.getValue();
	//but youre done here, its processed, set, all done? yea it works :) so for the meantime i'm just leaving it even though i hope to implement something cleaner later

	
	}catch(TCLAP::ArgException &e){
		derr << "TCLAP error" << std::endl;
	}



	/*
	2. All the old stuff that was there before
	
	From here it carries on as if my code wasn't even there except that I disabled the actual toggling of the "option" boolean vars in 'process_flags' in order to demonstrate that the TCLAP is actually working as expected. If the boolean-type (SwitchArgs) were not tied in with the other value-type (ValueArgs) here in this cmds_t cmds then that part couldjust be removed at this point.

	*/

	/*
	Here's Ohad's equivalent of TCLAP 'argument objects', except here you see '--level', and '--help' aren't listed because they aren't accounted for in a homogeneous fashion with the rest of the arguments like '--convert', '--prove' etc.
	*/
	cmds_t cmds = { {
			#ifdef with_marpa
			{ string ( L"load_n3" ) , new load_n3_cmd },
			#endif
			{ string ( L"convert" ) , new convert_cmd },
			{ string ( L"prove" ) , new prove_cmd }
		},
		//No different really?
		 {
			{ { L"--no-deref", L"show integers only instead of strings" }, &deref },
			{ { L"--pause", L"pause on each trace and offer showing the backtrace. available under -DDEBUG only." }, &_pause },
			{ { L"--shorten", L"on IRIs containig # show only what after #" }, &shorten },
			{ { L"--base", L"set file://<filename> as base in JsonLDOptions" }, &fnamebase },
			{ { L"--quads", L"set input format for prove command as quads" }, &quad_in },
			{ { L"--nocolor", L"disable color output" }, &nocolor }
		}//ah..
	};


	/*

	And then the argument handling starts
	*/
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
	
	/*
	And here's where Ohad toggles the boolean-type options like I've done with SwitchArgs. This could be removed except for that the 'process_flags' loop removes these boolean-type options from the list which affects the following command-line code which is very much based on argument counts. This is the next stuff I'm looking at working into TCLAP.
	*/
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
