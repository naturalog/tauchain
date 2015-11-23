#include <stdio.h>
#include <sstream>
#include <tuple>
#include <iostream>
#include <queue>
#include <stack>
#include <set>
#include <stdexcept>

#include "univar.h"
#include "cli.h"

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

std::wostream& dout = std::wcout;
std::wostream& derr = std::wcerr;
std::wistream& din = std::wcin;

std::deque<string> _argstream;

enum mode_t {COMMANDS, KB, QUERY, SHOULDBE, OLD};
mode_t mode = COMMANDS;

enum ParsingResult {FAIL, /* INCOMPLETE, */ COMPLETE};

string format = L"";
string base = L"";

int result_limit = 123;
bool irc = false;
std::set<string> silence;
bool in_silent_part = false;

std::map<string,bool*> _flags = {
		{L"nocolor",&nocolor}
		,{L"deref",&deref}
		,{L"irc",&irc}
		,{L"shorten",&shorten}
		,{L"base",&fnamebase}
};

std::vector<string> extensions = {L"jsonld", L"natural3", L"natq", L"n3", L"nq"};
std::vector<string> _formats = {L"nq",
								#ifdef with_marpa
								L"n3",
								#endif
								L"jsonld"};
std::vector<string> _commands = {L"kb", L"query",L"run",L"quit"};

yprover *tauProver = 0;

using namespace old;

std::vector<qdb> kbs;

void fresh_prover()
{
	if (tauProver)
		delete tauProver;
	tauProver = new yprover(old::merge_qdbs(kbs));
}


void set_mode(int m)
{
	dout << L"mode = ";
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
	}
	dout << endl;
	mode = m;
}

void help(string help_arg){
	if(input.end()){
		dout << L"Help -- commands: kb, query, help, quit; use \"help <topic>\" for more detail." << endl;
		dout << L"command 'kb': load a knowledge-base." << endl;
		dout << L"command 'query': load a query and run." << endl;
		dout << L"command 'help': Tau will help you solve all your problems." << endl;
		dout << L"command 'quit': exit Tau back to terminal" << endl;
		dout << "\"fin.\" is part of the kb/query-loading, it denotes the end of your rule-base" << endl;
	}
	else{
		old::string help_arg = input.pop();
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
		else if(help_arg == L"quit") {
			help_str = L"command 'quit': exit Tau back to terminal";
		}
		else if(help_arg == L"fin"){
			help_str = L"\"fin.\" is part of the kb/query-loading, it denotes the end of your rule-base";
		}else{
			dout << "No command \"" << help_arg << "\"." << endl;
			return;
		}
		dout << "Help -- " << help_str << endl;
	}
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

ParsingResult get_qdb(qdb &kb, string fname){
	std::wifstream is(ws(fname));

	if (!is.is_open()) {
		dout << "failed to open file." << std::endl;
		return FAIL;
	}

	qdb dummy_query;
	int dummy_fins;
	int r = parse(kb, dummy_query, is, fname, dummy_fins);
	dout << "qdb graphs count:"<< kb.first.size() << std::endl;
	
	/*
	int nrules = 0;
	for ( pquad quad :*kb.first[L"@default"])
		nrules++;
	dout << "rules:" << nrules << std::endl;
	*/
	
	return r;
}

bool nodes_same(pnode x, qdb &a, pnode y, qdb &b) {
	setproc(L"nodes_same");
	CLI_TRACE(dout << x->_type << ":" << x->tostring() << ", " <<
					  y->_type << ":" << y->tostring()  << endl);
	if(x->_type == node::BNODE && y->_type == node::BNODE)
	{
		//CLI_TRACE(dout << "BBB" << endl);
		auto la = a.second.find(*x->value);
		auto lb = b.second.find(*y->value);
		if ((la == a.second.end()) != (lb == b.second.end()))
			return false;
		if (la == a.second.end())
		{
			return true;//its a bnode not in lists, bail out for now
		}
		else {
			auto laa = la->second;
			auto lbb = lb->second;
			if (laa.size() != lbb.size())
				return false;
			auto ai = laa.begin();
			auto bi = lbb.begin();
			while(ai != laa.end()) {
				if (!nodes_same(*ai, a, *bi, b))
					return false;
				ai++;
				bi++;
			}
			return true;
		}
	}
	else
		return *x == *y;
}

bool qdbs_equal(qdb &a, qdb &b) {
	dout << "a.first.size  a.second.size  b.first.size  b.second.size" << endl;
	dout << a.first.size() << " " << a.second.size() << " " << b.first.size() << " " << b.second.size() << endl;
	dout << "maybe..";
	dout << "A:" << endl;
	dout << a;
	dout << "B:" << endl;
	dout << b;
	auto ad = *a.first[L"@default"];
	auto bd = *b.first[L"@default"];
	auto i = ad.begin();
	for (pquad x: bd) {
		if (dict[x->pred] == rdffirst || dict[x->pred] == rdfrest)
			continue;
		if (i == ad.end())
			return false;
		pquad n1 = *i;
		pquad n2 = x;
		if (!(*n1->pred == *n2->pred))
			return false;
		if (!nodes_same(n1->subj, a, n2->subj, b))
			return false;
		if (!nodes_same(n1->object, a, n2->object, b))
			return false;
		i++;
	}
	return true;
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
	dout << "test:";
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



qdb old::merge_qdbs(const std::vector<qdb> qdbs)
{
        qdb r;
		if (qdbs.size() == 0)
			return r;
		else if (qdbs.size() == 1)
			return qdbs[0];
		else
			dout << L"warning, kb merging is half-assed";

        for (auto x:qdbs) {
			for (auto graph: x.first) {
				string name = graph.first;
				qlist contents = *graph.second;

				if (r.first.find(name) == r.first.end())
					r.first[name] = make_shared<qlist>(*new qlist());

				for (pquad c: contents) {
					r.first[name]->push_back(c);
				}
			}
			for (auto list: x.second) {
				string name = list.first;
				auto val = list.second;
				r.second[name] = val;
				dout << L"warning, lists may get overwritten";
			}
		}

        return r;
}

#ifndef NOPARSER
int parse_nq(qdb &kb, qdb &query, std::wistream &f, int &fins)
{
	//We can maybe remove this class eventually and just
	//use functions? idk..
	nqparser parser;
	fins = 0;
        try {
                fins = parser.nq_to_qdb(kb, f);
        } catch (std::exception& ex) {
                derr << L"[nq]Error reading quads: " << ex.what() << endl;
                return 0;
        }
        try {
                fins += parser.nq_to_qdb(query, f);
        } catch (std::exception& ex) {
                derr << L"[nq]Error reading quads: " << ex.what() << endl;
                return 2;
        }
        return 2;
}
#endif


int _parse(qdb &kb, qdb &query, std::wistream &f, string fmt, int &fins)
{
	CLI_TRACE(dout << L"parse fmt: " << fmt << endl;)
#ifdef with_marpa
    if(fmt == L"natural3" || fmt == L"n3") {
		//dout << L"Supported is a subset of n3 with our fin notation" << endl;
		return parse_natural3(kb, query, f, fins, base);
	}
    else
#endif
	if(fmt == L"natq" || fmt == L"nq" || fmt == L"nquads")
		return parse_nq(kb, query, f, fins);
	else if(fmt == L"jsonld"){
		dout << L"[jsonld]Cannot yet read JSON-LD format" << endl;
		return FAIL;
	}
    else
		return FAIL;
}

string fmt_from_ext(string fn){
	string fn_lc(fn);
	boost::algorithm::to_lower(fn_lc);

	for (auto x:extensions)
		if (boost::ends_with(fn_lc, x))
			return x;

	return L"";
}

int parse(qdb &kb, qdb &query, std::wistream &f, string fn, int &fins) {
	string fmt = format;
	if (fmt == L"")
		fmt = fmt_from_ext(fn);
	if (fmt != L"")
		return _parse(kb, query, f, fmt, fins);
	else
	{
		int best = 0;
		for (auto x : _formats) {
			int r = _parse(kb, query, f, x, fins);
			if (r > best)
			{
				if (r==COMPLETE)
					return r;
				best = r;
			}
		}
		return best;
	}
	return 0;
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




bool dash_arg(string token, string pattern){
	return (token == pattern) || (token == L"-" + pattern) || (token == L"--" + pattern);
}


string peek(){
	string t;
	for line in input:
        	std::wstringstream ss(line);                                                        
	        while (std::getline(ss, t, L' ')) {//split on spaces                               
                   return t;                                            
                }
	CLITRACE(dout << "empty." << endl;)
}


void get_int(int &i, const string &tok)
{
	try
	{
		i = std::stoi(tok);
	}
	catch
	{
		dout << "bad int, ";
	}
}
		

bool read_option(string s){
	if(s.length() < 2 || s.at(0) != L'-' ||	s == L"-" || s == L"--")
		return false;
	
	if(s.at(1) == L'-'){
		s = s.substr(1, s.length()-1);
	}

	string _option = s;

	for( std::pair<string,bool*> x : _flags){
		if(x.first == _option){
			*x.second = !(*x.second);
			if(x.first == L"nocolor") switch_color();
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

	if (!input.end()) {
		string token = input.pop();
	
		if(_option == L"silence") {
			silence.emplace(token);
			/*dout << "silence:";
			for(auto x: silence)
				dout << x << " ";
			dout << endl;*/
			return true;
		}
	

		if(_option == L"level"){
			get_int(level, token);
			dout << "debug level:" << level << std::endl;
			return true;
		}

		if(_option == L"limit"){
			get_int(result_limit, token);
			dout << "result limit:" << result_limit << std::endl;
			return true;
		}
		
		input.take_back();
	}

	return false;
}



void mode_shouldbe() {
	set_mode(SHOULDBE);
}


void do_query(qdb &q_in)
{
	dout << "query size: " << q_in.first.size() << std::endl;
	tauProver->query(q_in);
}


void cmd_query(){
	if(kbs.size() == 0){
		dout << L"No kb; cannot query." << endl;
	}else{
		if(input.end()){
			set_mode(QUERY);
		}else{
			if(!input.end()){
				string fn = input.pop_long();
				qdb q_in;
				ParsingResult r = get_qdb(q_in,fn);
				if (r != FAIL)
					do_query(q_in);
			}
		}
	}
}



void cmd_kb(){
	if(input.end()){
		clear_kb();
		set_mode(KB);
	}else{
		old::string token = input.pop();
		if(dash_arg(token,L"clear")){
			clear_kb();
			return;
		}
		/*else if(dash_arg(token,L"set")){
			clear_kb();
		}*/
		else if(dash_arg(token,L"add")){
			//dont clear,
			if (input.end())
				throw std::runtime_error("add what?");
			else
				add_kb(input.pop_long());
		}else{
			load_kb(token);
		}	
	}
}


void displayPrompt(){
	if (irc || isatty(fileno(stdin))) {
		//Set the prompt string differently to
		//specify current mode:
		string prompt;
		if (mode == OLD)
			prompt = L"tau";
		if (mode == COMMANDS)
			prompt = L"Tau";
		else if (mode == KB)
			prompt = L"kb";
		else if (mode == QUERY)
			prompt = L"query";
		else if (mode == SHOULDBE)
			prompt = L"shouldbe";
		else
			assert(false);
		std::wcout << prompt;
		if (format != L"")
			std::wcout << L"["<<format<<L"]";
		std::wcout << L"> ";

	}
}

struct input_t
{
	std::string name;
	old::string pop();
	old::string pop_long();
	void take_back();
	void readline()	{};
	bool end();
}	

struct args_input_t:input_t
{
	int argc;
	char **argv;
	unsigned int counter = 1;
	args_input_t(int argc_, char**argv_)
	{
		argc = argc_;
		argv = argv_;
		name = L"args";
	}
	bool end()
	{
		return counter == argc;
	}
	old::string pop()
	{
		assert(!end());
		return argv[counter++];
	}
	old::string pop_long()
	{
		return pop();
	}
	void take_back()
	{
		counter--;
		assert(counter);
	}
}
	


/*		{
			string line;
			std::getline(input_stream, line);
			input += line + L"\n";
		}
					//is.rdbuf()+L"\n";

*/	


stack<struct { input_t*}> inputs;


void emplace_stdin()
{
	inputs.emplace(new stream_input_t("stdin", std::wcin));
}


int main ( int argc, char** argv) {
	//Initialize the prover strings dictionary with hard-coded nodes.
	dict.init();

	if (argc == 1)
		emplace_stdin();
	else
		inputs.emplace(new args_input_t(argv, argc));


	// to hold a kb/query string
	old::string qdb_text;


	while(true){

		input.readline();
		while (input.end())
		{
			if (!inputs.size())
				goto end;
			input = inputs.back();
			inputs.pop();		
			input.readline();
		}
			
			
	
		displayPrompt();
		
		
		if (mode == COMMANDS) {
			old::string token = input.pop();
			if (startsWith(token, L"#") || token == L"")
				continue;
			else if (read_option(token)) 
				continue;
			else if (token == L"help")
				help();
			else if (token == L"kb") 
				cmd_kb();
			else if (token == L"query") 
				cmd_query();
			else if (token == L"shouldbe")
				cmd_shouldbe();
			else if (token == L"thatsall")
				thatsall();
			else if (token == L"shouldbesteps") {
				//test_result(std::stoi(read_arg()) == tauProver->steps_);
			}
			else if (token == L"shouldbeunifys") {
				//test_result(std::stoi(read_arg()) == tauProver->unifys_);
			}
			else if (token == L"quit")
				break;
			else if (token == L"-")
				emplace_stdin();
			else {
				input.take_back();
				old::string line = input.pop_long()

				//maybe its a filename
				string fn = ws(line)
				auto &is = new std::wifstream(fn);
				if (!is.is_open()) {
					dout << "[cli]failed to open \"" << fn << "\"." << std::endl;
				}
				else {
					dout << "[cli]loading \"" << fn << "\"." << std::endl;
					inputs.emplace_back(new stream_input_t(fn, is));
					continue;
				}
				
				//maybe its old-style input
				try to parse the line, if it works,
				set_mode(OLD) and add it to qdb_text
			}
			continue;
		}

			if (mode == KB && fins > 0) {
				kbs.push_back(kb);
				fresh_prover();
				data_buffer=L"";
				set_mode(COMMANDS);
			}
			if (mode == QUERY && fins > 0) {
				(*tauProver).query(kb);//why not -> ?
				//(*tauProver).e.clear();
				data_buffer=L"";
				set_mode(COMMANDS);
			}
			if (mode == SHOULDBE && fins > 0) {
				shouldbe(kb);
				data_buffer=L"";
				set_mode(COMMANDS);
			}
			else
			{
				assert(mode == OLD);
				
				
				 //&& fins == 2) {
				dout << "querying" << std::endl;
				kbs.push_back(kb);
				fresh_prover();
				(*tauProver).query(query);
				//(*tauProver).e.clear();
				data_buffer=L"";
			}
			


		if (std::wcin == input && isatty(fileno(stdin)))
		{
			/*interactive mode, we read in a line and do a reparse
			if it succeeds we add the line to the qdb_buffer,
			otherwise we print error and throw it away*/
		
			string new_buffer = data_buffer + line;
			int fins;
			qdb kb, query;
			std::wstringstream ss(new_buffer);
			int pr = parse(kb, query, ss, L"", fins);
			CLI_TRACE(dout << "parsing result:"<<pr<<std::endl);
			if (pr) 
			{
				qdb_buffer += line;
				CLI_TRACE(dout << "fins:" << fins << std::endl);
			}
		}
		else
		{
			qdb_buffer += line;
			count_fins();
		}
			
		if(pr == COMPLETE)
		{

		}
		else if (pr == FAIL)
		{
			if (line == L"fin.\n")
			{
				data_buffer=L"";
				set_mode(COMMANDS);
			}
			//dout << "[cli]that doesnt parse, try again" << std::endl;
			if (mode == COMMANDS && trimmed_data == L"")
				if(token != L"")
					dout << "[cli]no such command: \"" << token << "\"." << endl;
		}
		
		
		input.forget();
	}
	if (tauProver)
		delete tauProver;
}

