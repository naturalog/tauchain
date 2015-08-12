#include <stdio.h>
#include "cli.h"
#include <sstream>
#include "prover.h"
#ifdef with_marpa
#include "marpa_tau.h"
#endif
#include <tclap/CmdLine.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

extern int level;
bool autobt = false, _pause = false, __printkb = false, fnamebase = true, nocolor = false;

#ifdef DEBUG
auto dummy = []() {
	return ( bool ) std::cin.tie ( &std::clog );
}();
#endif
#ifdef JSON
jsonld_options opts;
#endif


std::wostream& dout = std::wcout;
std::wostream& derr = std::wcerr;


/*
 *
 * return value of parse functions: 0: error, 1: incomplete input, 2: success
 *
 * */


#ifndef NOPARSER
int parse_nq(qdb &kb, qdb &q, std::wistream &f)
{
	try {
		readqdb(kb, f);
	} catch (std::exception& ex) {
		derr << L"Error reading quads: " << ex.what() << std::endl;
		return 0;
	}
	try {
		readqdb(q, f);
	} catch (std::exception& ex) {
		derr << L"Error reading quads: " << ex.what() << std::endl;
		return 0;
	}
	return 2;
}
#endif



void parse(qdb &kb, qdb &q, std::wistream &f , std::string fn, std::string fmt)
{
	boost::algorithm::to_lower(fmt);

	std::vector<std::string> exts({"jsonld", "natural3", "natq", "n3", "nq"});

	std::string fnl(fn);
	boost::algorithm::to_lower(fnl);

	if (fmt == "") // try to guess from file extension
	{
		for (auto x:exts)
			if (boost::ends_with(fnl, x))
				fmt = x;
	}

	if (fmt == "") // default
		fmt = "natq";

#ifdef with_marpa
	if(fmt == "natural3" || fmt == "n3")
		parse_natural3(kb, q, f);
	else
#endif
	if(fmt == "natq" || fmt == "nq" || fmt == "nquads")
		parse_nq(kb, q, f);

/*		else
		jsonld*/
	else
		throw std::runtime_error("unknown format");
}

typedef std::pair<std::string, std::string> fn_fmt;

int process_args(std::vector<std::string> args)
{
	if (args.size() == 0)
		{}//do interactive quads

	std::vector<fn_fmt>  inputs;

	std::string fmt, fn;

	for (std::string x: args) {
		if (boost::starts_with(x, "--"))
			fmt = std::string(x.begin() + 2, x.end());
		else
			fn = x;
		if (fn != "") {
			inputs.push_back(fn_fmt(fn, fmt));
			fn = "";
		}
	}
	//now that we have optionally collected a format

	if (inputs.size() == 0)
	{
		qdb kb, query;
		parse(kb, query, std::wcin, "", fmt);
		prover prvr(kb);
		TRACE(dout << "QUERYING" << std::endl);
		prvr.query(query);
		return 0;
	}

	uint pos = 0;
	std::vector<qdb> kbs;
	for (auto x: inputs) {
		std::string fn = x.first;
		std::string fmt = x.second;
		qdb kb;
		qdb query;

		std::wifstream f(fn);
		if (!f.is_open())
			throw std::runtime_error("couldnt open file \"" + fn + "\"");

		parse(kb, query, f, fn, fmt);

		if(++pos == inputs.size())
		{
			if(query.first.size()) {
				kbs.push_back(kb);
				prover prvr(merge_qdbs(kbs));
				TRACE(dout << "QUERYING" << std::endl);
				prvr.query(query);
			}
			else {
				if (kbs.size() == 0)
					throw std::runtime_error("would use last file as query, but have no kb");
				prover prvr(merge_qdbs(kbs));
				TRACE(dout << "QUERYING" << std::endl);
				prvr.query(kb);
			}
		}
		else {
			kbs.push_back(kb);
			if (query.first.size())
				dout << L"ignoring query in " << ws(fn) << std::endl;
		}
	}
	return 0;
}


typedef std::map<TCLAP::SwitchArg*,bool*> tcFlags;

int main ( int argc, char** argv ) {
	dict.init();
	std::vector<std::string> args;

	try {
		TCLAP::CmdLine cmd("Tau-Chain by http://idni.org. ", ' ', "0.0");

		TCLAP::ValueArg <int> tc_level("l", "level", "debug level", false, 1, "", cmd);

		TCLAP::SwitchArg
				tc_deref("d", "no-deref", "show integers only instead of strings", cmd, true),
				tc_pause("P", "pause",
						 "pause on each trace and offer showing the backtrace. available under -DDEBUG only.", cmd,
						 false),
				tc_shorten("s", "shorten", "on IRIs containing # show only what's after #", cmd, false),
				tc_base("b", "base", "set file://<filename> as base in JsonLDOptions", cmd, true),
				tc_nocolor("n", "nocolor", "disable color output", cmd, false);

		TCLAP::UnlabeledMultiArg<std::string> multi("stuff", "file names, optionally prefixed with formats", false, "typedesc", cmd);

		cmd.parse(argc, argv);

		level = tc_level.getValue();
		deref = tc_deref.getValue();
		_pause = tc_pause.getValue();
		shorten = tc_shorten.getValue();
		fnamebase = tc_base.getValue();
		nocolor = tc_nocolor.getValue();
		//sorry stoopkid

		args = multi.getValue();

	} catch (TCLAP::ArgException &e) {
		derr << "TCLAP error: " << e.what() << std::endl;
	}

	if (nocolor)
		KNRM = KRED = KGRN = KYEL = KBLU = KMAG = KCYN = KWHT = L"";

	return process_args(args);
}


#ifdef xxxxxxxxxxxxxxxxxxxxxxxxxxBLIMP


int process_args(args)
{
	int pos = 0;
	prover prvr;
	for (std::string x: args) {
		blimp(prvr, x, ++pos = args.size());
	}
}

void blimp(prover &prvr, std::string x, int togo) {
	static std::string fmt;
	std::string fn;
	static std::string block;

	const int COMMANDS = 0;
	const int KB = 1;
	const int QUERY = 2;
	const int WORK = 3;
	static int phase = 0;

	if (boost::starts_with(x, "--"))
	{
		/*
		if (x == "--pass")
		 {
		 if(!togo)
		 dump
		 else error
		 } else
		 */
		fmt = std::string(x.begin() + 2, x.end());
	}

	else {
		fn = x;
		//try to load file, return
	}
	if (x == "fin." || x == "fin .") {
		phase = phase + 1;
		if (phase== WORK) { }
	}
	block += x + "\n";
	if (fmt != "")
	{
		int r = parse(x, fmt);
	}
/*
	{
		qdb xx = ;
		prvr.add_qdb(load_file(x, fmt));
		else {
			qdb query;
			if
				load_file(query, query_format, query_fn);

		}
		fmt = "";
	}
*/
		/*
		if (name=="-"// or name == "")
			std::wistream* pis = &std::wcin;
		else.
			pis = new std::wifstream(ws(fname));
		std::wistream& is = *pis;
		ifstream blabla f(file.first);
		load_file(kb,..
		 */
}

/*
	}
	else
		print_qdb(kb)

	return rval;
 *//*
}
			} catch (std::exception& ex) { dout<<ex.what()<<std::endl; }
			catch (...) { dout<<"generic exception."<<std::endl; }
			sleep(1);
			}
*/

/*
 *
 *
 * chat
 *
 *
 *
 *

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
*/


#endif
