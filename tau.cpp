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

std::ostream& dout = std::cout;
std::ostream& derr = std::cerr;
std::istream& din = std::cin;

// to hold a kb/query string
string qdb_text;

enum Mode {COMMANDS, KB, QUERY, SHOULDBE, OLD};
Mode mode = COMMANDS;

enum ParsingResult {FAIL, /* INCOMPLETE, */ COMPLETE};

string format = "";
string base = "";

int result_limit = 123;
bool irc = false;
std::set<string> silence;
bool in_silent_part = false;

std::map<string,bool*> _flags = {
		{"nocolor",&nocolor}
		,{"deref",&deref}
		,{"irc",&irc}
		,{"shorten",&shorten}
		,{"base",&fnamebase}
};

std::vector<string> extensions = {"jsonld", "natural3", "natq", "n3", "nq"};
std::vector<string> _formats = {
								#ifndef NOPARSER
								"nq",
								#endif
								#ifdef with_marpa
								"n3",
								#endif
								#ifdef JSON
								"jsonld"
								#endif
};
std::vector<string> _commands = {"kb", "query","run","quit"};

yprover *tauProver = 0;


std::vector<qdb> kbs;

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
		case OLD:
			dout << "old";
			break;
	}
	dout << endl;
	mode = m;
}

void help(string help_arg){
	if(input.end()){
		dout << "Help -- commands: kb, query, help, quit; use \"help <topic>\" for more detail." << endl;
		dout << "command 'kb': load a knowledge-base." << endl;
		dout << "command 'query': load a query and run." << endl;
		dout << "command 'help': Tau will help you solve all your problems." << endl;
		dout << "command 'quit': exit Tau back to terminal" << endl;
		dout << "\"fin.\" is part of the kb/query-loading, it denotes the end of your rule-base" << endl;
	}
	else{
		string help_arg = input.pop();
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
	for ( pquad quad :*kb.first["@default"])
		nrules++;
	dout << "rules:" << nrules << std::endl;
	*/
	
	return r;
}

bool nodes_same(pnode x, qdb &a, pnode y, qdb &b) {
	setproc("nodes_same");
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
	auto ad = *a.first["@default"];
	auto bd = *b.first["@default"];
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
		dout << KRED << "FAI" << KNRM << endl;
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



qdb merge_qdbs(const std::vector<qdb> qdbs)
{
        qdb r;
		if (qdbs.size() == 0)
			return r;
		else if (qdbs.size() == 1)
			return qdbs[0];
		else
			dout << "warning, kb merging is half-assed";

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
				dout << "warning, lists may get overwritten";
			}
		}

        return r;
}

#ifndef NOPARSER
int parse_nq(qdb &kb, qdb &query, std::istream &f, int &fins)
{
	//We can maybe remove this class eventually and just
	//use functions? idk..
	nqparser parser;
	fins = 0;
        try {
                fins = parser.nq_to_qdb(kb, f);
        } catch (std::exception& ex) {
                derr << "[nq]Error reading quads: " << ex.what() << endl;
                return 0;
        }
        try {
                fins += parser.nq_to_qdb(query, f);
        } catch (std::exception& ex) {
                derr << "[nq]Error reading quads: " << ex.what() << endl;
                return 2;
        }
        return 2;
}
#endif


int _parse(qdb &kb, qdb &query, std::istream &f, string fmt, int &fins)
{
	CLI_TRACE(dout << "parse fmt: " << fmt << endl;)
#ifdef with_marpa
    if(fmt == "natural3" || fmt == "n3") {
		//dout << "Supported is a subset of n3 with our fin notation" << endl;
		return parse_natural3(kb, query, f, fins, base);
	}
#endif
#ifndef NOPARSER
	if(fmt == "natq" || fmt == "nq" || fmt == "nquads")
		return parse_nq(kb, query, f, fins);
#endif
#ifdef JSON
	if(fmt == "jsonld"){
		dout << "[jsonld]somobody wire the json-ld parser into the cli" << endl;
		return FAIL;
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

int parse(qdb &kb, qdb &query, std::istream &f, string fn, int &fins) {
	string fmt = format;
	if (fmt == "")
		fmt = fmt_from_ext(fn);
	if (fmt != "")
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

int count_fins()
{
	int fins = 0;
	string line;
	stringstream ss(qdb_text);
	while (!ss.eof()) {
		getline(ss, line);
		if (startsWith(s, "fin") && *wstrim(s.c_str() + 3) == ".")
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
	catch
	{
		dout << "bad int, ";
	}
}

int count_fins()
{
	int fins = 0;
	std::stringstream ss(qdb_text);
	do {
		string l;
		getline(ss, l);
		if(ss.end())break;
		trim(l);
		if (l == "fin.") fins++;
	}
	return fins;
}

struct input_t
{
	bool interactive = true;
	bool do_reparse = true;
	std::string name;
	string pop();
	string pop_long();
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
		name = "args";
	}
	bool end()
	{
		return counter == argc;
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
}



struct stream_input_t:input_t
{
	std::istream stream;
	string line;
	size_t pos;
	std::stack<size_t> starts;

	void figure_out_interactivity()
	{
		//if its a file
		auto s = dynamic_cast<std::wifstream>(stream);
		interactive = !s;
		//else its stdin
		if (!s) {
			assert(stream == std::wcin);
			//but its not attached to a tty
			if (!isatty(fileno(stdin)))
				interactive = false;
		}
	}

	stream_input_t(string fn_, std::wifstream is_)
	{
		name = fn_;
		stream = is_;
		figure_out_interactivity();
	}
	bool end()
	{
		return stream.eof();
	}

	void readline()
	{
		std::getline(stream, line);
		starts.clear();
		pos = 0;
		do_reparse = interactive && stream.peek() == EOF;
	}
	string pop_x(wchar_t x)
	{
		size_t start = pos;
		while(line[pos] == ' ') pos++;
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
		pos = starts.back();
		starts.pop();
	}
}


/*		{
			string line;

			input += line + "\n";
		}
					//is.rdbuf()+"\n";
*/


stack<struct { input_t*}> inputs;
input_t* input;



bool read_option(string s){
	if(s.length() < 2 || s.at(0) != L'-' ||	s == "-" || s == "--")
		return false;
	
	if(s.at(1) == L'-'){
		s = s.substr(1, s.length()-1);
	}

	string _option = s;

	for( std::pair<string,bool*> x : _flags){
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

	if (!input.end()) {
		string token = input.pop();
	
		if(_option == "silence") {
			silence.emplace(token);
			/*dout << "silence:";
			for(auto x: silence)
				dout << x << " ";
			dout << endl;*/
			return true;
		}
	

		if(_option == "level"){
			get_int(level, token);
			dout << "debug level:" << level << std::endl;
			return true;
		}

		if(_option == "limit"){
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
		dout << "No kb; cannot query." << endl;
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
		string token = input.pop();
		if(dash_arg(token,"clear")){
			clear_kb();
			return;
		}
		/*else if(dash_arg(token,"set")){
			clear_kb();
		}*/
		else if(dash_arg(token,"add")){
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
			prompt = "tau";
		if (mode == COMMANDS)
			prompt = "Tau";
		else if (mode == KB)
			prompt = "kb";
		else if (mode == QUERY)
			prompt = "query";
		else if (mode == SHOULDBE)
			prompt = "shouldbe";
		else
			assert(false);
		std::wcout << prompt;
		if (format != "")
			std::wcout << "["<<format<<"]";
		std::wcout << "> ";

	}
}




void try_to_parse_the_line__if_it_works__add_it_to_qdb_text()
{
	string x = qdb_text + input.pop_long() + "\n";

	if (!input.do_reparse) {
		qdb_text = x;
	}
	else {
		qdb kb, query;
		std::stringstream ss(x);
		int pr = parse(kb, query, ss, "");
		CLI_TRACE(dout << "parsing result:" << pr << std::endl);
		if (pr) {
			qdb_text = x;
		}
		else
			dout << "[cli]that doesnt parse, try again" << std::endl;
	}
}

/*
	if (pr == COMPLETE) {
		}
		else if (pr == FAIL) {
			if (line == "fin.\n") {
				data_buffer = "";
				set_mode(COMMANDS);
			}
			if (mode == COMMANDS && trimmed_data == "") if (token != "")
				dout << "[cli]no such command: \"" << token << "\"." << endl;
		}
	}
}
*/

void emplace_stdin()
{
	inputs.emplace(new stream_input_t("stdin", std::wcin));
}


int main ( int argc, char** argv)
{
	//Initialize the prover strings dictionary with hard-coded nodes.
	dict.init();

	if (argc == 1)
		emplace_stdin();
	else
		inputs.emplace(new args_input_t(argv, argc));


	while (true) {

		input.readline();
		while (input.end()) {
			if (!inputs.size())
				goto end;
			input = inputs.back();
			inputs.pop();
			input.readline();
		}


		displayPrompt();


		if (mode == COMMANDS) {
			string token = input.pop();
			if (startsWith(token, "#") || token == "")
				continue;
			else if (read_option(token))
				continue;
			else if (token == "help")
				help();
			else if (token == "kb")
				cmd_kb();
			else if (token == "query")
				cmd_query();
			else if (token == "shouldbe")
				cmd_shouldbe();
			else if (token == "thatsall")
				thatsall();
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
				input.take_back();
				string line = input.pop_long()

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
				if (try_to_parse_the_line__if_it_works__add_it_to_qdb_text())
					set_mode(OLD);
				else
					dout << "[cli]that doesnt parse, try again" << std::endl;
				continue;
			}
		}
		else if (mode == KB || mode == QUERY || mode == SHOULDBE) {
			try_to_parse_the_line__if_it_works__add_it_to_qdb_text();
			int fins = count_fins();
			if (fins > 0) {

				qdb kb,kb2;
				int dummy_fins;
				std::stringstream ss(qdb_text);
				auto pr = parse(kb, kb2, ss, input.name, dummy_fins);

				if(pr == COMPLETE) {
					if (mode == KB) {
						kbs.push_back(kb);
						fresh_prover();
					}
					else if (mode == QUERY) {
						tauProver->query(kb);
					}
					else if (mode == SHOULDBE) {
						shouldbe(kb);
					}
				}
				else
					dout << "error" << endl;
				qdb_text = "";
				set_mode(COMMANDS);
			}
		}
		else {
			assert(mode == OLD);
			try_to_parse_the_line__if_it_works__add_it_to_qdb_text();
			int fins = count_fins();
			if (fins > 1) {
				dout << "querying" << std::endl;
				kbs.push_back(kb);
				fresh_prover();
				tauProver->query(query);
			}
		}
	}
	end:
	if (tauProver)
		delete tauProver;
}

