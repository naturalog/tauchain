//#include tau.h
#include "marpa.h"

const string pmatches = "http://www.w3.org/2000/10/swap/grammar/bnf#matches";
const string pmbos = "http://www.w3.org/2000/10/swap/grammar/bnf#mustBeOneSequence";

        typedef int earleme;
        typedef int sym;
        typedef int rule;
        typedef int event_type;
        typedef int rank;

struct Grammar{
	Marpa_Grammar g;
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

	void check_int(int result){
		if (result < 0)//== -2)
			error();
		return result;
	}

	void check_null(int result){
		if (result == NULL)
			error();
		return result;
	}

	void error_clear(){
		marpa_g_error_clear(g);
	}

	sym symbol_new()
	{
		return check_int(marpa_g_symbol_new(g));
	}
	
	/*
	bool symbol_is_accessible(s, sym):
		assert type(sym) == symbol_int
		r = s.check_int(lib.marpa_g_symbol_is_accessible(s.g, sym))
		assert r != -1
		return r
	*/	
	
	rule_id rule_new(sym lhs, syms rhs) {
            return check_int(marpa_g_rule_new(g, lhs, rhs, rds.size()));
        }

	rule_id sequence_new(sym lhs, sym rhs, sym separator=-1, int min=1, bool proper=false){
		return check_int(marpa_g_sequence_new(g, lhs, rhs, separator, min,
		    proper ? MARPA_PROPER_SEPARATION : 0));
	}

	void precompute(s):
		check_int(marpa_g_precompute(g));
		print_events();
	}

	def events(s):
		count = s.check_int(lib.marpa_g_event_count(s.g))
		log('%s events'%count)
		if count > 0:
			result = ffi.new('Marpa_Event*')
			for i in range(count):
				event_type = s.check_int(lib.marpa_g_event(s.g, result, i))
				event_value = result.t_value
				r = event_type, event_value
				log((i,r))
				yield r

	def print_events(s):
		for dummy in s.events():
			pass

	void start_symbol_set(sym_id s){
		check_int( marpa_g_start_symbol_set(g, s) );
	}

	void error(int s){
		int e = marpa_g_error(g, NULL);
		throw(new exception(e + ": " + errors[e]));
	}

def log_error(e):
	err = codes.errors[e]
	log(err[0] + " " + repr(err[1]) )#constant + description

class Recce(object):
	def __init__(s, g):
		s.g = g
		s.r = s.g.check_null(lib.marpa_r_new(g.g))
		log(s.r)
	def __del__(s):
		lib.marpa_r_unref(s.r)
	def start_input(s):
		s.g.check_int(lib.marpa_r_start_input(s.r))

	def alternative(s, sym, val, length=1):
		assert type(sym) == symbol_int
		assert type(val) == int
		
		r = lib.marpa_r_alternative(s.r, sym, val, length)
		#Return value: On success, MARPA_ERR_NONE. On failure, some other error code.
		if r != lib.MARPA_ERR_NONE:
			log_error(r)


	def earleme_complete(s):
		s.g.check_int(lib.marpa_r_earleme_complete(s.r))
	def latest_earley_set(s):
		return lib.marpa_r_latest_earley_set(s.r)

class Bocage(object):
	def __init__(s, r, earley_set_ID):
		s.g = r.g
		s.b = s.g.check_null(lib.marpa_b_new(r.r, earley_set_ID))
		log(s.b)
	def __del__(s):
		lib.marpa_b_unref(s.b)

class Order(object):
	def __init__(s, bocage):
		s.g = bocage.g
		s.o = s.g.check_null(lib.marpa_o_new(bocage.b))
		log(s.o)
	def __del__(s):
		lib.marpa_o_unref(s.o)

class Tree(object):
	def __init__(s, order):
		s.g = order.g
		s.t = s.g.check_null(lib.marpa_t_new(order.o))
		log(s.t)
	def __del__(s):
		lib.marpa_t_unref(s.t)
	def nxt(s):
		while True:
			r = s.g.check_int(lib.marpa_t_next(s.t))
			if r == -1:
				break
			yield r

class Valuator(object):
	def __init__(s, tree):
		s.g = tree.g
		s.v = s.g.check_null(lib.marpa_v_new(tree.t))
		log(s.v)
	def __del__(s):
		if s.v != None:
			s.unref()
	def unref(s):
		lib.marpa_v_unref(s.v)
		s.v = None
	def step(s):
		assert s.v
		return s.g.check_int(lib.marpa_v_step(s.v))
		







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
	
	
	
	
	
	
	
/*
resources
https://github.com/jeffreykegler/libmarpa/blob/master/test/simple/rule1.c#L18
https://github.com/jeffreykegler/Marpa--R2/issues/134#issuecomment-41091900
https://github.com/pstuifzand/marpa-cpp-rules/blob/master/marpa-cpp/marpa.hpp
