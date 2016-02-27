#include <stdio.h>
#include <sstream>
#include <tuple>
#include <iostream>
#include <queue>
#include <stack>
#include <set>
#include <stdexcept>

#include "univar.h"
#include "jsonld_tau.h"

#ifdef with_marpa
#include "marpa_tau.h"
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#ifdef debug_cli
#define CLI_TRACE(x) TRACE(x)
#else
#define CLI_TRACE(x)
#endif

std::ostream& dout = std::cout;
std::ostream& derr = std::cerr;
std::istream& din = std::cin;

// to hold a kb/query string
string qdb_text;

enum Mode {COMMANDS, KB, QUERY, SHOULDBE, CPPOUT, OLD, RUN};

string format = "";
string base = "";

int result_limit = 123;
bool irc = false;
std::set<string> silence;
bool in_silent_part = false;
bool nocolor = false;
bool fnamebase = true;//?

std::map<string,bool*> _flags = {
		 {"nocolor",&nocolor}
		,{"deref",&deref}
		,{"irc",&irc}
		,{"shorten",&shorten}
		,{"base",&fnamebase}
};

std::vector<string> extensions = {"jsonld", "natural3", "natq", "n3", "nq"};
std::vector<string> _formats = {
								#ifdef with_marpa
								"n3",
								#endif
								#ifndef NOPARSER
								"nq",
								#endif
								#ifdef JSON
								"jsonld"
								#endif
};


yprover *tauProver = 0;

std::vector<qdb> kbs;

bool done_anything = false;


class Input
{
public:
//Structure
	bool interactive = false;
	bool do_reparse = true;
	std::string name;
	Mode mode = COMMANDS;
	int limit = 123;
	

	virtual string pop() = 0;
	virtual string pop_long() = 0;
	virtual void take_back() = 0;
	virtual void readline()	{};
	virtual bool end() = 0;
	virtual bool done() = 0;
};

class ArgsInput : public Input
{
public:
/*
	bool interactive = false;
	bool do_reparse = true;
	std::string name;
	Mode mode = COMMANDS;
	int limit = 123;
*/
	int argc;
	char **argv;
	int counter = 1;


	ArgsInput(int argc_, char**argv_)
	{
		argc = argc_;
		argv = argv_;
		name = "args";
	}


	bool end()
	{
		return counter == argc;
	}
	bool done()
	{
		return end();
	}
	string pop()
	{
		assert(!end());
		return argv[counter++];
	}
	string pop_long()
	{
		return pop();
	}
	void take_back()
	{
		counter--;
		assert(counter);
	}
};



class StreamInput : public Input
{
public:
	std::istream& stream;
	string line;
	size_t pos;
	std::stack<size_t> starts;

	void figure_out_interactivity()
	{
		//if its a file
		std::ifstream* s = dynamic_cast<std::ifstream*>(&stream);
		interactive = !s;
		//else its stdin
		if (!s) {
			//assert(stream == std::cin);//sometimes doesnt compile
			//but its not attached to a tty
			if (!isatty(fileno(stdin)) && !irc)
				interactive = false;
		}
	}

	StreamInput(string fn_, std::istream& is_) : stream(is_)
	{
		name = fn_;
		figure_out_interactivity();
	}
	bool done()
	{
		return stream.eof();
	}
	bool end()
	{
		bool r = pop_long() == "";
		take_back();
		return r;
	}

	void readline()
	{
		std::getline(stream, line);
		while(!starts.empty()) starts.pop();
		pos = 0;

		auto m = stream.rdbuf()->in_avail();
		//TRACE(dout << m << endl);
		do_reparse = interactive && m <= 0;
		/* got_more_to_read()? this isnt guaranteed to work.
		 * i would just use select here, like http://stackoverflow.com/a/6171237
		 * got any crossplatform idea?
		 */
	}
	string pop_x(char x)
	{

		while(line[pos] == ' ') pos++;
		size_t start = pos;
		while(line[pos] != x && line[pos] != '\n' && line[pos] != 0) pos++;
		size_t end = pos;
		string t = line.substr(start, end);
		starts.push(start);
		return t;
	}
	string pop()
	{
		return pop_x(' ');
	}
	string pop_long()
	{
		return pop_x(0);
	}
	void take_back()
	{
		assert(starts.size());
		pos = starts.top();
		starts.pop();
	}
};


std::stack<Input *> inputs;
#define INPUT (inputs.top())













void fresh_prover()
{
	if (tauProver)
		delete tauProver;
	tauProver = new yprover(merge_qdbs(kbs));
}


void set_mode(Mode m)
{
	dout << "mode = ";
	switch(m) {
		case COMMANDS:
			dout << "commands";
			break;
		case KB:
			dout << "kb";
			break;
		case QUERY:
			dout << "query";
			break;
		case SHOULDBE:
			dout << "shouldbe";
			break;
		case CPPOUT:
			dout << "cppout";
			break;
		case OLD:
			dout << "old";
			break;
		case RUN:
			dout << "run";
			break;
	}
	dout << endl;
	INPUT->mode = m;
}

void help(){
	if(INPUT->end()){
		dout << "Help -- commands: kb, query, help, quit; use \"help <topic>\" for more detail." << endl;
		dout << "command 'kb': load a knowledge-base." << endl;
		dout << "command 'query': load a query and run." << endl;
		dout << "command 'help': Tau will help you solve all your problems." << endl;
		dout << "command 'quit': exit Tau back to terminal" << endl;
		dout << "\"fin.\" is part of the kb/query-loading, it denotes the end of your rule-base" << endl;
	}
	else{
		string help_arg = INPUT->pop();
		string help_str = "";
		if(help_arg == "kb"){
			help_str = "command 'kb': load a knowledge-base.";
		}
		else if(help_arg == "query"){
			help_str = "command 'query': load a query and run.";
		}
		else if(help_arg == "help"){
			help_str = "command 'help': Tau will help you solve all your problems.";
		}
		else if(help_arg == "quit") {
			help_str = "command 'quit': exit Tau back to terminal";
		}
		else if(help_arg == "fin"){
			help_str = "\"fin.\" is part of the kb/query-loading, it denotes the end of your rule-base";
		}else{
			dout << "No command \"" << help_arg << "\"." << endl;
			return;
		}
		dout << "Help -- " << help_str << endl;
	}
}


void switch_color(){
	if(nocolor){
                KNRM = KRED = KGRN = KYEL = KBLU = KMAG = KCYN = KWHT = "";
	}else{
		KNRM = "\x1B[0m";
		KRED = "\x1B[31m";
		KGRN = "\x1B[32m";
		KYEL = "\x1B[33m";
		KBLU = "\x1B[34m";
		KMAG = "\x1B[35m";
		KCYN = "\x1B[36m";
		KWHT = "\x1B[37m";
	}
}

bool _shouldbe(qdb &sb) {
	if (sb.first.empty() && sb.second.empty()) {
		return tauProver->results.empty();
	}
	if(!tauProver->results.size())
		return false;
	auto r = tauProver->results.front();
	tauProver->results.pop_front();
	return qdbs_equal(r, sb);
}



void test_result(bool x) {
	dout << INPUT->name << ":test:";
	if (x)
		dout << KGRN << "PASS" << KNRM << endl;
	else
		dout << KRED << "FAIL" << KNRM << endl;
}


void shouldbe(qdb &sb) {
	test_result(_shouldbe(sb));
}

void thatsall()
{
	test_result(tauProver->results.empty());
}

void clear_kb(){
	kbs.clear();
}


#ifndef NOPARSER
ParsingResult parse_nq(qdb &kb, qdb &query, std::istream &f)
{
	//We can maybe remove this class eventually and just
	//use functions? idk..
	nqparser parser;
        try {
                parser.parse(kb, f);
        } catch (std::exception& ex) {
                derr << "[nq]Error reading quads: " << ex.what() << endl;
                return FAIL;
        }
        try {
                parser.parse(query, f);
        } catch (std::exception& ex) {
                derr << "[nq]Error reading quads: " << ex.what() << endl;
                return FAIL;
        }
        return COMPLETE;
}
#endif



ParsingResult _parse(qdb &kb, qdb &query, std::istream &f, string fmt)
{
	CLI_TRACE(dout << "parse fmt: " << fmt << endl;)
#ifdef with_marpa
    if(fmt == "natural3" || fmt == "n3") {
		//dout << "Supported is a subset of n3 with our fin notation" << endl;
		return parse_natural3(kb, query, f, base);
	}
#endif
#ifndef NOPARSER
	if(fmt == "natq" || fmt == "nq" || fmt == "nquads")
		return parse_nq(kb, query, f);
#endif
#ifdef JSON
	if(fmt == "jsonld"){
		return parse_jsonld(kb, f);
	}
#endif
	return FAIL;
}

string fmt_from_ext(string fn){
	string fn_lc(fn);
	boost::algorithm::to_lower(fn_lc);

	for (auto x:extensions)
		if (boost::ends_with(fn_lc, x))
			return x;

	return "";
}

ParsingResult parse(qdb &kb, qdb &query, std::istream &f, string fn) {
	string fmt = format;
	if (fmt == "")
		fmt = fmt_from_ext(fn);
	if (fmt != "")
		return _parse(kb, query, f, fmt);
	else
	{
		ParsingResult best = FAIL;
		for (auto x : _formats) {
			ParsingResult r = _parse(kb, query, f, x);
			if (r > best)
			{
				if (r==COMPLETE)
					return r;
				best = r;
			}
		}
		return best;
	}
	return FAIL;
}


ParsingResult get_qdb(qdb &kb, string fname){
	std::ifstream is(fname);

	if (!is.is_open()) {
		dout << "failed to open file." << std::endl;
		return FAIL;
	}

	qdb dummy_query;

	auto r = parse(kb, dummy_query, is, fname);
	dout << "qdb graphs count:"<< kb.first.size() << std::endl;

	/*
	int nrules = 0;
	for ( pquad quad :*kb.first["@default"])
		nrules++;
	dout << "rules:" << nrules << std::endl;
	*/

	return r;
}

/*
int count_fins()
{
	int fins = 0;
	std::stringstream ss(qdb_text);
	do {
		string l;
		getline(ss, l);
		if(!ss.good())break;
		std::trim(l);
		if (l == "fin.") fins++;
	}
	return fins;
}
*/
int count_fins()
{
	int fins = 0;
	string line;
	stringstream ss(qdb_text);
	while (!ss.eof()) {
		getline(ss, line);
		if (startsWith(line, "fin") && *wstrim(line.c_str() + 3) == ".")
			fins++;
	}
	return fins;
}

bool dash_arg(string token, string pattern){
	return (token == pattern) || (token == "-" + pattern) || (token == "--" + pattern);
}


void get_int(int &i, const string &tok)
{
	try
	{
		i = std::stoi(tok);
	}
	catch (std::exception& ex)
	{
		dout << "bad int, ";
	}
}

bool read_option(string s){
	if(s.length() < 2 || s.at(0) != '-' || s == "--")
		return false;
	
	while(s.at(0) == '-'){
		s = s.substr(1, s.length()-1);
	}

	string _option = s;

	for( std::pair<string,bool*> x : _flags){
		CLI_TRACE(dout << _option << _option.size() << x.first << x.first.size() << std::endl;)
		if(x.first == _option){
			*x.second = !(*x.second);
			if(x.first == "nocolor") switch_color();
			return true;
		}
	}
	
	for(string x : _formats){
		if(x == _option){
			format = x;
			dout << "input format:"<<format<<std::endl;
			return true;
		}
	}

	if (!INPUT->end()) {
		string token = INPUT->pop();
	
		if(_option == "silence") {
			silence.emplace(token);
			CLI_TRACE(dout << "silence:";
			for(auto x: silence)
				dout << x << " ";
			dout << endl;)
			return true;
		}
	

		if(_option == "level"){
			get_int(level, token);
			dout << "debug level:" << level << std::endl;
			return true;
		}

		if(_option == "limit"){
			get_int(INPUT->limit, token);
			dout << "result limit:" << result_limit << std::endl;
			return true;
		}
		
		INPUT->take_back();
	}

	return false;
}



void do_run(string fn)
{
	std::ifstream &is = *new std::ifstream(fn);
	if (!is.is_open()) {//weird behavior with directories somewhere around here
		dout << "[cli]failed to open \"" << fn << "\"." << std::endl;
	}
	else {
		dout << "[cli]loading \"" << fn << "\"." << std::endl;
		inputs.push(new StreamInput(fn, is));
	}
}
//err
void run()
{
	set_mode(RUN);
}

void do_query(qdb &q_in)
{
	dout << "query size: " << q_in.first.size() << std::endl;
	result_limit = INPUT->limit;
	tauProver->query(q_in);
	done_anything = true;
}


void cmd_query(){
	if(kbs.size() == 0){
		dout << "No kb; cannot query." << endl;
	}else{
		if(INPUT->end()){
			set_mode(QUERY);
		}else{
			if(!INPUT->end()){
				string fn = INPUT->pop_long();
				qdb q_in;
				ParsingResult r = get_qdb(q_in,fn);
				if (r != FAIL)
					do_query(q_in);
			}
		}
	}
}


void add_kb(string fn)
{
	qdb kb;
	ParsingResult r = get_qdb(kb,fn);
	if (r != FAIL)
		kbs.push_back(kb);
}


void cmd_kb(){
	if(INPUT->end()){
		clear_kb();
		set_mode(KB);
	}else{
		string token = INPUT->pop();
		if(dash_arg(token,"clear")){
			clear_kb();
			return;
		}
		/*else if(dash_arg(token,"set")){
			clear_kb();
		}*/
		else if(dash_arg(token,"add")){
			//dont clear,
			if (INPUT->end())
				throw std::runtime_error("add what?");
			else
				add_kb(INPUT->pop_long());
		}else{
			clear_kb();
			add_kb(token);
		}	
	}
}


//print the prompt string differently to
//specify current mode:
void displayPrompt(){
	if (INPUT->interactive) {
		string prompt;
		switch(INPUT->mode) {
			case OLD:
				prompt = "old";
				break;
			case COMMANDS:
				prompt = "tau";
				break;
			case KB:
				prompt = "kb";
				break;
			case RUN:
				prompt = "run";
				break;
			case QUERY:
				prompt = "query";
				break;
			case SHOULDBE:
				prompt = "shouldbe";
				break;
			case CPPOUT:
				prompt = "cppout";
				break;
		}
		std::cout << prompt;
		if (format != "")
			std::cout << "["<<format<<"]";
		std::cout << "> ";

	}
}




bool try_to_parse_the_line__if_it_works__add_it_to_qdb_text()
{
	string x = qdb_text + INPUT->pop_long() + "\n";

	if (!INPUT->do_reparse) {
		qdb_text = x;
	}
	else {
		qdb kb, query;
		std::stringstream ss(x);
		int pr = parse(kb, query, ss, "");
		dout << "parsing result:" << pr << std::endl;
		if (pr) {
			qdb_text = x;
		}
		else {
			dout << "[cli]that doesnt parse, try again" << std::endl;
			return false;
		}
	}
	return true;
}










void emplace_stdin()
{
	inputs.push(new StreamInput("stdin", std::cin));
}







int main ( int argc, char** argv)
{
  //This should probably go logically with other initialization stuff.
	//Initialize the prover strings dictionary with hard-coded nodes.
	dict.init();


	//start by processing program arguments
	inputs.emplace(new ArgsInput(argc, argv));

	while (true) {

		displayPrompt();
		INPUT->readline();

		//maybe its time to go to the next input
		while (INPUT->done()) {
			auto popped = inputs.top();
			inputs.pop();
			/*if there werent any args, drop into repl*/
			if (dynamic_cast<ArgsInput*>(popped))
				if (!done_anything)
					emplace_stdin();
			if (!inputs.size())
				goto end;

			displayPrompt();
			INPUT->readline();
		}


		if (INPUT->mode == COMMANDS) {
			string token = INPUT->pop();
			if (startsWith(token, "#") || token == "")
				continue;
			else if (read_option(token))
				continue;
			else if (token == "help" || token == "halp" || token == "hilfe")
				help();
			else if (token == "kb")
				cmd_kb();
			else if (token == "query")
				cmd_query();
			else if (token == "shouldbe")
				set_mode(SHOULDBE);
			else if (token == "cppout")
				set_mode(CPPOUT);
			else if (token == "thatsall")
				thatsall();
			else if (token == "run")
				run();
			else if (token == "shouldbesteps") {
				//test_result(std::stoi(read_arg()) == tauProver->steps_);
			}
			else if (token == "shouldbeunifys") {
				//test_result(std::stoi(read_arg()) == tauProver->unifys_);
			}
			else if (token == "quit")
				break;
			else if (token == "-")
				emplace_stdin();
			else {
				INPUT->take_back();
				//maybe its old-style input
				if (try_to_parse_the_line__if_it_works__add_it_to_qdb_text())
					set_mode(OLD);
				else
					dout << "[cli]that doesnt parse, try again" << std::endl;
				continue;
			}
		}
		else if (INPUT->mode == KB || INPUT->mode == QUERY || INPUT->mode == SHOULDBE || INPUT->mode ==  CPPOUT) {
			try_to_parse_the_line__if_it_works__add_it_to_qdb_text();
			int fins = count_fins();
			if (fins > 0) {

				qdb kb,kb2;

				std::stringstream ss(qdb_text);
				auto pr = parse(kb, kb2, ss, INPUT->name);

				if(pr == COMPLETE) {
					if (INPUT->mode == KB) {
						kbs.push_back(kb);
						fresh_prover();
					}
					else if (INPUT->mode == QUERY) {
						do_query(kb);
					}
					else if (INPUT->mode == SHOULDBE) {
						shouldbe(kb);
					}
					else if (INPUT->mode == CPPOUT) {
						tauProver->cppout(kb);
						done_anything = true;
					}
				}
				else
					dout << "error" << endl;
				qdb_text = "";
				set_mode(COMMANDS);
			}
		}
		else if (INPUT->mode == RUN) {
			do_run(INPUT->pop_long());
		}
		else {
			assert(INPUT->mode == OLD);
			try_to_parse_the_line__if_it_works__add_it_to_qdb_text();
			int fins = count_fins();
			if (fins > 1) {
				kbs.clear();
				qdb kb,kb2;
				std::stringstream ss(qdb_text);
				auto pr = parse(kb, kb2, ss, INPUT->name);
				dout << "querying" << std::endl;
				kbs.push_back(kb);
				fresh_prover();
				do_query(kb2);
				qdb_text = "";
				set_mode(COMMANDS);
			}
		}
	}


	end:
	if (tauProver)
		delete tauProver;
}
