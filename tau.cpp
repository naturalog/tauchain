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
	//this is cli stuff, and there's cli stuff in marpa, etc. it gets slightly
	//extensive before all the loose ends get tied together
	//hm.. well actually i can just continue to match ohad's structure there (possibly)
	//right now cli stuff is scattered and doesn't perform what you asked for
	//im offering that you make tclap or whatever produce a list of files with formats,
	//some --query thrown in, some no-flags case, whatever, and from there
	/*we can start reshaping all the mess...thats how i wouldddddddddddddddddddddn
do it, if that doesnt work for you, i guess i will more or less leave you to your
devices:)ah sure, that's what i'm trying to do now :)the files list?all the command line stuff
//including that..ok..well..do your thing then

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
		
		TCLAP::ValueArg<int> tc_level("l","level","level",false,1,"",cmd);
		//
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

	/*hmm.. where can this go... does not fit here */
	if (nocolor)
		KNRM = KRED = KGRN = KYEL = KBLU = KMAG = KCYN = KWHT = L"";


	vector<std::pair<string, string>>input;

	/*magic goes here*/
	/*test*/
	input.push_back("short.natural3", "natural3");
	
	/*optionally, if we didnt get format, try to guess?*/
	
	qdb kb;
	for file in input:
	{
		string format = file.second;
		string name = file.first;
		if (name=="-" or name == "")
			std::wistream* pis = &std::wcin;
		else.
			pis = new std::wifstream(ws(fname));
		std::wistream& is = *pis;
//exactly, if the rest of the command line logic were set up over this it would be
//what i described below. because from here i imagine something like a 'batch file'
//which would just be a really long command line line..stuck into a file and
//interpreted with some option like --batch
//i mean as far as simplicity and consistency goes i think its pretty solid,
//"everything works the same wherever you're doing it... basically"

//yeah it sounds good
//this will definitely be good when its more complete and we get to bigger more serious
//testing, especially the batch files. two commands :) i'm thinking of is --kb and
//--kb_append and maybe 3 with a --kb_clear or something
//also in interpeter mode what happens if you enter a bad line? does it fail out or
//does it just give you a warning and give you a new line? ofc if we have the
//integration between command line, interpreter and batch files then in some cases
//we'd want one, and in some cases we'd want the other, sounds like a command line option
//--fail_on_error or --continue_on_error or something like that
//also, what about merging multiple files into a single kb?
//--kb file.n3 file.json file.nq  --query qfile1 qfile2 --kb-append file.n3 --query qfile3
// --kb-clear --kb_fail_on_error true  --kb file.n3  file2.n3 and etc.......
// ^ supposed to all be one command line
//and the -- options are just what you would do in between "fin."s and starting the
//next 'activity' (--kb, --kb-append, --query) etc. etc.

//these 'activities' not really any different whether its cli, batch file, interpreter
//mode or IRC bot (and especially these last two are identical)

//thats my thoughts of it
//ok...i think you should run it past HMC
//sure lets see if hes around
		ifstream blabla f(file.first);
		load_file(kb,..
	}
	
	if (have_query)
	{
		qdb query;
		if
		load_file(query, query_format, query_fn);
		
		
	}
	else
		print_qdb(kb)
	
	
	
	
	
	
	
	//ok, now i need to think it thru a bit more, so it will work more or less
	//like now.................
	//one thing i was thinking is that load_quads has it set up so that it will
	//work over both files and stdin in the interpreter mode (which i've got 
	//prefixed with 'Tau>' now btw), was thinking whatever we set up the same things
	//could be done in interpreter mode and it would be essentially the same interface
	//command line commands essentially the same commands you'll give inside the
	//interpreter to direct the course of things. like say i want to run another query
	//after i'm done with my first one. right now Tau just exits.. but why?
	//is essentially the same as just reading the next file and doing something with it
	//from the commandline
	//also brings up the idea of "quit" command :) which could actually have use
	//already when i get into interpreter and didn't mean to.. it's cleaner and more
	//considerate than making them Ctrl+Z, even though i personally have no problems
	//with Ctrl+Z it's the thought that counts :) and could be a bigger thing later
	//if we need more graceful tau shutdown, which i'm sure we would down the road.
	//i agree down the road some sort of interpreter mode could be handy
	//i figure that's already what we have and if we expanded the command line like
	//we're talking about but kept it like it is now where it handles both files
	//and 'interpreter', i.e. plain './tau', then that's exactly what we'd get :)
	//tangential: we should bring back the IRC bot
	//actually not completely tangential :)
	//well..i will let you think about it:)
	//command line commands, interpreter mode, irc mode(?), im not sure how
	//to really approach any of it now
	//basically just like we are now in the code except with the revisions of the command
	//line like we've been talking about, and making these command line commands accessible
	//in the interpreter
	//basically, yeah
	//basically an interpreter and the command line is no different except one has input
	//from the user from the terminal and the other has input from files, and the commands/flags
	//are all the same, and it all works exactly the same either way you do it
	
	
	
	return rval;
} 

load_file(qdb &kb, format, ifstream f)
{
		
		if(format == "natural3")
			load_natural3(kb, f);
		else..
		nq
		else
		jsonld
}
