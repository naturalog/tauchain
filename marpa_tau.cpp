

#include <iostream>
#include <fstream>

#include "marpa_tau.h"
#include "prover.h"
#include "object.h"
#include "cli.h"
#include "rdf.h"
#include "misc.h"

extern "C" {
#include "marpa.h"
#include "marpa_codes.c"
}

#include <boost/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>

typedef std::vector<resid> resids;

resids ask(prover *prover, resid s, const pnode p)
{	/* s and p in, a list of o's out */
	resids r = resids();
	prover::termset query;
	prover::termid o_var = prover->tmpvar();
	prover::termid iii = prover->make(p, prover->make(s), o_var);
	query.emplace_back(iii);

	//dout << "query: "<< prover->format(query) << endl;;

	(*prover)(query);
	
	//dout << "substs: "<< std::endl;

	for (auto x : prover->substs) 
	{
		prover::subst::iterator binding_it = x.find(prover->get(o_var).p);
		if (binding_it != x.end())
		{
			r.push_back( prover->get((*binding_it).second).p);
			//prover->prints(x); dout << std::endl;
		}
	}

	//dout << r.size() << ":" << std::endl;

	prover->substs.clear();
	prover->e.clear();
	return r;
}

resid ask1(prover *prover, resid s, const pnode p)
{
	auto r = ask(prover, s, p);
	if (r.size() > 1)
		throw wruntime_error(L"well, this is weird");
	if (r.size() == 1)
		return r[0];
	else
		return 0;
}

const pnode rdfs_first=mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#first"));
const pnode rdfs_rest= mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#rest"));
const pnode rdfs_nil = mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#nil"));
const pnode pmatches = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#matches"));
const pnode pmustbos = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#mustBeOneSequence"));
const pnode pcslo    = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#commaSeparatedListOf"));
const resid pcomma   = dict[mkliteral(pstr(L","), pstr(L"XSD_STRING"), pstr(L"en"))];


typedef Marpa_Symbol_ID sym;
typedef Marpa_Rule_ID rule;
typedef std::vector<sym> syms;
typedef std::pair<string::const_iterator,string::const_iterator> tokt;


class terminal
{
public:
	string name, regex_string;
	boost::wregex regex;
	terminal(string name_, string regex_)
	{
		name = name_; 
		regex_string = regex_;
		regex = boost::wregex(regex_);
	}
};
typedef std::shared_ptr<terminal> pterminal;

struct Marpa{
	Marpa_Grammar g;
	bool precomputed = false;
	map<sym, pterminal> terminals;
	map<sym, string> literals;
	map<resid, sym> done;
	prover* grmr;

	string sym2str_(sym s)
	{
		if (terminals.find(s) != terminals.end())
			return terminals[s]->name + L"(" + terminals[s]->regex_string + L")";
		if (literals.find(s) != literals.end())
			return L": \"" + literals[s] + L"\"";
		for (auto it = done.begin(); it != done.end(); it++)
			if (it->second == s)
				return *dict[it->first].value;
		return L"(? ? ?dunno? ? ?)";
	}

	string sym2str(sym s)
	{
		std::wstringstream sss;
		sss << L"(" << s << L")" << sym2str_(s);
		return sss.str();
	}


	Marpa()
	{
		if (marpa_check_version (MARPA_MAJOR_VERSION ,MARPA_MINOR_VERSION, MARPA_MICRO_VERSION ) != MARPA_ERR_NONE)
			throw std::runtime_error ("marpa version...");
		Marpa_Config marpa_config;
		marpa_c_init(&marpa_config);
		g = marpa_g_new(&marpa_config);
		if (!g)
		{
			stringstream ssss("marpa_g_new: error ");
			ssss  << marpa_c_error (&marpa_config, NULL);
			throw std::runtime_error (ssss.str());
		}
		if(marpa_g_force_valued(g) < 0)
			throw std::runtime_error ("marpa_g_force_valued failed?!?!?");//bah
	}
	~Marpa()
	{
		marpa_g_unref(g);
	}

	void load_grammar(prover *_grmr, pnode root)
	{
		grmr = _grmr;
		sym start = add(grmr, dict[root]);
		start_symbol_set(start);
	}

	string value(resid val)
	{
		return *dict[val].value;
	}

	//create marpa symbols and rules from grammar description in rdf
	sym add(prover *grmr, resid thing)
	{
		string thingv = value(thing);
		dout << "thingv:" << thingv  << std::endl;

		if (done.find(thing) != done.end())
			return done[thing];	

		if (dict[thing]._type == node::LITERAL)
		{
			//dout << "itsa str"<<std::endl;
			for (auto t: literals)
				if (t.second == thingv)
					return t.first;
					
			sym symbol = symbol_new();
			//dout << "adding " << thingv << std::endl;
			
			literals[symbol] = thingv;
			return symbol;
		}

		//dout << "adding " << thingv << std::endl;
		sym symbol = symbol_new_resid(thing);

		resid bind;
		if((bind = ask1(grmr, thing, pmatches)))
		{
			//dout << "terminal: " << thingv << std::endl;
			terminals[symbol] = pterminal(new terminal(thingv, value(bind)));
			return symbol;
		}

		// mustBeOneSequence is a list of lists
		
		resid ll;
		if ((ll = ask1(grmr, thing, pmustbos)))
		{
			//if (ll == rdfs_nil)
			//	throw wruntime_error(L"mustBeOneSequence empty");

			while(ll)
			{
				resid seq_iterator = ask1(grmr, ll, rdfs_first);;
				if (!seq_iterator) break;
				syms rhs;
				while(seq_iterator)
				{
					resid item = ask1(grmr, seq_iterator, rdfs_first);
					if (!item) break;
					rhs.push_back(add(grmr, item));
					
					seq_iterator = ask1(grmr, seq_iterator, rdfs_rest);
				}
				rule_new(symbol, rhs);
				
				ll = ask1(grmr, ll, rdfs_rest);
			}
		}
		else if ((ll = ask1(grmr, thing, pcslo)))
		{
			seq_new(symbol, add(grmr, ll), add(grmr, pcomma), 0, MARPA_PROPER_SEPARATION);
		}
		else if (thingv == L"http://www.w3.org/2000/10/swap/grammar/bnf#eof")
			{}//so what?
		else
			throw wruntime_error(L"whats " + thingv + L"?");

		dout << "added sym "<< symbol << std::endl;
		return symbol;
	}


int check_int(int result){
	if (result == -2)
		error();
	return result;
}

void check_null(void* result){
	if (result == NULL)
		error();
}

void error_clear(){
	marpa_g_error_clear(g);
}

sym symbol_new()
{
	return check_int(marpa_g_symbol_new(g));

}

sym symbol_new_resid(resid thing)
{
	return done[thing] = symbol_new();

}

rule rule_new(sym lhs, syms rhs)
{
	dout << sym2str(lhs) << L" ::= "; 
	for (sym s: rhs)
		dout << sym2str(s);
	dout << std::endl;
	return check_int(marpa_g_rule_new(g, lhs, &rhs[0], rhs.size()));
	/*if (r == -2)
	{
		int e = marpa_g_error(g, NULL);
		if (e == MARPA_ERR_DUPLICATE_RULE)
		{
	*/
}

rule seq_new(sym lhs, sym rhs, sym separator, int min, int flags)
{
	int r = marpa_g_sequence_new(g, lhs, rhs, separator, min, flags);
	if (r == -2)
	{
		int e = marpa_g_error(g, NULL);
		if (e == MARPA_ERR_DUPLICATE_RULE)
			dout << sym2str(lhs) << L" ::= sequence of " << sym2str(rhs) << std::endl;
	}
	return check_int(r);
}


void print_events()
{	
	
	int count = check_int(marpa_g_event_count(g));
	dout << count << "events" << std::endl;
	if (count > 0)
	{
		Marpa_Event e;
		for (int i = 0; i < count; i++)
		{
			int etype = check_int(marpa_g_event(g, &e, i));
			dout << etype << ", " << e.t_value << std::endl;
		}
	}
	
}

void start_symbol_set(sym s){
	check_int( marpa_g_start_symbol_set(g, s) );
}

void error()
{
	
	int e = marpa_g_error(g, NULL);
	stringstream s;
	s << e << ":" << marpa_error_description[e].name << " - " << marpa_error_description[e].suggested ;
	throw(std::runtime_error(s.str()));// + ": " + errors[e]));
	
}


void parse (const string inp)
{
	if (!precomputed)
		check_int(marpa_g_precompute(g));

	print_events();

	dout << "terminals:\n";
	for (auto t:terminals)
		dout << "(" << t.first << ")" << t.second->name << ": " << t.second->regex << std::endl;

	dout << "tokenizing..\n";
	
	std::vector<tokt> toks; // ranges of individual tokens within the input string
	toks.push_back(tokt(inp.end(), inp.end()));

	Marpa_Recognizer r = marpa_r_new(g);
	check_null(r);
	marpa_r_start_input(r);

	auto pos = inp.begin();

	std::vector<sym> expected;
	expected.resize(check_int(marpa_g_highest_symbol_id(g)));

	boost::wregex comment (L"#[^$]*");

	while (pos < inp.end())
	{
		string wss = L"\n\r \t";
		for (auto ws: wss)
			if ((*pos) == ws)
			{
				pos++;
				continue;
			}
		
		boost::wsmatch what;
		if (regex_search(pos, inp.end(), what, comment, boost::match_continuous))
		{
			if(!what.empty())
			{
				int llll = what[0].length();
				dout << L"skipping " << llll << L" comment chars" << std::endl;
				pos += llll;
				continue;
			}
		}			
		
		std::vector<sym>best_syms;
		size_t best_len = 0;
		expected.clear();
		int num_expected = check_int(marpa_r_terminals_expected(r, &expected[0]));
		for (int i = 0; i < num_expected; i++) 
		{
			sym e = expected[i];
			if (literals.find(e) != literals.end())
			{
				if (boost::starts_with(string(pos, inp.end()), literals[e]))
				{
					if (literals[e].size() > best_len)
					{
						best_syms.clear();
						best_syms.push_back(e);
						best_len = literals[e].size();
					}
					else if (literals[e].size() == best_len)
						best_syms.push_back(e);
				}
			}
			else
			{
				for (auto t: terminals)
				{
					if (!regex_search(pos, inp.end(), what, t.second->regex, boost::match_continuous)) continue;
					if (what.empty()) continue;
					size_t l = what.length();
					if (l > best_len)
					{
						best_len = l;
						best_syms.clear();
						best_syms.push_back(e);
					}
					else if (l == best_len)
						best_syms.push_back(e);
				}
			}
		}

		if (best_len)
		{
			if(best_syms.size() > 1)
			{
				dout << L"cant decide between:" << std::endl;
				for (auto ccc: best_syms)
					dout << L" " << sym2str(ccc) << std::endl;
			}
			assert (best_syms.size());
			toks.push_back(tokt(pos, pos + best_len));
			dout << std::distance(inp.begin(), pos) << L"-" << std::distance(inp.begin(), pos + best_len) << 
				L"\"" <<  string(pos, pos + best_len) << L"\" - " << sym2str(best_syms[0]) << std::endl;
			check_int(marpa_r_alternative(r, best_syms[0], toks.size(), 1));
			check_int(marpa_r_earleme_complete(r));
			pos += best_len;
		}
		else
		{
			dout << "expecting:" << std::endl;
			for (int i = 0; i < num_expected; i++) 
			{
				sym e = expected[i];
				dout << sym2str(e) << std::endl;
                        }
			throw(std::runtime_error("no parse"));
		}
	}

    	dout << "evaluating.." << std::endl;

	//marpa allows variously ordered lists of ambiguous parses, we just grab the default
	Marpa_Bocage b = marpa_b_new(r, -1);
	check_null(b);
	Marpa_Order o = marpa_o_new(b);
	check_null(o);
	Marpa_Tree t = marpa_t_new(o);
	check_null(t);
	check_int(marpa_t_next(t));
	Marpa_Value v = marpa_v_new(t);
	check_null(v);


	//"valuator" loop. marpa tells us what rules with what child nodes or tokens were matched, 
	//we bottom-up build the "value" that is the ast

	//map<int, prover::termid> stack; // intermediate values
	map<int, string> stack; // intermediate values

	while(1)
	{
		Marpa_Step_Type st = check_int(marpa_v_step_type(v));
		if (st == MARPA_STEP_INACTIVE) break;
		switch(st)
		{
			case MARPA_STEP_TOKEN:
			{
				sym symbol = marpa_v_symbol(v);
				size_t token = marpa_v_token_value(v) - 1;
				stack[marpa_v_result(v)] = string(toks[token].first, toks[token].second);
				break;
			}
			case MARPA_STEP_RULE:
			{
				rule r = marpa_v_rule(v);
				std::vector<prover::termid> args;
				stack[marpa_v_arg_0(v)] = L"( ";
				for (int i = marpa_v_arg_0(v); i <= marpa_v_arg_n(v); i++)
					//args.push_back(stack[i]);
					stack[marpa_v_arg_0(v)] += stack[i] + L" ";
				stack[marpa_v_arg_0(v)] += L") ";				
				break;
			}
			//case MARPA_STEP_NULLING_SYMBOL:
                                //stack2[v.v.t_result] = "nulled"
                                
		}
	}
	marpa_v_unref(v);
	marpa_t_unref(t);
	marpa_o_unref(o);
	marpa_b_unref(b);
	
	//bind(?X, stack[0])
	dout << stack[0] << std::endl;
	
}




};


std::string load_n3_cmd::desc() const {return "load n3";}
std::string load_n3_cmd::help() const 
{
	stringstream ss("Hilfe! Hilfe!:");
	return ss.str();
}


string load(string fname)
{
	std::ifstream t(ws(fname));
 	if (!t.is_open())
		throw std::runtime_error("couldnt open file");// \"" + fname + L"\"");

	std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	return ws(str);
}

int load_n3_cmd::operator() ( const strings& args )
{	
	Marpa m;
//	prover prover(convert(load_json(L"n3-grammar.jsonld")));
	prover prover(*load_quads(L"n3-grammar.nq"));
	m.load_grammar(
		&prover, 
//		mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/n3#language")));
		mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/n3#document")));
 	string input = load(args[2]);
	m.parse(input);
	return 0;
}




/*
---------------------


?M :marpa_for_grammar http://www.w3.org/2000/10/swap/grammar/n3#language; ?X :parsed (?M ?FN).


->


?X a n3:document
?X n3:declarations [ ("@base" "xxx")  ("@prefix" "xxx" "yyy")  ("@keywords" ("aaa" "bbb" "ccc")) ]
?X n3:universals

?X n3:statements [ 



*/


/*
resources
http://www.w3.org/2000/10/swap/grammar/bnf
http://www.w3.org/2000/10/swap/grammar/n3.n3

https://github.com/jeffreykegler/libmarpa/blob/master/test/simple/rule1.c#L18
https://github.com/jeffreykegler/Marpa--R2/issues/134#issuecomment-41091900
https://github.com/pstuifzand/marpa-cpp-rules/blob/master/marpa-cpp/marpa.hpp
*/


