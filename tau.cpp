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

string format = L"";

std::map<string,bool*> _flags = {
	{L"nocolor",&nocolor},
	{L"deref",&deref},
	//this one.
	{L"shorten",&shorten},
	{L"base",&fnamebase},
	{L"quads",&quad_in},
	{L"pause",&_pause}
};

std::vector<string> extensions = {L"jsonld", L"natural3", L"natq", L"n3", L"nq"};
std::vector<string> _formats = {L"nq", L"n3", L"jsonld"};

std::vector<qdb> kbs;
prover *tauProver;


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


int parse(qdb &kb, qdb &query, std::wistream &f, string fmt)
{
	dout << L"parse " << fmt << endl;
#ifdef with_marpa
    if(fmt == L"natural3" || fmt == L"n3") {
		dout << L"Supported is a subset of n3 with our fin notation" << endl;
		return parse_natural3(kb, query, f);
	}
    else
#endif
	if(fmt == L"natq" || fmt == L"nq" || fmt == L"nquads")
                return parse_nq(kb, query, f);

	else if(fmt == L"jsonld"){
		dout << L"Cannot yet read JSON-LD format" << endl;
		return 0;
	}
    else
		return 0;
}

string fmt_from_ext(string fn){
	string fn_lc(fn);
	boost::algorithm::to_lower(fn_lc);

	for (auto x:extensions)
		if (boost::ends_with(fn_lc, x))
			return x;

	return L"";
}

int try_parse(qdb &kb, qdb &query, std::wistream &f, string fn) {
	string fmt;
	if (format == L"")
		fmt = fmt_from_ext(fn);
		return parse(kb, query, f, format);
	else
	{
		int best = 0;
		for (auto x : _formats) {
			int r = parse(kb, query, f, x);
			if (r > best)
				best = r;
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



int main ( int argc, char** argv) {
	dict.init();
	string input_buffer, data_buffer;
	int mode;
	const int DATA = 1;
	const int COMMANDS = 0;

	for(int i=1;i<argc;i++){
		input_buffer += ws(argv[i]) + " ";
	}

	for(ever){
		if (isatty(fileno(stdin))
			std::wcout << L"Tau> ";
		if (input_buffer.size() == 0);
		{
			if (std::wcin.eof())
				exit(0);
			else
				input_buffer << std::cin << L"\n"/*?*/;
		}

		size_t end = input_buffer.find("\n");
		string line = string(input_buffer.begin(), input_buffer.begin() + end);
		input_buffer = string(input_buffer.begin() + end, input_buffer.end());
		dout << L"line is \"" << line << "\"" << std::endl;


		string token;
		if (mode == COMMANDS) {

			string _t;
			while (std::getline(line, _t, L' ')) {//split on spaces
					_argstream.push_back(_t);
				}

			token = read_arg();
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

			if (token == L"quit") {
				break;
			}
		}

		string new_buffer = data_buffer + line;
		int fins;
		int pr = try_parse(new_buffer, fins);
		if(pr == COMPLETE)
		{
			if (mode == KB || mode == QUERY && fins == 1) {
				//add kb or execute query
			}
			else if(mode == COMMANDS && fins == 2) {
				//do both
			}
		}
		else if (pr == INCOMPLETE) {
			data_buffer += line;
		}
		else if (pr == FAIL && mode == COMMANDS)
			dout << "No such command: \"" << token << "\"." << endl;
		}
	}
}

