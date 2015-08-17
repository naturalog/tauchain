#include <stdio.h>
#include "cli.h"
#include <sstream>
#include <tuple>
#include <iostream>
#include <queue>
#include "prover.h"
#include <stdexcept>
#ifdef with_marpa
#include "marpa_tau.h"
#endif
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>



#define ever ;;

std::wostream& dout = std::wcout;
std::wostream& derr = std::wcerr;
std::wistream& din = std::wcin;

std::deque<string> _argstream;
std::vector<string> _commands = {L"kb", L"query",L"run",L"quit"};
string _def_format = L"";

std::map<string,bool*> _flags = {
	{L"nocolor",&nocolor},
	{L"deref",&deref},
	//this one.
	{L"shorten",&shorten},
	{L"base",&fnamebase},
	{L"quads",&quad_in},
	{L"pause",&_pause}
};

std::vector<string> _formats = {L"jsonld", L"natural3", L"natq", L"n3", L"nq"};

qdb merge_qdbs(const std::vector<qdb> qdbs)
{
        if (qdbs.size() > 1)
                dout << L"warning, bnode renaming not implemented";
        qdb r;
        for (auto x:qdbs) {
                r.first.insert(x.first.begin(), x.first.end());
                r.second.insert(x.second.begin(), x.second.end());
        }
        return r;
}

std::vector<qdb> kbs;
prover *tauProver;

#ifndef NOPARSER
int parse_nq(qdb &r, std::wistream &f)
{
        try {
                readqdb(r, f);
        } catch (std::exception& ex) {
                derr << L"Error reading quads: " << ex.what() << endl;
                return 0;
        }
        return 2;
}
#endif


int parse(qdb &r, std::wistream &f, string fmt)
{
	dout << L"parse " << fmt << endl;
#ifdef with_marpa
        if(fmt == L"natural3" || fmt == L"n3") {
			dout << L"Supported is a subset of n3 with our fin notation" << endl;
			return parse_natural3(r, f);
		}
        else
#endif
        if(fmt == L"natq" || fmt == L"nq" || fmt == L"nquads")
                return parse_nq(r, f);

	if(fmt == L"jsonld"){
		dout << L"Cannot read JSON-LD format" << endl;
		return 0;
	}
 
       else
                throw std::runtime_error("unknown format");
}

string get_format(string fn){
        string fn_lc(fn);
        boost::algorithm::to_lower(fn_lc);

	for (auto x:_formats)
		if (boost::ends_with(fn_lc, x))
			return x;
        
	return _def_format;

}

bool dash_arg(string token, string pattern){
	return (token == pattern) || (token == L"-" + pattern) || (token == L"--" + pattern);
}

string read_arg(){
	string ret;
	if(_argstream.size() != 0){
		ret = _argstream.at(0);
		_argstream.pop_front();
	}else{
		dout << L"_argstream is empty." << endl;
	}
	return ret;	
}

bool is_command(string s){
	if(s.length() == 0) return false;
	if(s.at(0) == '-') s = s.substr(1,s.length()-1);
	if(s.length() == 0) return false;
	if(s.at(0) == '-') s = s.substr(1,s.length()-1);
	if(s.length() == 0) return false;
	for( string x : _commands){
		if(x == s){
			return true;
		}
	}	
	return false;
	
}

void switch_color(){
	if(nocolor){
                KNRM = KRED = KGRN = KYEL = KBLU = KMAG = KCYN = KWHT = L"";
	}else{
		KNRM = L"\x1B[0m";
		KRED = L"\x1B[31m";
		KGRN = L"\x1B[32m";
		KYEL = L"\x1B[33m";
		KBLU = L"\x1B[34m";
		KMAG = L"\x1B[35m";
		KCYN = L"\x1B[36m";
		KWHT = L"\x1B[37m";
	}
}

bool check_option(string s){
	if(s.length() == 0){
		return false;
	}
	if(s.at(0) != L'-' || s.length() < 2){
		return false;
	}
	int dash = 1;
	if(s == L"-" || s == L"--"){
		return false;
	}
	
	if(s.at(1) == L'-'){
		if(s.length() == 2){
			return false;
		}
		dash = 2;
	}

	string _option = s.substr(dash,s.length()-dash);

	for( std::pair<string,bool*> x : _flags){
		if(x.first == _option){
			*x.second = !(*x.second);
			if(x.first == L"nocolor") switch_color();
			return true;
		}
	}
	
	for(string x : _formats){
		if(x == _option){
			_def_format = x;
			return true;
		}
	}

	if(_option == L"level"){
		if(_argstream.size() != 0){
			string token = read_arg();
			int tmpLevel = level;
			try{
				tmpLevel = std::stoi(token);
			}catch(const std::invalid_argument& e){
				_argstream.push_front(token);
				return true;
			}
	
			if(tmpLevel < 1) 
				level = 1;
			else if(tmpLevel > 100) 
				level = 100;
			else
				level = tmpLevel;
			
		}
		return true;
	}
	return false;
}

      
int get_qdb(qdb &kb, string fname){
        std::wistream* pis = &std::wcin;
        if (fname != L"")
           pis = new std::wifstream(ws(fname));
        std::wistream& is = *pis;
	return parse(kb,is,get_format(fname));
}

void mode_query(){
	if(kbs.size() == 0){
		dout << L"No kb; cannot query." << endl;
	}else{
		if(_argstream.size() == 0){
                	qdb q_in;
                	int r = get_qdb(q_in,L"");
                	if(r == 2){
                        	(*tauProver).query(q_in);
				(*tauProver).e.clear();
                	}else if(r == 1 || r == 0){
                	        dout << L"Error parsing query input." << endl;
                	}else{
                	        dout << L"Return code from get_qdb(): '" << r << L"', is unrecognized. Exiting." << endl;
                        	assert(false);
			}
		}else{
			while(_argstream.size() > 0){
				string token = read_arg();
				if(is_command(token)){
					_argstream.push_front(token);
					break;
				}
				if(check_option(token)){
					continue;
				}
				qdb q_in;
				int r = get_qdb(q_in,token);
				if(r == 2){
                         	       (*tauProver).query(q_in);
				       (*tauProver).e.clear();	
                        	}else if(r == 1 || r == 0){
                        	       dout << L"Error parsing query file \"" << token << "\"" << endl;
                        	}else{
                        	        dout << L"Return code from get_qdb(): '" << r << L"', is unrecognized. Exiting." << endl;
                        	        assert(false);
                        	}
			}
		}
	}
}


void clear_kb(){
	kbs.clear();
}


void mode_kb(){
	if(_argstream.size() == 0){
		clear_kb();
		qdb kb_in;
		int r = get_qdb(kb_in,L"");
		if(r == 2){
			kbs.push_back(kb_in);
			tauProver = new prover(merge_qdbs(kbs));
		}else if(r == 1 || r == 0){
			dout << L"Error parsing kb input." << endl;
		}else{
			dout << L"Return code from get_qdb(): '" << r << L"', is unrecognized. Exiting." << endl;
			assert(false);
		}
		
	}else{
		string token = read_arg();
		if(dash_arg(token,L"clear")){
			clear_kb();
			return;
		}
		else if(dash_arg(token,L"set")){
			clear_kb();
		}
		else if(dash_arg(token,L"add")){
		}else{
			_argstream.push_front(token);
			clear_kb();
		}
	
		if(_argstream.size() == 0){
			qdb kb_in;
			int r = get_qdb(kb_in,L"");
			if(r == 2){
				kbs.push_back(kb_in);
				tauProver = new prover(merge_qdbs(kbs));
			}else if(r == 1 || r==0){
				dout << L"Error parsing kb input." << endl;
			}else{
				dout << L"Return value, '" << r << L"', of get_qdb() not recognized. Exiting." << endl;
				assert(false);
			}
		}else{
			while(_argstream.size() > 0){
				string token = read_arg();
				if(is_command(token)){
					_argstream.push_front(token);
					break;
				}
				if(check_option(token)){
					continue;
				}
				
			
				qdb kb_in;
				int r = get_qdb(kb_in, token);	
				if(r == 2){
					kbs.push_back(kb_in);
					tauProver = new prover(merge_qdbs(kbs));
				}else if(r == 1 || r == 0){
					dout<< L"Error parsing kb file: \"" << token << L"\"" << endl;
				}else{
					dout << L"Return value, '" << r << L"', of get_qdb() not recognized. Exiting." << endl;
					assert(false);
				}
			}
		}	
	}
}



void tau_shell(){
	dout << L"Tau> ";
	string in;

	if (std::wcin.rdstate() && std::wcin.eofbit) std::exit(0);

//i think this?
	//this pulls a line, the other one splits it by spaces ahh
ok well i guess we'd have to rework this to work off from a string instead

ie. getline(buffer, line)
wstringstream...yadayda
getline (xxx " ")
and we have what could be a command

	getline(std::wcin, in);
	std::wistringstream i(in);
//here, instead of just one _argstream, we'll have a line buffer
	///even rather, its just a string
	//delimited by new-lines?delimited for who?between each line of input
//yes between each line is a newline, but why care?to make sure we're putting that in our string
	//std::vector<string> line_v;
	string tmp;
	while(std::getline(i,tmp,L' ')){//so this means delimited by spaces? yep and newlines too?
		line_v.push_back(tmp);
	}
	//line_buffer.push_back(line_v);
}

void help(string help_arg){
	if(help_arg == L""){
		dout << L"Help -- commands: kb, query, help, quit; use \"help <command>\" for more detail." << endl;
	}else{
		string help_str = L"";
		if(help_arg == L"kb"){
			help_str = L"command 'kb': load a knowledge-base.";
		}
		else if(help_arg == L"query"){
			help_str = L"command 'query': load a query and run.";
		}
		else if(help_arg == L"help"){
			help_str = L"command 'help': Tau will help you solve all your problems.";
		}
		else if(help_arg == L"quit"){
			help_str = L"command 'quit': exit Tau back to terminal";
		}else{
			dout << "No command \"" << help_arg << "\"." << endl;
			return;
		}
		dout << "Help -- " << help_str << endl;
		
		
	}
}


int main ( int argc, char** argv) {
	dict.init();
//im still having trouble seeing how we know when we have a valid block
//what makes a 'block' becomes a lot of things with the new interactive stuff
///o/r ideally a very small number of building block things
// we are only talking about a block of code
//n 3 or qnq yea or jsonld! code ok
//so
//marpa tells if it's kb or kb+query
//new command-line needs to work with that 2nd one
//and there needs to be a special initial case for '<' still? why?
//well..
//no no no: ) not a special case really just a bit of code like...
//it was only looking at argv
	//like this
//theres absolutely nothing special about it..look..i need to get some food first,
//but i think i have a good idea how to shuffle this stuff ok :) sounds like a plan
ok:)
	for(int i=1;i<argc;i++){
		_argstream.push_back(ws(argv[i]));
	}

//ok so we come into "./tau < examples/socrates"
//but it's not thinking to look at stdin right now
//it only looks at argv and loads that up into _argstream
//and then because i have a continuously looping Tau> shell
//hooked up to std::in, then once it gets into Tau> shell it
//starts reading the file lol
//and magically it parses '< examples/socrates'
/*look... the < sends the file contents into the stdin of tau,
i dont get it.. yes. it sends the file contents to stdin of tau.

and interactive mode handles it...thats the intention, no that can
only happen from the terminal command line, so it just happen once
here and be a special case, because we'll never see anything like that
again for the rest of the duration of the tau session

what can happen? what do you mean? i dont understand what you meant..well..
theres this bit of code i found for telling if we have stdin connected to a console
or a pipe, thats not enough for what you want? what do you want to happen when we
pipe something to tau as opposed to interactive mode?

..thats a good question
like, i can do python and type something in or i can pipe it in, there must be no difference, ah but here there is a difference
because we don't pipe in actual tau shell stuff, we only pipe in pure like nq/n3 files

ah ok, so thats a different problem, yea basically this is to make it back-compatible withall the old stuff to make all the previous examples work correctly and have that kb/query same-file format...yeah..cant we just handle it with some smartness? yea i just dont have any XD i think you started on it yesterday? if it doesnt match any command, try to read it ... hm.. what if my first line is 'kb add filename' <-- a *rule* in a nq file. its not, thered have to be a period..but yeah..this kind of thing risks some ambiguity, so thats why i'm trying to leave my tau-shell parsing and the other parsing completely separate sub-systems so we aren't just admixing all over the place. well, on the positive side, there would still be unambiguous ways to input things, sure, i've added some things to be able to make it unambiguous, but, we don't want to be mixing between this and the file parsing. it should be two distinct phases of 'ok we're taking cli' and 'ok we're taking a file'. no. hah
a iif  we don't then HMC will break it to pieces XD
i dont think taking the difference between pipe and console and understanding it as the difference between old and new format makes really much sense
so.. enih no thats not what i meant
yes you say if we are connected to a terminal, parse new style , if to a pipe, old style, no thats not what i meant hmm then what? i didn't mean admixing of new vs old, i meant admixing of cli vs. nq/n3 file, it shouldn't just be one continuous stream of maybe i'm in one maybe i'm in the other, that should be 2 distinct phases, with 2 separate subsystems handling it, otherwise HMC will break us with injection attacks

i dont think it will be more vulnerable than the rest of the code.. it is XD it already islol, especially when i hook into tau-bot and he has yet another layer of injections to play with, well tau bot-will never, while tau is in development, never can be considered secure, sure i'm just saying we want to avoid admixing distinct sources of data/data-streams otherwise we risk injection attacks and to avoid them we have to get really down in detail to make sure we've designed our two sub-systems to work perfectly together, instead of just have your sub-system of parsing files be a distinct phase so we're not worrying if it's accidentally doing command-line parsing, or if i'm accidentally parsing files from commandline etc... sorry but youre really overthinking it, i mean, we have like this
./tau < examples/socrates_from_mallory

socrates_from_mallory:
	kb add mallory's_malicious_kb

oops we had it all parsing in the same continuous stream with ambiguity?
now if we just feed in this file, it will go onto our stdin
but our stdin will parse this like command line
and
then we add mallory's kb
dont parse untrusted strings in default ambiguous mode..in fact, until everything is finished and proven and checked over 20 times, dont parse untrusted strings, period

ok, i just want to check if there's something on stdin when the program starts, and if
there is then i attempt to load it like a kb/query combo.
ok...i suppose its as bad idea as all the other solutions, let me look it up for you
but ... its eeek..its not unixy imho
id much rather prefer an interesting smart thingy that makes sense of its input, we can do that at some point i just want something quick that won't break the examples for now, 
std::istream::peek

int peek();

Peek next character
Returns the next character in the input sequence, without extracting it: The character is left as the next character to be extracted from the stream.
Return Value
The next character in the input sequence, as a value of type int.

If there are no more characters to read in the input sequence, or if any internal state flags is set, the function returns the end-of-file value (EOF), and leaves the proper internal state flags set:
	eofbit	No character could be peeked because the input sequence has no characters available (end-of-file reached).
failbit	The construction of sentry failed (such as when the stream state was not good before the call).
badbit	Error on stream (such as when this function catches an exception thrown by an internal operation).
When set, the integrity of the stream may have been affected.
	
	int c = std::cin.peek();  // peek character
	if ( c == EOF ) {//that smart stuff would actually tie in to being smart
in irc mode, trying to parse input, rejecting failing lines..i
heres the problem though, the old format doesn't have any kb/query specifiers
so. how does it know if it's 2 kbs, or 1 kb and 1 query? its always the second case that doesnt really tie in with interactive mode though it does
		//ok so, we have a default mode for '< [filename]'hat
		//[filename] is intended to be in the combo kb/query format
		//we can add some smart stuff to see if it's just a kb
		//i dont want to mess with having any cli stuff in the files yet
	}
	
	for(ever){
		if(_argstream.size() == 0){
			tau_shell();
		}else{//so lets start with, how would we determine when we've got a full block marpa tells you..or..maybe it doesnt right now, let me think about it a bit, sure, i'm thinking like the case where we have like, just a query, or just an option/flag
you have some tokenizer somewhere here, right?
			string token = read_arg();
			if(check_option(token)){
				continue;
			}
	/hmhmhmhmhmhmh		
			if(token == L"help"){
				string help_arg = L"";
				if(_argstream.size() > 0){
					help_arg = read_arg();
				}
				help(help_arg);
				continue;	
			}
			
			if(token == L"kb"){
				mode_kb();
				continue;
			}

			if(token == L"query"){
				mode_query();
				continue;
			}
			if(token == L"quit"){
				break;
			}

			//not a command then we'll attempt to parse it like a file.
			//well.. there's a couple things, but
/*so, not a command, so we keep some buffer for this input yeah? o
we try if this buffer + this line parses
if it does, we add it to the buffer
we know if its an incomplete or complete parse
and we know if its just kb or also query..marpa returns all that info
and we can make nq parser return it too
if the line doesnt parse, we just say so and throw it away
well..you see, i thought about this.. hmm.. it works for me
i was just going for a quickfix but this would probably be better
so, we're defining a grammar mm

			dout << "No command \"" << token << "\"." << endl;
		}
		
	}
}
