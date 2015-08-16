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
/*
boost::interprocess::managed_heap_memory* segment;
allocator_t* alloc;
*/
std::wostream& dout = std::wcout;
std::wostream& derr = std::wcerr;
std::wistream& din = std::wcin;

std::deque<string> _argstream;
std::vector<string> tauCommands = {L"kb", L"query",L"run",L"quit"};
string _def_format = L"nq";

std::map<string,bool*> tauFlags = {
	{L"nocolor",&nocolor},
	{L"deref",&deref},
	{L"shorten",&tau_shorten},
	{L"base",&fnamebase},
	{L"quads",&quad_in},
	{L"pause",&_pause}
};

std::vector<string> tauFormats = {L"jsonld", L"natural3", L"natq", L"n3", L"nq"};

qdb merge_qdbs(const std::vector<qdb> qdbs)
{
	//std::cout << "merge_qdbs()" << std::endl;
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
prover *tauProver;//i think this wont work; yea im trying to figure out the correct way // create it later, in a func
/*
btw, do you still have the credentials to your vps? i can get them
i would try to set up the CI there, CI?tests automatic ah cool, yea ill get the creds in just a bit yea no hurry; now the reason i have it a global like this is because we add to an existing kb, like "kb add file" doesnt clear the kb and make a new, it just appends filewell, if you want a global, make it a just a pointer.. hmm ok, like how?
*/

#ifndef NOPARSER
int parse_nq(qdb &r, std::wistream &f)
{
        try {
                readqdb(r, f);//this?
        } catch (std::exception& ex) {
                derr << L"Error reading quads: " << ex.what() << endl;
                return 0;
        }
        return 2;
}
#endif


//the improtant bit here is that the called parse functions and this too should return,
//as i wrote, 0 on fail, 1 on incomplete input, 2 on success, and either put something
//into kb and q or not...thats one way, to report failure.

//ok i need to change this up just a bit, thats why i held it off til now//sure
//just remember that n3 parser should handle the fins itself
//sure i was just going to have fin. be part of the parsing anyway (as opposed to parsing it up here)ok
int parse(qdb &r, std::wistream &f, string fmt)
{
#ifdef with_marpa
        if(fmt == L"natural3" || fmt == L"n3")
                return parse_natural3(r, f);
        else
#endif
        if(fmt == L"natq" || fmt == L"nq" || fmt == L"nquads")
                return parse_nq(r, f);

	if(fmt == L"jsonld"){
		dout << L"Cannot read JSON-LD format" << endl;
		return 0;
	}
/*              else
                jsonld*/
 
       else
                throw std::runtime_error("unknown format");
}

string _getFormat(string fn){
        string fn_lc(fn);
        boost::algorithm::to_lower(fn_lc);

	for (auto x:tauFormats)
		if (boost::ends_with(fn_lc, x))
			return x;
        
	return _def_format;

}

bool dashArg(string token, string pattern){
	return (token == pattern) || (token == L"-" + pattern) || (token == L"--" + pattern);
}

string readArg(){
	string ret;
	if(_argstream.size() != 0){
		ret = _argstream.at(0);
		_argstream.pop_front();
	}else{
		dout << L"_argstream is empty." << endl;
	}
	return ret;	
}

bool _isCommand(string s){
	if(s.length() == 0) return false;
	if(s.at(0) == '-') s = s.substr(1,s.length()-1);
	if(s.length() == 0) return false;
	if(s.at(0) == '-') s = s.substr(1,s.length()-1);
	if(s.length() == 0) return false;
	for( string x : tauCommands){
		if(x == s){
			return true;
		}
	}	
	return false;
	
}

bool _checkOption(string s){
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

	for( std::pair<string,bool*> x : tauFlags){
		if(x.first == _option){
			*x.second = !(*x.second);
			return true;
		}
	}
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
	
	for(string x : tauFormats){
		if(x == _option){
			_def_format == x;
			return true;
		}
	}

	if(_option == L"level"){
		if(_argstream.size() != 0){
			string token = readArg();
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

      
int getTauCode(qdb &kb, string fname){
	try {
                std::wistream* pis = &std::wcin;
                if (fname != L"")
                        pis = new std::wifstream(ws(fname));
                std::wistream& is = *pis;
		
		int read_attempt = parse(kb,is,_def_format);
		if(read_attempt != 2){
			string fmt = _getFormat(fname);
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

void _mode_query(){
	if(kbs.size() == 0){
		dout << L"No kb; cannot query." << endl;
	}else{
		if(_argstream.size() == 0){
                	qdb q_in;
                	int r = getTauCode(q_in,L"");
                	if(r == 2){
                        	(*tauProver).query(q_in);
				(*tauProver).e.clear();
                	}else if(r == 1 || r == 0){
                	        dout << L"Error parsing query input." << endl;
                	}else{
                	        dout << L"Return code from getTauCode(): '" << r << L"', is unrecognized. Exiting." << endl;
                        	assert(false);
			}
		}else{
			while(_argstream.size() > 0){
				string token = readArg();
				if(_isCommand(token)){
					_argstream.push_front(token);
					break;
				}
				if(_checkOption(token)){
					continue;
				}
				qdb q_in;
				int r = getTauCode(q_in,token);
				if(r == 2){
                         	       (*tauProver).query(q_in);
				       (*tauProver).e.clear();	
                        	}else if(r == 1 || r == 0){
                        	       dout << L"Error parsing query file \"" << token << "\"" << endl;
                        	}else{
                        	        dout << L"Return code from getTauCode(): '" << r << L"', is unrecognized. Exiting." << endl;
                        	        assert(false);
                        	}
			}
		}
	}
}


void clear_kb(){
	kbs.clear();
}


void _mode_kb(){
	if(_argstream.size() == 0){
		clear_kb();
		qdb kb_in;
		int r = getTauCode(kb_in,L"");
		if(r == 2){
			kbs.push_back(kb_in);
			tauProver = new prover(merge_qdbs(kbs));
		}else if(r == 1 || r == 0){
			dout << L"Error parsing kb input." << endl;
		}else{
			dout << L"Return code from getTauCode(): '" << r << L"', is unrecognized. Exiting." << endl;
			assert(false);
		}
		
	}else{
		string token = readArg();
		if(dashArg(token,L"clear")){
			clear_kb();
			return;
		}
		else if(dashArg(token,L"set")){
			clear_kb();
		}
		else if(dashArg(token,L"add")){
		}else{
			_argstream.push_front(token);
			clear_kb();
		}
	
		if(_argstream.size() == 0){
			qdb kb_in;
			int r = getTauCode(kb_in,L"");
			if(r == 2){
				kbs.push_back(kb_in);
				tauProver = new prover(merge_qdbs(kbs));
			}else if(r == 1 || r==0){
				dout << L"Error parsing kb input." << endl;
			}else{
				dout << L"Return value, '" << r << L"', of getTauCode not recognized. Exiting." << endl;
				assert(false);
			}
		}else{
			while(_argstream.size() > 0){
				string token = readArg();
				if(_isCommand(token)){
					_argstream.push_front(token);
					break;
				}
				if(_checkOption(token)){
					continue;
				}
				
			
				qdb kb_in;
				int r = getTauCode(kb_in, token);	
				if(r == 2){
					kbs.push_back(kb_in);
					tauProver = new prover(merge_qdbs(kbs));
				}else if(r == 1 || r == 0){
					dout<< L"Error parsing kb file: \"" << token << L"\"" << endl;
				}else{
					dout << L"Return value, '" << r << L"', of getTauCode not recognized. Exiting." << endl;
					assert(false);
				}
			}
		}	
	}
}



void tauShell(){
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

void tauHelp(string token){
	dout << L"No command '" << token << L"'." << endl;
}
//  function_name()  int  f(int x, int y)
int main ( int argc, char** argv) {
	dict.init();

	for(int i=1;i<argc;i++){
		_argstream.push_back(ws(argv[i]));
	}

	for(ever){
		if(_argstream.size() == 0){
			tauShell();
		}else{
			string token = readArg();
			if(_checkOption(token)){
				continue;
			}
			if(token == L"kb"){
				_mode_kb();
				continue;
			}

			if(token == L"query"){
				_mode_query();
				continue;
			}
			if(token == L"quit"){
				break;
			}

			tauHelp(token);
		}
		
	}
}
