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
string _def_format = L"nq";

std::map<string,bool*> _flags = {
	{L"nocolor",&nocolor},
	{L"deref",&deref},
	//this one.
	{L"shorten",&tau_shorten},
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
#ifdef with_marpa
        if(fmt == L"natural3" || fmt == L"n3")
		dout << "There's actually no such thing as natural3" << endl;
                return parse_natural3(r, f);
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
			_def_format == x;
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
	try {
                std::wistream* pis = &std::wcin;
                if (fname != L"")
                        pis = new std::wifstream(ws(fname));
                std::wistream& is = *pis;
		
		int read_attempt = parse(kb,is,_def_format);
		if(read_attempt != 2){
			string fmt = get_format(fname);
			if(fmt != _def_format){
				return parse(kb,is,fmt);
			}
			return read_attempt;
			
		}
		return 2;
        } catch (std::exception& ex) {
		return 1;
        }
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

	if (std::wcin.rdstate() && std::wcin.eofbit) assert(false); 
	getline(std::wcin, in);
	std::wistringstream i(in);

	string tmp;
	while(std::getline(i,tmp,L' ')){
		_argstream.push_back(tmp);
	}
}

void tau_help(string token){
	dout << L"No command '" << token << L"'." << endl;
}
int main ( int argc, char** argv) {
	dict.init();

	for(int i=1;i<argc;i++){
		_argstream.push_back(ws(argv[i]));
	}

	for(ever){
		if(_argstream.size() == 0){
			tau_shell();
		}else{
			string token = read_arg();
			if(check_option(token)){
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

			tau_help(token);
		}
		
	}
}
