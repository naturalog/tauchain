//#include tau.h
#include "marpa.h"

const string pmatches = "http://www.w3.org/2000/10/swap/grammar/bnf#matches";
const string pmbos = "http://www.w3.org/2000/10/swap/grammar/bnf#mustBeOneSequence";

map<int,regex> regexes

parsed (result, filename, grammar)
{

	start = symbol_new();
	add(grammar["document"])
	
	

	inp = read(filename)
	for(pos = inp.begin(); pos != inp.end();;)
	
		//lexing:just find longest match?
		regex:match m
		int symbol_id;
		for (auto r = regexes.begin(); r != regexes.end(); r++)
			m2 = r.second.match(inp)
			if !m.valid or m.length > m2.length
				m = m2
				symbol_id = r.first
		if !m.valid die brutally;
		
		
		alternative(symbol_id)
		earleme_complete()
		
	bocage, order, tree, valuator..
	
	
	
		in what form do we want the output?..


//create marpa grammar
add(obj x)
{
	s = symbol_new();
	if x.has(pmbos)
		for i in x[pmbos]
			rule_new(x, add_list(i))
	elif x.has(pmatches)
		regexes[s] = new regex (x[pmatches]).compile()
	return s
}	
			
add_list(list x)
{
	s = symbol_new();
	r = rule_new(s, [add(i) for i in x])
	return s
}
	