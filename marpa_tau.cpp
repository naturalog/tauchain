

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

#include "lexertl/generator.hpp"
#include "lexertl/iterator.hpp"
#include "lexertl/lookup.hpp"


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


class terminal
{
public:
	string name, regex;
	sym symbol;
	terminal(string name_, string regex_, sym symbol_){name = name_; regex = regex_; symbol = symbol_;}
};




struct Marpa{
	Marpa_Grammar g;
	std::vector<terminal> terminals;
	map<resid, sym> done; // rules
	bool precomputed = false;
	prover* grmr;

	string sym2str_(sym s)
	{
		for (auto it = done.begin(); it != done.end(); it++)
			if (it->second == s)
				return *dict[it->first].value;
		for (auto it = terminals.begin(); it != terminals.end(); it++)
			if (it->symbol == s)
				return it->name;
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
		dout << "starting with " << *root->value << " " <<  dict[root] << std::endl;
		sym start = add(grmr, dict[root]);
//		sym start = add(grmr, grmr->get(grmr->make(root)).p);
		start_symbol_set(start);
		terminals.push_back(terminal(L"comment", L"#.*", -2));
	}

	string value(resid val)
	{
		return *dict[val].value;
	}

	string lexertl_literal(string s){return L"\"" + s + L"\"";}//todo escape escapes. the goal is that lexertl should match s literally, not as a regex


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
			thingv = lexertl_literal(thingv);
			for (auto t :terminals)
				if (t.regex == thingv)
					return t.symbol;		
			sym symbol = symbol_new();
			//dout << "adding " << thingv << std::endl;
			terminals.push_back(terminal(thingv, thingv, symbol));
			return symbol;
		}

		//dout << "adding " << thingv << ",termid:" << thing << std::endl;
		sym symbol = symbol_new_resid(thing);

		resid bind;
		if((bind = ask1(grmr, thing, pmatches)))
		{
			//dout << "terminal: " << thingv << std::endl;
			terminals.push_back(terminal(thingv, value(bind), symbol));
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
	{
		check_int(marpa_g_precompute(g));
		print_events();
	}

	lexertl::wrules rules;
	lexertl::wstate_machine sm;

	sort( terminals.begin( ), terminals.end( ), [ ]( const terminal& a, const terminal& b )
	{
		return a.regex.size() > b.regex.size();
	});
	
	dout << "terminals:\n";
	for (auto t:terminals)
	{
		dout << "(" << t.symbol << ")" << t.name << ": " << t.regex << std::endl;
		rules.push(t.regex, t.symbol);
	}

	dout << "building tokenizer..\n";
	lexertl::wgenerator::build(rules, sm);
	lexertl::wsiterator iter(inp.begin(), inp.end(), sm);
	lexertl::wsiterator end;
	dout << "tokenizing..\n";
	
	std::vector<string> toks; // token strings
	toks.push_back(L"dummy coz of marpa unvalued shit");

	Marpa_Recognizer r = marpa_r_new(g);
	check_null(r);
	marpa_r_start_input(r);

	for (; iter != end; ++iter)
	{
		if(iter->id == 18446744073709551615)
			continue;

		//Like flex, end of input returns an id of 0,  (you can set this to another value if you like using the eoi() method on the rules class) 
		
		sym s = iter->id;

		print_terminal(iter->id);
		dout << ", Text: '" << iter->str() << "'\n";
	
		//#Return value: On success, MARPA_ERR_NONE. On failure, some other error code.
		int err = marpa_r_alternative(r, s, toks.size(), 1);

		if (err == MARPA_ERR_UNEXPECTED_TOKEN_ID)
		{
			std::vector<sym> expected;
			expected.resize(check_int(marpa_g_highest_symbol_id(g)));
			int num_expected = check_int(marpa_r_terminals_expected(r, &expected[0]));
			dout << "expecting:" << std::endl;
			for (int i = 0; i < num_expected; i++) 
			{
				dout << " ";
				print_terminal(expected[i]);
				dout << std::endl;
			}
		}
		else if (err != MARPA_ERR_NONE)
			dout << err << ":" << marpa_error_description[err].name << " - " << marpa_error_description[err].suggested ;

		check_int(marpa_r_earleme_complete(r));
		toks.push_back(iter->str());
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
				stack[marpa_v_result(v)] = toks[token];
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

void print_terminal(sym s)
{
	for (auto t:terminals)
		if (t.symbol == s)
		{
			dout << "(" << t.symbol << ")" << t.name;
			if (t.name != t.regex)
				dout << ": " << t.regex;
		}
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


