#include <stdio.h>
#include "cli.h"
#include <sstream>
#include "prover.h"
#ifdef with_marpa
#include "marpa_tau.h"
#endif
#include <tclap/CmdLine.h>


boost::interprocess::managed_heap_memory* segment;
allocator_t* alloc;

std::wostream& dout = std::wcout;
std::wostream& derr = std::wcerr;

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

//well first i split off that core logic, it all still works well and good
	void runTau(){
		
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

			runTau();	


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


typedef std::map<TCLAP::SwitchArg*,bool*> tcFlags;

int main ( int argc, char** argv ) {
	dict.init();
	
	try{
		TCLAP::CmdLine cmd("Tau command line",' ',"0.0");
		
		TCLAP::ValueArg<std::string> tc_loadn3("3","load_n3","load n3",false,"","",cmd);
		TCLAP::ValueArg<int> tc_level("l","level","level",false,1,"",cmd);
		
		/*Ok now my goal is to make these definitions...*/
		TCLAP::SwitchArg
			tc_deref("d","no-deref","show integers only instead of strings",cmd,true),
			tc_pause("P","pause","pause on each trace and offer showing the backtrace. available under -DDEBUG only.",cmd,false),
			tc_shorten("s","shorten","on IRIs containing # show only what's after #",cmd,false),
			tc_base("b","base","set file://<filename> as base in JsonLDOptions",cmd,true),
			tc_quads("q","quads","set input format for prove as quads",cmd,false),
			tc_nocolor("n","nocolor","disable color output",cmd,false);		
		
		cmd.parse(argc,argv);
		
		/*Within this std::map<TCLAP::Arg*,bool*>, so that it looks pretty much exactly like the second part of cmds_t down there */
		tcFlags tauFlags = {
			{&tc_deref,&deref},
			{&tc_pause,&_pause},
			{&tc_shorten,&shorten},
			{&tc_base,&fnamebase},
			{&tc_quads,&quad_in},
			{&tc_nocolor,&nocolor}
		};

		for( auto x : tauFlags){
			*x.second = (*(x.first)).getValue();
		}	
	
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
		 {
			{ { L"--no-deref", L"show integers only instead of strings" }, &deref },
			{ { L"--pause", L"pause on each trace and offer showing the backtrace. available under -DDEBUG only." }, &_pause },
			{ { L"--shorten", L"on IRIs containig # show only what after #" }, &shorten },
			{ { L"--base", L"set file://<filename> as base in JsonLDOptions" }, &fnamebase },
			{ { L"--quads", L"set input format for prove command as quads" }, &quad_in },
			{ { L"--nocolor", L"disable color output" }, &nocolor }
		}//ah..
	};//see what i mean? //well..well...lets throw all the old crap away already? can't yet, it's too tied into other things, but that's the next step.. this thing i want to do with the lists is not so important, it can wait, here i commented the stuff down below pretty well though


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


	/*hmm.. where can this go... does not fit here */
	if (nocolor)
		KNRM = KRED = KGRN = KYEL = KBLU = KMAG = KCYN = KWHT = L"";


	argc = args.size();
	//Not sure how much of this you've already gone through?havent been going thru this much, ok well scan this real quick it's fairly straight-forward

	//No arguments (because 1st argument is './tau'). Default to 'prove' on an input stream, with quads as expected input format. This could either be interpreter, as in './tau', or a file as in './tau < examples/socrates'
	
	//in tclap you mean? im not sure what i mean :):) but you seem stuck in this..., ah not stuck so much as it just gets into other code and there are some questions/things to be agreed upon /i think i just have to try to see, do your thing :)ok
	
	if ( argc == 1 ) {
		prove_cmd p;
		quad_in = true;
		return p({L"",L"",L"",L""});
	}

	//Arguments
	//If first argument not found in cmds.first, and is not "help" command
	if (( cmds.first.find ( args[1] ) == cmds.first.end() && args[1] != L"help" ) ) {
		print_usage ( cmds );
		return 1;
	}
	//If the first argument is "help" and there's a command listed after it, i.e. './tau help prove'
	if ( args[1] == L"help" && argc == 3 ) {
		dout << ws(cmds.first[string ( args[2] )]->help());
		return 0; 
	}

	//If the first argument is "help" and there's no command listed after it, i.e. './tau help'
	if ( args[1] == L"help" && argc == 2 ) {
		print_usage ( cmds );
		return 0;
	}

	//If there are legal commands, but it's not 'help'
	int rval = ( *cmds.first[args[1]] ) ( args );
//look..how about i try to reorganize it all a bit? 






	return rval;
} 
