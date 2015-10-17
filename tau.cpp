#include <stdio.h>
#include <sstream>
#include <tuple>
#include <iostream>
#include <queue>
#include "univar.h"
#include <set>

#include "cli.h"
#include <stdexcept>
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

#define ever ;;

std::wostream& dout = std::wcout;
std::wostream& derr = std::wcerr;
std::wistream& din = std::wcin;

std::deque<string> _argstream;

int mode = 0;
const int COMMANDS = 0;
const int KB = 1;
const int QUERY = 2;
const int SHOULDBE = 3;

const int FAIL = 0;
//const int INCOMPLETE = 1;
const int COMPLETE = 2;

string format = L"";
string base = L"";

bool irc = false;
std::set<string>& silence = *new std::set<string>;

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
		else if(help_arg == L"quit") {
			help_str = L"command 'quit': exit Tau back to terminal";
		}
		else if(help_arg == L"fin"){
			help_str = L"fin is part of the kb/query-loading, it denotes the end of your rule-base";
		}else{
			dout << "No command \"" << help_arg << "\"." << endl;
			return;
		}
		dout << "Help -- " << help_str << endl;


	}
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
		fins = 0;
        try {
                fins = readqdb(kb, f);
        } catch (std::exception& ex) {
                derr << L"[nq]Error reading quads: " << ex.what() << endl;
                return 0;
        }
        try {
                fins += readqdb(query, f);
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

bool dash_arg(string token, string pattern){
	return (token == pattern) || (token == L"-" + pattern) || (token == L"--" + pattern);
}

string read_arg(){
	string ret;
	if(_argstream.size() != 0){
		ret = _argstream.at(0);
		boost::algorithm::trim(ret);
		_argstream.pop_front();
	}else{
		dout << L"_argstream is empty." << endl;
	}
	return ret;
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
			format = x;
			dout << "format:"<<format<<std::endl;
			return true;
		}
	}

	if(_option == L"silence") {
		if (_argstream.size() != 0) {
			string token = read_arg();
			silence.emplace(token);
			dout << "silence:";
			for(auto x: silence)
				dout << x << " ";
			dout << endl;
		}
		return true;
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
			/*else if(tmpLevel > 100) 
				level = 100;why?*/
			else
				level = tmpLevel;
			dout << "level:" << level << std::endl;
		}
		return true;
	}
	return false;
}

int get_qdb(qdb &kb, string fname){
	std::wifstream is(ws(fname));
	if (!is.is_open()) {
		dout << "failed to open file." << std::endl;
		return 0;
	}
	qdb dummy_query;
	int dummy_fins;
	int r = parse(kb, dummy_query, is, fname, dummy_fins);
	dout << "qdb graphs count:"<< kb.first.size() << std::endl;
	int nrules = 0;
	for ( pquad quad :*kb.first[L"@default"])
		nrules++;
	dout << "rules:" << nrules << std::endl;
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


void mode_shouldbe() {
	set_mode(SHOULDBE);
}

void thatsall()
{
	test_result(tauProver->results.empty());
}

void mode_query(){
	if(kbs.size() == 0){
		dout << L"No kb; cannot query." << endl;
	}else{
		if(_argstream.size() == 0){
			set_mode(QUERY);
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
					dout << "query size: " << q_in.first.size() << std::endl;
					(*tauProver).query(q_in);
					//(*tauProver).e.clear();
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
		set_mode(KB);
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
			//clear_kb();
		}
	
		if(_argstream.size() == 0){
			qdb kb_in;
			int r = get_qdb(kb_in,L"");
			if(r == 2){
				kbs.push_back(kb_in);
				fresh_prover();
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
					fresh_prover();
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



int main ( int argc, char** argv) {
	dict.init();
	string input_buffer, data_buffer;

	for(int i=1;i<argc;i++){
		input_buffer += ws(argv[i]) + L" ";
	}

	for(ever){
		CLI_TRACE(dout << "argstream.size=" << _argstream.size() << std::endl;)

		if (irc || isatty(fileno(stdin))) {
			string prompt;
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

		if (input_buffer.size() == 0 && _argstream.size() == 0)
		{
			if (std::wcin.eof())
				break;
			string line;
			std::getline(std::wcin, line);
			input_buffer += line + L"\n";
		}

		size_t nlpos = input_buffer.find(L"\n");
		size_t end;
		if (nlpos == input_buffer.npos)
			end = input_buffer.size();
		else
			end = nlpos + 1;
		CLI_TRACE(dout << L"nlpos=" << nlpos << " end=" << end << std::endl;)
		string line = string(input_buffer.begin(), input_buffer.begin() + end);
		input_buffer = string(input_buffer.begin() + end, input_buffer.end());
		CLI_TRACE(
		dout << L"line is \"" << line << "\"" << std::endl;
		dout << L"input buffer: \"" << input_buffer << "\"" << std::endl;
		dout << L"data buffer: \"" << data_buffer << "\"" << std::endl;
		dout << L"mode: \"" << mode << "\"" << std::endl;
		)
		string trimmed_data = data_buffer;
		boost::algorithm::trim(trimmed_data);

		dout << L"<\"" << line << "\"" << std::endl;

		string token;
		if (mode == COMMANDS && trimmed_data == L"") {

			string _t;
			std::wstringstream liness(line);
			while (std::getline(liness, _t, L' ')) {//split on spaces
					_argstream.push_back(_t);
				}

			CLI_TRACE(
			dout << "argstream:[";
			for (auto a:_argstream)
				dout << a << ", ";
			dout << "]" << std::endl;
			)

			token = read_arg();

			CLI_TRACE(dout << L"token: \"" << token << "\"" << std::endl);

			if (check_option(token)) {
				continue;
			}

			if (token == L"help") {
				string help_arg = L"";
				if (_argstream.size() > 0) {
					help_arg = read_arg();
				}
				help(help_arg);
				continue;
			}

			if (token == L"kb") {
				mode_kb();
				continue;
			}

			if (token == L"query") {
				mode_query();
				continue;
			}
			if (token == L"shouldbe") {
				mode_shouldbe();
				continue;
			}
			if (token == L"thatsall") {
				thatsall();
				continue;
			}
			if (token == L"quit") {
				break;
			}
			if(token != L"") {
				std::wifstream is(ws(token));
				if (!is.is_open()) {
					dout << "failed to open \"" << token << "\"." << std::endl;
				}
				else {
					std::wstringstream ss;
					ss << is.rdbuf();
					input_buffer += ss.str() + L"\n";
				}
			}
		}

		string new_buffer = data_buffer + line;

		int fins;
		qdb kb, query;
		std::wstringstream ss(new_buffer);
		int pr = parse(kb, query, ss, L"", fins);
		CLI_TRACE(dout << "parsing result:"<<pr<<std::endl);
		if (pr) {
			data_buffer += line;
			CLI_TRACE(dout << "fins:" << fins << std::endl);
		}
		if(pr == COMPLETE)
		{
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
			else if(mode == COMMANDS && fins == 2) {
				dout << "querying" << std::endl;
				kbs.push_back(kb);
				fresh_prover();
				(*tauProver).query(query);
				//(*tauProver).e.clear();
				data_buffer=L"";
			}
		}
		else if (pr == FAIL)
		{
			if (line == L"fin.\n")
			{
				data_buffer=L"";
				set_mode(COMMANDS);
			}
		
			dout << "[cli]that doesnt parse, try again" << std::endl;
			if (mode == COMMANDS && trimmed_data == L"")
				dout << "[cli]and theres no such command: \"" << token << "\"." << endl;
		}
		_argstream.clear();
	}
	if (tauProver)
		delete tauProver;
}

