/*
http://www.w3.org/2000/10/swap/grammar/bnf
http://www.w3.org/2000/10/swap/grammar/n3.n3
*/

#include <iostream>
#include <fstream>

#include "prover.h"
#include "json_object.h"
#include "jsonld_tau.h"
#include "rdf.h"
#include "misc.h"
#include "jsonld.h"
#include "marpa_tau.h"

extern "C" {
#include "marpa.h"
#include "marpa_codes.c"
}

#include <boost/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>

#ifdef NOPARSER
#include "jsonld_tau.h"
#endif

extern bool irc;


typedef Marpa_Symbol_ID sym;
typedef Marpa_Rule_ID rule;
typedef std::vector<sym> syms;
typedef std::pair<string::const_iterator, string::const_iterator> tokt;


class terminal {
public:
	nodeid thing;
	string name, regex_string;
	boost::regex regex;

	terminal(nodeid thing_, string name_, string regex_)
	{
		name = name_;
		thing = thing_;
		regex_string = regex_;
		regex = boost::regex(regex_);
	}
};

typedef std::shared_ptr<terminal> pterminal;

struct MarpaIris {
	nodeid iri(const std::string s)
	{
		return dict[mkiri(pstr(prefix + s))];
	}

	string prefix = "http://idni.org/marpa#";
	const nodeid has_value = iri("has_value");
	const nodeid is_parse_of = iri("is_parse_of");
	const nodeid list_of = iri("list_of");
	const nodeid separator = iri("separator");
	const nodeid proper = iri("proper");
	const nodeid arg0 = iri("arg0");
	const nodeid arg1 = iri("arg1");
	const nodeid arg2 = iri("arg2");
};

MarpaIris *marpa = 0;

const string RDFS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
const string BNF = "http://www.w3.org/2000/10/swap/grammar/bnf#";

nodeid iri(const string prefix, const string s)
{
	return dict[mkiri(pstr(prefix + s))];
}

struct Marpa {
	Marpa_Grammar g;
	bool precomputed = false;
	// everything in the grammar is either a terminal, defined by a regex
	map<sym, pterminal> terminals;
	//, a "literal" - a simple string,
	map<sym, string> literals;
	///or a rule
	map<rule, sym> rules;
	//done tracks both rules and terminals
	map<nodeid, sym> done;
	/*prvr is the one we are called from. we will use prvr2 for querying the grammar, so that prvr doesnt get messed up*/
	prover *prvr, *prvr2;
	// i use this for comment regex, comments are processed kinda specially so they dont have to pollute the grammar
	string whitespace = "";
	const nodeid pcomma = dict[mkliteral(pstr(","), 0, 0)];
	const nodeid rdfs_nil = iri(RDFS, "nil");
	const nodeid rdfs_rest = iri(RDFS, "rest");
	const nodeid rdfs_first = iri(RDFS, "first");
	const nodeid bnf_matches = iri(BNF, "matches");
	const nodeid bnf_document = iri(BNF, "document");
	const nodeid bnf_whitespace = iri(BNF, "whiteSpace");
	const nodeid bnf_zeroormore = iri(BNF, "zeroOrMore");
	const nodeid bnf_mustBeOneSequence = iri(BNF, "mustBeOneSequence");
	const nodeid bnf_commaSeparatedListOf = iri(BNF, "commaSeparatedListOf");

	nodeid sym2resid(sym s)
	{
		for (auto it = done.begin(); it != done.end(); it++)
			if (it->second == s)
				return it->first;
		throw std::runtime_error("this sym isnt for a rule..this shouldnt happen");
	}

	nodeid rule2resid(rule r)
	{
		for (auto rr : rules)
			if (rr.first == r) {
				for (auto rrr: done)
					if (rr.second == rrr.second)
						return rrr.first;
			}
		throw std::runtime_error("and this shouldnt happen either");
	}

	string sym2str_(sym s)
	{
		if (literals.find(s) != literals.end())
			return /*"\"" +*/ literals[s] /*+ "\""*/;
		if (terminals.find(s) != terminals.end())
			return shorten_uri(terminals[s]->name) + ":" + terminals[s]->regex_string;
		return shorten_uri(*dict[sym2resid(s)].value);
	}

	string sym2str(sym s)
	{
		std::stringstream sss;
		//if (!irc)
		//	sss << "(" << s << ")";
		sss << sym2str_(s);
		return sss.str();
	}

	string value(nodeid val)
	{
		return *dict[val].value;
	}

	~Marpa()
	{
		marpa_g_unref(g);
	}

	Marpa(prover *prvr_, nodeid language)
	{
		assert(language);
		assert(prvr_);
		/*init marpa*/
		if (marpa_check_version(MARPA_MAJOR_VERSION, MARPA_MINOR_VERSION, MARPA_MICRO_VERSION) != MARPA_ERR_NONE)
			throw std::runtime_error("marpa version...");
		Marpa_Config marpa_config;
		marpa_c_init(&marpa_config);
		g = marpa_g_new(&marpa_config);
		if (!g) {
			stringstream ssss("marpa_g_new: error ");
			ssss << marpa_c_error(&marpa_config, NULL);
			throw std::runtime_error(ssss.str());
		}
		if (marpa_g_force_valued(g) < 0)
			throw std::runtime_error("marpa_g_force_valued failed?!?!?");

		if (!marpa)marpa = new MarpaIris();

		prvr = prvr_;
		prvr2 = new prover(*prvr);
		/*bnf:whitespace is a property of bnf:language*/
		nodeid whitespace_ = prvr2->askn1o(language, bnf_whitespace);
		if (whitespace_) {
			whitespace = value(whitespace_);
			TRACE(dout << "whitespace:" << whitespace << std::endl);
		}
		/*so is bnf:document, the root rule*/
		nodeid root = prvr2->askn1o(language, bnf_document);
		assert(root);
		sym start = add(root);
		start_symbol_set(start);
		delete prvr2;
	}

	//create marpa symbols and rules from grammar description in rdf
	sym add(nodeid thing)
	{
		assert(thing);
		//what we are adding
		string thingv = value(thing);
		TRACE(dout << "thingv:" << thingv << std::endl);

		//is it a rule or terminal thats been added or we started adding it already?
		if (done.find(thing) != done.end())
			return done[thing];

		//is it a literal?
		if ((dict[thing]._type == node::LITERAL) ||
			(dict[thing]._type == node::IRI && thingv == ".")) // crap: nquads parser bug workaround
		{
			//dout << "itsa str"<<std::endl;
			for (auto t: literals)
				if (t.second == thingv) // is it a done literal?
					return t.first;

			sym symbol = symbol_new();
			//dout << "adding " << thingv << std::endl;

			//we add it to the literals table and are done with it.
			literals[symbol] = thingv;
			return symbol;
		}

		//dout << "adding " << thingv << std::endl;
		sym symbol = symbol_new_resid(thing);

		// is it a...
		nodeid bind;
		if ((bind = prvr2->askn1o(thing, bnf_matches))) {
			//dout << "terminal: " << thingv << std::endl;
			terminals[symbol] = pterminal(new terminal(thing, thingv, value(bind)));
			return symbol;
		}
		if ((bind = prvr2->askn1o(thing, bnf_mustBeOneSequence))) {
			// mustBeOneSequence is a list of lists
			std::vector<nodeid> lll = prvr2->get_list(bind);
			if (!bind)
				throw runtime_error("mustBeOneSequence empty");

			for (auto l:lll) {
				syms rhs;
				std::vector<nodeid> seq = prvr2->get_list(l);
				for (auto rhs_item: seq)
					rhs.push_back(add(rhs_item));
				rule_new(symbol, rhs);
			}
		}
		else if ((bind = prvr2->askn1o(thing, bnf_commaSeparatedListOf))) {
			seq_new(symbol, add(bind), add(pcomma), 0, MARPA_PROPER_SEPARATION);
		}
		else if ((bind = prvr2->askn1o(thing, marpa->list_of))) {
			auto sep = prvr2->askn1o(thing, marpa->separator);
			auto proper = prvr2->askn1o(thing, marpa->proper);
			seq_new(symbol, add(bind), sep ? add(sep) : -1, 0, proper ? MARPA_PROPER_SEPARATION : 0);
		}
		else if ((bind = prvr2->askn1o(thing, bnf_zeroormore))) {
			seq_new(symbol, add(bind), -1, 0, 0);
		}
		else if (thingv == "http://www.w3.org/2000/10/swap/grammar/bnf#eof") { }//so what?
		else
			throw runtime_error("whats " + thingv + "?");

		TRACE(dout << "added sym " << symbol << std::endl);
		return symbol;
	}


	int check_int(int result)
	{
		if (result == -2)
			error();
		return result;
	}

	void check_null(void *result)
	{
		if (result == NULL)
			error();
	}

	void error_clear()
	{
		marpa_g_error_clear(g);
	}

	sym symbol_new()
	{
		return check_int(marpa_g_symbol_new(g));

	}

	sym symbol_new_resid(nodeid thing)
	{
		return done[thing] = symbol_new();

	}

	void rule_new(sym lhs, syms rhs)
	{
		TRACE(dout << sym2str(lhs) << " ::= ");
		for (sym s: rhs)
			TRACE(dout << sym2str(s));
		TRACE(dout << std::endl);
		auto s = rhs.size();
		auto a = s ? &rhs[0] : 0;
		auto r = marpa_g_rule_new(g, lhs, a, s);
		auto c = check_int(r);
		rules[c] = lhs;
		/*if (r == -2)
		{
			int e = marpa_g_error(g, NULL);
			if (e == MARPA_ERR_DUPLICATE_RULE)
			{
		*/
	}

	void seq_new(sym lhs, sym rhs, sym separator, int min, int flags)
	{
		int r = marpa_g_sequence_new(g, lhs, rhs, separator, min, flags);
		if (r == -2) {
			int e = marpa_g_error(g, NULL);
			if (e == MARPA_ERR_DUPLICATE_RULE)
				dout << sym2str(lhs) << " ::= sequence of " << sym2str(rhs) << std::endl;
		}
		rules[check_int(r)] = lhs;
	}


	void print_events()
	{

		int count = check_int(marpa_g_event_count(g));
		if (count > 0) {
			dout << count << " events" << std::endl;
			Marpa_Event e;
			for (int i = 0; i < count; i++) {
				int etype = check_int(marpa_g_event(g, &e, i));
				dout << " " << etype << ", " << e.t_value << std::endl;
			}
		}

	}

	void start_symbol_set(sym s)
	{
		check_int(marpa_g_start_symbol_set(g, s));
	}

	void error()
	{

		int e = marpa_g_error(g, NULL);
		stringstream s;
		s << e << ":" << marpa_error_description[e].name << " - " << marpa_error_description[e].suggested;
		throw (std::runtime_error(s.str()));// + ": " + errors[e]));

	}


	bool is_ws(char x)
	{
		string wss = "\n\r \t";

		for (auto ws: wss)
			if (x == ws)
				return true;
		return false;
	}


	ParsingResult parse(const string inp, termid &raw)
	{
		if (!precomputed) {
			check_int(marpa_g_precompute(g));
			precomputed = true;
		}

		print_events();

		TRACE(dout << "terminals:\n");
		for (auto t:terminals)
			TRACE(dout << "(" << t.first << ")" << t.second->name << ": " << t.second->regex << std::endl);

		TRACE(dout << "tokenizing..\n");

		std::vector<tokt> toks; // ranges of individual tokens within the input string
		toks.push_back(tokt(inp.end(), inp.end()));

		Marpa_Recognizer r = marpa_r_new(g);
		check_null(r);
		marpa_r_start_input(r);

		auto pos = inp.begin();

		std::vector<sym> expected;
		expected.resize(check_int(marpa_g_highest_symbol_id(g)));

		boost::regex whitespace_regex = boost::regex(whitespace);

		while (pos < inp.end()) {
			if (is_ws(*pos)) {
				pos++;
				continue;
			}

			boost::smatch what;

			if (whitespace.size() &&
				regex_search(pos, inp.end(), what, whitespace_regex, boost::match_continuous)) {
				if (what.size()) {
					int llll = what[0].length();
					TRACE(dout << "skipping " << llll << " comment chars" << std::endl);
					pos += llll;
					continue;
				}
			}

			std::vector<sym> best_syms;
			size_t best_len = 0;
			expected.clear();
			int num_expected = check_int(marpa_r_terminals_expected(r, &expected[0]));
			for (int i = 0; i < num_expected; i++) {
				sym e = expected[i];
				if (literals.find(e) != literals.end()) {
					if (boost::starts_with(string(pos, inp.end()), literals[e])) {
						if (literals[e].size() > best_len) {
							best_syms.clear();
							best_syms.push_back(e);
							best_len = literals[e].size();
						}
						else if (literals[e].size() == best_len)
							best_syms.push_back(e);
					}
				}
				else {
					if (terminals.find(e) != terminals.end()) {
						auto t = terminals[e];

						if (!regex_search(pos, inp.end(), what, t->regex, boost::match_continuous)) continue;
						if (what.empty()) continue;
						size_t l = what.length();
						if (l > best_len) {
							best_len = l;
							best_syms.clear();
							best_syms.push_back(e);
						}
						else if (l == best_len)
							best_syms.push_back(e);
					}
				}
			}

			if (best_len) {
				if (best_syms.size() > 1) {
					dout << "cant decide between:" << std::endl;
					for (auto ccc: best_syms)
						dout << " " << sym2str(ccc) << std::endl;
				}
				assert(best_syms.size());
				toks.push_back(tokt(pos, pos + best_len));
				TRACE(dout << std::distance(inp.begin(), pos) << "-" <<
					  std::distance(inp.begin(), pos + best_len) <<
					  " \"" << string(pos, pos + best_len) << "\" - " << sym2str(best_syms[0]) << std::endl);
				if (MARPA_ERR_UNEXPECTED_TOKEN_ID ==
					check_int(marpa_r_alternative(r, best_syms[0], toks.size(), 1)))
					return FAIL;
				check_int(marpa_r_earleme_complete(r));
				pos += best_len;
			}
			else {
				auto pre(pos);
				size_t charnum = 0;
				while (pre != inp.begin() && *pre != '\n') {
					pre -= 1;
					charnum++;
				}
				//pre -= 10;
				auto post(pos);
				while (post != inp.end() && *post != '\n')
					post += 1;
				//post += 10;
				dout << "[n3]at line " << 1 + std::count(inp.begin(), pos, '\n') << ", char " << charnum <<
				":" << std::endl;
				auto poss(pos);
				//if (poss != inp.begin()) poss--;
				dout << string(pre, poss) << "<HERE>" << string(pos, post) << std::endl;
				//                        dout << "..\"" << string(pre, pos-1) << "<HERE>" << string(pos, post) << "\"..." << std::endl;


				dout << "[n3]expecting: ";
				//if (!irc) dout << std::endl;
				for (int i = 0; i < num_expected; i++) {
					sym e = expected[i];
					dout << sym2str(e) << " ";
					//if (!irc)
				}
				dout << std::endl;
				return FAIL;
			}
		}

		TRACE(dout << "retrieving parse tree.." << std::endl);

		//marpa allows variously ordered lists of ambiguous parses, we just grab the default
		Marpa_Bocage b = marpa_b_new(r, -1);
		if (!b) {
			TRACE(dout << "[n3]parsing failed, failed to create bocage" << std::endl);
			return FAIL;
		}
		Marpa_Order o = marpa_o_new(b);
		check_null(o);
		Marpa_Tree t = marpa_t_new(o);
		check_null(t);
		check_int(marpa_t_next(t));
		Marpa_Value v = marpa_v_new(t);
		check_null(v);

		//"valuator" loop. marpa tells us what rules with what child nodes or tokens were matched,
		//we build the value/parse/ast in the stack, bottom-up

		map<int, termid> stack;
		map<int, string> sexp;

		while (1) {
			Marpa_Step_Type st = check_int(marpa_v_step(v));
			print_events();
			if (st == MARPA_STEP_INACTIVE) break;
			switch (st) {
				case MARPA_STEP_TOKEN: {
					sym symbol = marpa_v_symbol(v);
					size_t token = marpa_v_token_value(v) - 1;
					string token_value = string(toks[token].first, toks[token].second);
					sexp[marpa_v_result(v)] = "/*" + token_value + "*/";
					termid xx;
					if (terminals.find(symbol) != terminals.end()) {
						xx = prvr->make(mkbnode(gen_bnode_id()));
						prvr->kb.add(prvr->make(bnf_matches, xx, prvr->make(terminals[symbol]->thing)));
						prvr->kb.add(prvr->make(marpa->has_value, xx,
												prvr->make(mkliteral(pstr(token_value), XSD_STRING, 0))));
					}
					else // it must be a literal
						xx = prvr->make(mkliteral(pstr(token_value), 0, 0));
					stack[marpa_v_result(v)] = xx;
					break;
				}
				case MARPA_STEP_RULE: {
					nodeid res = rule2resid(marpa_v_rule(v));
					string sexp_str = "\"" + value(res) + "\":{ ";

					std::list<termid> args;
					for (int i = marpa_v_arg_0(v); i <= marpa_v_arg_n(v); i++) {
						if (stack[i]) {
							args.push_back(stack[i]);
							sexp_str += sexp[i] + " ";
						}
					}

					termid xx;
					// if its a sequence
					if (check_int(marpa_g_sequence_min(g, marpa_v_rule(v))) != -1) {
						xx = prvr->list2term_simple(args);
					}
					else {
						xx = prvr->make(mkbnode(gen_bnode_id()));
						//dout << ".xx: " << prvr->format(xx) << std::endl;
						for (int i = 0; i <= marpa_v_arg_n(v) - marpa_v_arg_0(v); i++) {
							termid arg = stack[marpa_v_arg_0(v) + i];

							if (!arg) continue; // if it was nulled

							sym arg_sym = check_int(marpa_g_rule_rhs(g, marpa_v_rule(v), i));

							if (literals.find(arg_sym) == literals.end())
								prvr->kb.add(prvr->make(sym2resid(arg_sym), xx, arg));

							std::stringstream arg_pred;
							arg_pred << "http://idni.org/marpa#arg" << i;

							prvr->kb.add(prvr->make(mkiri(pstr(arg_pred.str())), xx, arg));
						}
					}

					prvr->kb.add(prvr->make(marpa->is_parse_of, xx, prvr->make(res)));
					//dout << "xx: " << prvr->format(xx) << std::endl;
					stack[marpa_v_result(v)] = xx;
					sexp[marpa_v_result(v)] = sexp_str + "} ";

					break;
				}
				case MARPA_STEP_NULLING_SYMBOL:
					sexp[marpa_v_result(v)] = "0";
					stack[marpa_v_result(v)] = 0;
					break;
				default:
					dout << marpa_step_type_description[st].name << std::endl;


			}
		}
		marpa_v_unref(v);
		if (check_int(marpa_t_next(t)) != -1)
			throw std::runtime_error("ambiguous parse");
		marpa_t_unref(t);
		marpa_o_unref(o);
		marpa_b_unref(b);

		TRACE(dout << "{" << sexp[0] << "}" << std::endl << std::endl);

		raw = stack[0];
		TRACE(dout << "result0: " << prvr->format(raw) << std::endl);
		return COMPLETE;
	}


};


class N3 {
public:

	nodeid uri(std::string s)
	{
		return dict[mkiri(pstr("http://www.w3.org/2000/10/swap/grammar/n3#" + s))];
	}

	nodeid n3symbol = uri("symbol");
	nodeid n3subject = uri("subject");
	nodeid n3object = uri("object");
	nodeid n3objecttail = uri("objecttail");
	nodeid n3explicituri = uri("explicituri");
	nodeid n3propertylist = uri("propertylist");
	nodeid n3propertylisttail = uri("propertylisttail");
	nodeid n3expression = uri("expression");
	nodeid n3pathitem = uri("pathitem");
	nodeid n3predicate = uri("predicate");
	//nodeid n3statement_with_dot = uri("statement_with_dot");
	nodeid n3statement = uri("statement");
	nodeid n3declaration = uri("declaration");
	nodeid n3simpleStatement = uri("simpleStatement");
	nodeid n3prefix = uri("prefix");
	nodeid n3qname = uri("qname");
	nodeid n3literal = uri("literal");
	nodeid n3numericliteral = uri("numericliteral");
	nodeid n3string = uri("string");
	nodeid n3boolean = uri("boolean");
	nodeid n3integer = uri("integer");
	//nodeid n3dtlang = uri("dtlang");
	nodeid n3quickvariable = uri("quickvariable");
	nodeid n3formulacontent = uri("formulacontent");

	prover *prvr;

	qdb &kb;
	qdb &query;

	qdb *dest;
	bool finfin_mode;
	int fins = 0;

	string base;
	map<string, string> prefixes;

	//bool has_keywords = false;
	//std::vector<string> keywords;

	prover::nodeids get_dotstyle_list(termid l)
	{
		std::list<nodeid> r;
		prvr->get_dotstyle_list(l, r);
		prover::nodeids rr;
		for (auto x:r)
			rr.push_back(x);
		return rr;
	}

	//query marpa#has_value, return string
	string get_value(nodeid n)
	{
		assert(n);
		nodeid v = q(n, marpa->has_value);
		assert(v);
		auto s = dict[v].value;
		assert(s);
		return *s;
	}

	N3(prover &prvr_, qdb &kb_, qdb &query_, bool single_file_mode_ = false) : prvr(&prvr_), kb(kb_), query(query_),
																			   dest(&kb),
																			   finfin_mode(single_file_mode_)
	{
		if (!marpa)marpa = new MarpaIris();
	}

	nodeid q(nodeid s, nodeid p)
	{
		return prvr->askn1o(s, p);
	}

	void trim(string &s, string xxx)
	{
		assert(startsWith(s, xxx));
		assert(endsWith(s, xxx));
		s.erase(0, xxx.size());
		s.erase(s.size() - xxx.size());
	}

	pnode add_literal(nodeid x)
	{
		nodeid sss = q(x, n3string);
		assert (sss);
		nodeid v = q(sss, marpa->has_value);
		assert(v);
		string s = *dict[v].value;
		string triple = string("\"\"\"");

		if (startsWith(s, triple))
			trim(s, triple);
		else
			trim(s, "\"");

		return mkliteral(pstr(s), 0, 0);
	}

	pnode add_numericliteral(nodeid x)
	{
		nodeid sss = q(x, n3integer);
		assert (sss);
		nodeid v = q(sss, marpa->has_value);
		assert(v);
		return mkliteral(dict[v].value, XSD_INTEGER, 0);
	}

	pnode add_boolean(nodeid x)
	{
		nodeid v = q(x, marpa->arg0);
		assert(v);

		auto s = *dict[v].value;
		s.erase(0, 1);
		return mkliteral(pstr(s), XSD_BOOLEAN, 0);
	}

	pnode add_symbol(nodeid x)
	{
		nodeid uri = q(x, n3explicituri);
		if (uri) {
			nodeid val = q(uri, marpa->has_value);
			assert(val);
			string uri_s = *dict[val].value;
			assert(uri_s.size() > 1);
			string expluri = string(uri_s.begin() + 1, uri_s.end() - 1);
			return mkiri(pstr(expluri));
		}
		nodeid qname = q(x, n3qname);
		if (qname) {
			string v = get_value(qname);

			auto pos = v.find(":");
			if (pos != string::npos) {
				string pref = string(v.begin(), v.begin() + pos);
				TRACE(dout << "comparing \"" << pref << "\" with ";
							  for (auto it: prefixes)
								  dout << it.first << " ";
							  dout << std::endl;)
				if (prefixes.find(pref) != prefixes.end()) {
					string rest = string(v.begin() + pos + 1, v.end());
					v = prefixes[pref] + rest;
				}
			}
			else {
				v = base + v;
			}
			return mkiri(pstr(v));
		}
		assert(false);
	}

	pnode add_quickvariable(nodeid x)
	{
		nodeid v = q(x, marpa->has_value);
		assert(v);
		string s = *dict[v].value;
		//s[0] = '?';//workaround
		return mkiri(pstr(s));
	}

	pnode add_formulacontent(termid x)
	{
		auto graph = mkbnode(gen_bnode_id());
		add_statements(x, *graph->value);
		return graph;
	}

	//this does what it says
/*	out_of_tape> its probably not quite to the spec and buggy
<out_of_tape> the idea is to turn lists that come like (x y z) into triples*/
/*this is then picked up by get_list()*/
	pnode create_list_triples(std::vector<pnode> l)
	{
		string head = *RDF_NIL;

		if (l.size())//is empty list simply a nil?
		//are you asking me? i think yes//if you know the answer then yes:)
		//hehe i'll check the rdf specs later but we'll roll with it for now i guess ok
		{

			if (dest->first.find("@default") == dest->first.end())
				dest->first["@default"] = mk_qlist();

			listid();

			head = list_bnode_name(0);
			string x = head;
			auto &c = *dest->first["@default"];
			auto size = l.size();
			for (size_t idx = 0; idx < size;) {
				pnode i = l[idx];
				pnode y = mkbnode(pstr(x));
				//we just write into the dest kb
				c.push_back(make_shared<quad>(quad(y, mkiri(RDF_FIRST), i)));
				x = list_bnode_name(++idx);
				pnode z = (idx == size) ? mkiri(RDF_NIL) : mkbnode(pstr(x));
				c.push_back(make_shared<quad>(quad(y, mkiri(RDF_REST), z)));
			}
		}
		return mkbnode(pstr(head));
	}


	pnode add_pathlist(termid/*TERMID - internalized list*/x)
	{
		std::vector<pnode> r;
		if (x) {
			//how does this work with nested lists?
			//isnt the nodeid of the item a dot?
			prover::nodeids items = get_dotstyle_list(x);
			for (auto i:items)
				r.push_back(add_expression(i));
		}
		return create_list_triples(r);
	}

	pnode add_pathitem(nodeid pi)
	{
		nodeid sym = q(pi, n3symbol);
		if (sym)
			return add_symbol(sym);
		termid f = prvr->askt1o(pi, n3formulacontent);
		if (f)
			return add_formulacontent(f);
		nodeid qv = q(pi, n3quickvariable);
		if (qv)
			return add_quickvariable(qv);
		nodeid nl = q(pi, n3numericliteral);
		if (nl)
			return add_numericliteral(nl);
		nodeid lit = q(pi, n3literal);
		if (lit)
			return add_literal(lit);

		//( "[" propertylist "]"  )

		nodeid l = q(pi, marpa->arg0);
		if (l && *dict[l].value == "(") {
			termid x = prvr->askt1o(pi, marpa->arg1);
			return add_pathlist(x);
		}
		nodeid b = q(pi, n3boolean);
		if (b)
			return add_boolean(b);
		assert(false);

	}

	pnode add_expression(nodeid e)
	{
		nodeid sepi = q(e, n3pathitem);
		assert(sepi);
		//todo pathtail
		return add_pathitem(sepi);
	}

	void add_simpleStatement(nodeid sim, string graph)
	{
		assert(sim);
		TRACE(dout << "   " << sim << "   ");
		nodeid subj = q(sim, n3subject);
		assert(subj);
		nodeid se = q(subj, n3expression);
		assert(se);
		nodeid sepi = q(se, n3pathitem);
		assert(sepi);
		pnode subject = add_pathitem(sepi);


		nodeid sspl = q(sim, n3propertylist);
		if (sspl) {
			nodeid prop = sspl;
			prover::nodeids props;
			while (prop) {
				props.push_back(prop);
				prop = q(prop, n3propertylisttail);
				if (prop)
					prop = q(prop, n3propertylist);
			}
			TRACE(dout << "props:" << props.size());
			//now we have property lists in a list
			for (auto prop:props) {

				pnode predicate;
				bool reverse = false;

				nodeid pred = q(prop, n3predicate);
				nodeid pe = q(pred, n3expression);

				//get string value of first item
				nodeid i0 = q(pred, marpa->arg0);
				string i0v = *dict[i0].value; //?

				if (pe) { // if this predicate contains an expression
					predicate = add_expression(pe);
					/*      ( expression )
						( "@has" expression )
					  ( "@is" expression "@of" )*/
					if (i0v == "@is")
						reverse = true;
				}
				else {
					string ps;
					if (i0v == "@a")
						ps = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
					else if (i0v == "=")
						ps = "http://www.w3.org/2002/07/owl#sameAs";
					else if (i0v == "=>")
						ps = "http://www.w3.org/2000/10/swap/log#implies";
					else if (i0v == "<=") {
						ps = "http://www.w3.org/2000/10/swap/log#implies";
						reverse = true;
					}
					else
						throw std::runtime_error("this shouldnt happen");
					predicate = mkiri(pstr(ps));
				}

				/* objects */
				std::vector<pnode> objs;
				nodeid t = prop;
				while (t) {
					nodeid ob = q(t, n3object);
					if (!ob)break;
					nodeid oe = q(ob, n3expression);
					objs.push_back(add_expression(oe));
					t = q(t, n3objecttail);
				}

				for (auto object: objs) {
					pnode s, o;
					if (reverse) {
						s = object;
						o = subject;
					}
					else {
						o = object;
						s = subject;
					}
					if (dest->first.find(graph) == dest->first.end())
						dest->first[graph] = mk_qlist();
					dest->first[graph]->push_back(make_shared<quad>(s, predicate, o, graph));
				}
			}
		}
		else if (finfin_mode && *subject->value == "fin" && graph == "@default") {
			finfin_mode = false;//ignore further fins
			fins++;
			if (fins == 1) {
				//dout << kb.first.size() << " rules loaded." << std::endl;
				dest = &query;
			}
		}
	}

	void parse_declaration(nodeid decl)
	{
		nodeid a0 = q(decl, marpa->arg0);
		assert(a0);
		string dec = *dict[a0].value;
		TRACE(dout << "DECLARATION:" << dec << std::endl;)
		if (dec == "@prefix") {
			nodeid p = q(decl, n3prefix);
			nodeid uri = q(decl, n3explicituri);
			string uri_s = get_value(uri);
			string p_s = get_value(p);
			assert(p_s.size());
			assert(string(p_s.end() - 1, p_s.end()) == ":");
			string prefix = string(p_s.begin(), p_s.end() - 1);
			assert(uri_s.size() > 1);
			string expluri = string(uri_s.begin() + 1, uri_s.end() - 1);
			prefixes[prefix] = expluri;
			TRACE(dout << "@prefix\"" << p_s << "\": \"" << uri_s << "\"" << std::endl;)
		}
		else throw std::runtime_error("not supported: " + dec);
		//todo:make an own error class and throw it here and catch it outside and return a parsing FAIL, dont die
		//else if(*dict[a0].value == "@keywords")
	}

	void add_statements(termid list, string graph)
	{
		prover::nodeids statements_and_dots = get_dotstyle_list(list);
		TRACE(dout << std::endl << graph << ":" << std::endl);
		for (auto s: statements_and_dots) {
			nodeid x = q(s, n3simpleStatement);
			if (x)
				add_simpleStatement(x, graph);
			else if ((x = q(s, n3declaration)))
				parse_declaration(x);
		}
	}
};


string load_file(std::istream &f)
{
	std::stringstream ss;
	ss << f.rdbuf();
	return ss.str();
}


void open_file(std::ifstream &f, string gfn)
{
	f.open(gfn);
	if (!f.is_open())
		throw std::runtime_error("couldnt open file \"" + gfn + "\"");
}

void hack_add( qdb &gkb, string s, string p, string o)
{
	gkb.first.at("@default")->push_front(make_shared<quad>(quad(s, p, o, "@default")));
}

ParsingResult parse_natural3(qdb &kb, qdb &q, std::istream &f, string base)
{
	setproc("N3");
	static Marpa *parser = 0;
	if (!parser) {

		std::ifstream gf;
		qdb gkb;
#ifndef NOPARSER
		open_file(gf, "n3-grammar.nq");
		nqparser p;
		p.parse(gkb, gf);
#else
		open_file(gf, "n3-grammar.jsonld");
		assert(load_jsonld ( gkb, gf ) == COMPLETE);
#endif
		/*hack_add(gkb, "string", BNF+"matches", "("""[^"\\]*(?:(?:\\.|"(?!""))[^"\\]*)*""")|("[^"\\]*(?:\\.[^"\\]*)*")");
		 * http://www.w3.org/TeamSubmission/n3/#grammar
		 * http://userguide.icu-project.org/strings/regexp
		 * also see branch icu
		 * */

		static prover grmr(gkb);

		TRACE(dout << "grammar loaded." << std::endl);
		//TRACE(dout << std::endl << std::endl << "grmr:" << std::endl << grmr.formatkb());
		parser = new Marpa(&grmr, dict[mkiri(pstr("http://www.w3.org/2000/10/swap/grammar/n3#language"))]);
	}

	string in = load_file(f);
	termid raw;
	ParsingResult success = parser->parse(in, raw);

	if (success == COMPLETE) {
		TRACE(dout << std::endl << std::endl << "prvr:" << std::endl << parser->prvr->formatkb());

		if (!raw)
			return FAIL;

		TRACE(dout << "retrieving results." << std::endl);

		N3 n3(*parser->prvr, kb, q, true);
		n3.base = base;
		n3.add_statements(raw, "@default");
	}

	return success;
}


/*builtins*/
/*
void *marpa_parser(prover *p, nodeid language) {
	return (void*)new Marpa(p, language);
}

termid marpa_parse(void* marpa, string input) {
	Marpa *m = (Marpa *)marpa;
	termid result; = m->parse(input);
	return result;
}

string load_file(std::ifstream &f) {
	std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	return ws(str);
}
*/


/*
resources

https://github.com/jeffreykegler/libmarpa/blob/master/test/simple/rule1.c#L18
https://github.com/jeffreykegler/Marpa--R2/issues/134#issuecomment-41091900
https://github.com/pstuifzand/marpa-cpp-rules/blob/master/marpa-cpp/marpa.hpp
 */
