#include "marpa.h"
#include "prover.h"
//#include "marpa.h"//not yet
#include "object.h"
#include "cli.h"
#include "rdf.h"
#include "misc.h"


#include "lexertl/generator.hpp"
#include <iostream>
#include "lexertl/iterator.hpp"
#include "lexertl/lookup.hpp"


prover::termset ask(prover *prover, prover::termid s, const pnode p)
{
	prover::termset r = prover::termset();
	prover::termset query;
	prover::termid o_var = prover->tmpvar();
	prover::termid iii = prover->make(p, s, o_var);
	query.emplace_back(iii);
	(*prover)(query);
	
	dout << "query: "<< prover->format(query) << endl;;
	dout << "substs: "<< std::endl;

	for (auto x : prover->substs) 
	{
		//prover->prints(x);
		//dout << std::endl;

		prover::subst::iterator binding_it = x.find(prover->get(o_var).p);
		if (binding_it != x.end())
		{
			r.push_back( (*binding_it).second);
		}
	}

	prover->substs.clear();
	return r;
}


const pnode rdfs_first=mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#first"));
const pnode rdfs_rest= mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#rest"));
const pnode rdfs_nil = mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#nil"));
const pnode pmatches = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#matches"));
const pnode pmustbos = mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/bnf#mustBeOneSequence"));



/*typedef Marpa_Symbol_ID sym;
typedef Marpa_Rule_ID rule;*/
typedef size_t pos;
typedef long long sym;
typedef std::vector<sym> syms;
typedef int rule;

class terminal
{
public:
	string name, regex;
	sym symbol;
	terminal(string name_, string regex_, sym symbol_){name = name_; regex = regex_; symbol = symbol_;}
};




struct Marpa{
	uint num_syms = 0;
	//Marpa_Grammar g;
	std::vector<terminal> terminals;
	map<prover::termid, sym> done;
	prover* grmr;

	void load_grammar(prover *_grmr, pnode root)
	{
		grmr = _grmr;
		sym start = add(grmr, grmr->make(root));
		start_symbol_set(start);
		terminals.push_back(terminal(L"comment", L"#.*", -2));
	}

	string value(prover::termid val)
	{
		return *dict[grmr->get(val).p].value;
	}

	string lexertl_literal(string s){return L"\"" + s + L"\"";}

	//create marpa symbols and rules from grammar description in rdf
	sym add(prover *grmr, prover::termid thing)
	{
		string thingv = value(thing);
		
		dout << "adding " << thing << std::endl;

		if (grmr->get(thing).isstr())
		{
			dout << "itsa str"<<std::endl;
			thingv = lexertl_literal(thingv);
			for (auto t :terminals)
				if (t.regex == thingv)
					return t.symbol;		
			sym symbol = ++num_syms;//check_int(marpa_g_symbol_new(g));
			terminals.push_back(terminal(thingv, thingv, symbol));
			return symbol;
		}
	
		if (done.find(thing) != done.end())
			return done[thing];	

		sym symbol = symbol_new(thing);
		prover::termset bind;
	
		if((bind = ask(grmr, thing, pmatches)).size())
		{
			terminals.push_back(terminal(thingv, value(bind[0]), symbol));
		}

		// note that in the grammar file things and terminology are switched,
		// mustBeOneSequence is a sequence of lists
		
		else if ((bind = ask(grmr, thing, pmustbos)).size())
		{
			for (auto l : bind) // thing must be one of these lists
			{
				dout << "[";
				syms rhs;
				while(1)
				{
					dout << "l: ";
					dout << l;
					dout << "..." << std::endl;
			
					prover::termset first = ask(grmr, l, rdfs_first);
					if (!first.size()) break;
					rhs.push_back(add(grmr, first[0]));
				
					prover::termset rest = ask(grmr, l, rdfs_rest);
					if (!rest.size()) {dout << "wut" << std::endl; break;}//err
					l = rest[0];
				}
				dout << "]" << std::endl;
				
				rule_new(symbol, rhs);
			}
		}
		else
			dout << "nope\n";
	}


int check_int(int result){
	if (result < 0)//== -2)
		error();
	return result;
}

void check_null(void* result){
	if (result == NULL)
		error();
}

void error_clear(){
	//marpa_g_error_clear(g);
}

sym symbol_new(prover::termid thing)
{
	return done[thing] = ++num_syms;//check_int(marpa_g_symbol_new(g));

}

rule rule_new(sym lhs, syms rhs) {
		return 5;//check_int(marpa_g_rule_new(g, lhs, rhs, rds.size()));
	}

rule sequence_new(sym lhs, sym rhs, sym separator=-1, int min=1, bool proper=false){
	return 5;//check_int(marpa_g_sequence_new(g, lhs, rhs, separator, min, proper ? MARPA_PROPER_SEPARATION : 0));
}


void print_events()
{	
	/*
	int count = check_int(marpa_g_event_count(g));
	dout << '%s events'%count
	if (count > 0)
	{
		Marpa_Event e;
		for (int i = 0; i < count; i++)
		{
			int etype = check_int(marpa_g_event(g, &e, i));
			dout << etype << ", " << e.t_value;
		}
	}
	*/
}

void start_symbol_set(sym s){
	//check_int( marpa_g_start_symbol_set(g, s) );
}

void error()
{
	/*
	int e = marpa_g_error(g, NULL);
	throw(new exception(e + ": " + errors[e]));
	*/
}


void parse (const string inp)
{
	/*
	Marpa_Recognizer r = marpa_r_new(g);
	check_null(r);
	marpa_r_start_input(r);
	*/

	//const std::string inp = ws(inp_);
	
	
	lexertl::wrules rules;
	lexertl::wstate_machine sm;

	dout << "terminals:\n";
	for (auto t:terminals)
	{
		dout << "(" << t.symbol << ")" << t.name << ": " << t.regex << std::endl;
		rules.push(t.regex, t.symbol);
	}
	dout << "build..\n";
	lexertl::wgenerator::build(rules, sm);

	//std::string input("abc012Ad3e4");
	
	lexertl::wsiterator iter(inp.begin(), inp.end(), sm);
	
	
	lexertl::wsiterator end;

	dout << "go..\n";

    for (; iter != end; ++iter)
    {
        dout << "Id: " << iter->id << ", Token: '" <<
            iter->str() << "'\n";
    }
    	dout << "\ndone\n";

	/*
	for(auto pos = inp.begin(); pos != inp.end();)
	{
		//boost::smatch m,m2;
		sym s;
		for (auto r : regexes) 
		{
		
			boost::regex_search (pos, inp.end(), m2, boost::regex(ws(r.first)));
			if (!m.valid or m.length > m2.length) {
				m = m2;
				s = r.second;
			}
		
		}
		
		//if (!m.valid) throw std::runtime_error("no match");
		
		//p += m.length();
		
	*/
	/*
		//#Return value: On success, MARPA_ERR_NONE. On failure, some other error code.
		int err = marpa_r_alternative(r, s, pos, 1);

		if (err == MARPA_ERR_UNEXPECTED_TOKEN_ID)
		{
			sym[sym_names.size()] expected;
			num_expected = check_int(marpa_r_terminals_expected(r, sym));
			dout << "expecting:";
			//for (int i = 0; i < num_expected; i++)
			dout << endl;
			error();
		}
		else if (r != MARPA_ERR_NONE)
			error();

		check_int(marpa_r_earleme_complete(r));
		lengths[pos] = m.lenght();
	}
	*/
	
	

}

void precompute(){
	//check_int(marpa_g_precompute(g));
	print_events();
}


};


std::string load_n3_cmd::desc() const {return "load n3";}
std::string load_n3_cmd::help() const 
{
	stringstream ss("Hilfe! Hilfe!:");
	return ss.str();
}


#include <iostream>
#include <fstream>


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
 	string input = load(args[2]);

	prover prover(convert(load_json(L"dot.jsonld")));
//	prover prover(convert(load_json(L"n3-grammar.jsonld")));
	Marpa m;
	m.load_grammar(
		&prover, 
		//mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/n3#objecttail")));
		mkiri(pstr(L"http://www.w3.org/2000/10/swap/grammar/n3#document")));
	m.precompute();
	
	m.parse(input);
	return 0;
}




/*
---------------------





	Grammar()
	{
		if (marpa_check_version (MARPA_MAJOR_VERSION ,MARPA_MINOR_VERSION, MARPA_MICRO_VERSION ) != MARPA_ERR_NONE)
			throw runtime_error ("marpa version...");
		Marpa_Config marpa_config;
		marpa_c_init(&marpa_config);
		g = marpa_g_new(marpa_config);
		if (!g)
			throw runtime_error ("marpa_g_new: error %d" % marpa_c_error (&marpa_config, NULL));
		if(lib.marpa_g_force_valued(s.g) < 0)
			throw runtime_error ("marpa_g_force_valued failed?!?!?");
	}
	~Grammar()
	{
		marpa_g_unref(g);
	}







*/


/*
resources
https://github.com/jeffreykegler/libmarpa/blob/master/test/simple/rule1.c#L18
https://github.com/jeffreykegler/Marpa--R2/issues/134#issuecomment-41091900
https://github.com/pstuifzand/marpa-cpp-rules/blob/master/marpa-cpp/marpa.hpp
*/




/*



	//marpa allows variously ordered lists of ambiguous parses,
	we just grab the default


	Marpa_Bocage b = marpa_b_new(r, -1);
	check_null(b);
	Marpa_Order o = marpa_o_new(b);
	check_null(o);
	Marpa_Tree t = marpa_t_new(o);
	check_null(t);
	check_int(marpa_t_next(t));
	Marpa_Value marpa_v_new(t);
	check_null(v);



	//"valuator" loop. marpa tells us what rules with what child nodes or tokens were matched, we build the ast with our "values"


	while(1)
	{
		Marpa_Step_Type st = check_int(marpa_v_step(v));
		if (st == MARPA_STEP_INACTIVE) break;
		switch(st)
		{
			case MARPA_STEP_TOKEN:
				sym id = marpa_v_symbol(v);

				obj x;
				x["name"] = names[id];

				pos p = marpa_v_token_value(v);
				stack[marpa_v_result(v)] = input[p:lengths[p]]

				break;
			case MARPA_STEP_RULE:

*/