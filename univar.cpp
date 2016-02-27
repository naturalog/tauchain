#include <functional>
#include <unordered_map>
#include <queue>
#include <limits>
#include <string.h>
#include <assert.h>

#include "univar.h"

/* http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
 * http://www.codeproject.com/Articles/29524/Generators-in-C
 * < HMC_Alf> JIT are just crazy "specialized jump table" constructing machines
 *
 */

using namespace std;

//Must only be used when (x).find(y) and (x).end() will be defined.
#define has(x,y) ((x).find(y) != (x).end())

typedef unsigned char byte;
typedef size_t pos_t;

typedef intptr_t offset_t;//ptrdiff_t?

intptr_t ofst(pos_t target, pos_t me)
{
	assert(target < numeric_limits<int>::max());
	assert(me < numeric_limits<int>::max());
	return (int)target - (int)me;
}

unsigned long kbdbg_part;
unsigned long kbdbg_part_max;

extern int result_limit ;



//just leave it for now maybe change it later
//we don't modify this for any compiler directives/macros, can we just
//use the regular code instead? i guess, i just turned it into a macro the second or third time i was going thru all the code and changing the line when it was in flux
#define EEE char entry = 0

//we don't modify this one either
#define TRACE_ENTRY TRACE(dout << "entry = " << (int)entry << endl)
//At the very least call them ENTRY & TRCENTRY //sounds good
/*after all, these macros are an abstraction like any other....
so lets give them good names and leave them there?*/

const char LAST = 33; // last case statement (in release mode), not last entry, the coro might still be invoked with 33 repeatedly before returning false


//my keybindings are all default and fuckedup


//DEBUG directive:
#ifdef DEBUG

#define DBG(x) x
#define DBGC(x) x,
//same as DBG
#define TRC(x) x

#define TRCCAP(x) ,x
#define ITEM(x,y) x.at(y)

#define ASSERT assert

//For the switch coros
#define case_LAST case LAST
#define DONE {entry = 66; return false; }
#define END DONE; default: assert(false);

#else

#define DBG(x)
#define DBGC(x)
//same as DBG
#define TRC(x)

#define TRCCAP(x)
#define ITEM(x,y) x[y]

#define ASSERT(x)

//For the switch coros
#define case_LAST default
#define DONE return false
#define END DONE;

#endif





//NEW & KBDBG directives:
#ifdef NEW
	#define KBDBG
#endif

#ifdef KBDBG
	#ifdef oneword
	//what specifically does nope do?
	choke the parser
		nope
	#endif
	#ifndef DEBUG
		nope
	#endif
	#define IFKBDBG(x) x
#else
	#define IFKBDBG(x)
#endif



//for kbdbg
typedef vector<unsigned long> Markup;
typedef std::pair<nodeid, Markup> toadd;








enum ThingType {BOUND, NODE, OFFSET, LIST, LIST_BNODE, UNBOUND};


//Two different versions of Thing:
#ifndef oneword

//Version A:
/***************************************************************/
/*so, the idea behind this new data structuring is that each little thing doesnt have to be allocated separately,
we can put them in one big array per rule, and we can initialize such locals array from a template simply by memcpy
this is just an attempt at optimization, it isnt a change needed for the correct functioning

	UNBOUND, 	// unbound var
	BOUND,  	// bound var
	NODE, 		// nodeid - atom
	LIST, 		// has size, is followed by its items
	LIST_BNODE	// ...
	OFFSET		// pointer offset to another item in the vector

so, a rule needs local variables shared among its body joins.
these will be in one contiguous vector of things
a link is an offset, it points to another thing within the vector
its like a rule-local var
the reason for this is that vars may occur multiple times, for example:
{(?z ?y) a b. (?y ?x) a b} => ...
we have two distinct lists in this rule, each contains ?y
the lists themselves will be just consecutive things in the locals vector
*/

class Thing {
public:
	//Structure
	ThingType type;
	union {
		//maybe call this value? how about binding?
		//both sound appropriate; better than thing anyway
		Thing *thing;     // for bound var
		nodeid node;      // for node
		size_t size;      // for list. not sure
		offset_t offset;
	};
#ifdef KBDBG
	Markup markup; //a list of ints saying where in the reconstructed kb text (by print_kbdbg) this Thing comes from
#endif

	Thing(ThingType type_, offset_t thing_) : type(type_), offset(thing_) {};
	Thing() {};
};


//we could add some assertions here maybe
//this abstracts away oneword and struct implementations of Thing
//Thing::Access
//All these macros must be applied to Things
#define get_type(thing) ((thing).type)
#define get_node(thing) (DBGC(ASSERT(get_type(thing) == NODE || get_type(thing) == LIST_BNODE)) /*then return*/ (thing).node)
#define get_size(thing) ((thing).size)
//get_binding(var) or get_value(var) would be better
#define get_thing(ttttt) ((ttttt).thing)
#define get_offset(thing) ((thing).offset)


//these make sense
//Thing::Property -- specifically a unary boolean function Things
#define is_offset(thing)	(get_type(thing) == OFFSET)
#define is_unbound(thing)	(get_type(thing) == UNBOUND)
#define is_bound(thing)		(get_type(thing) == BOUND)
#define is_list(thing)		(get_type(thing) == LIST)
#define is_list_bnode(thing)(get_type(thing) == LIST_BNODE)
#define is_node(thing)		(get_type(thing) == NODE)
#define is_var(thing)		(get_type(thing) == BOUND || get_type(thing) == UNBOUND)


//Thing::Comparison
#define types_differ(x, y) (x.type != y.type)
#define sizes_differ(x, y) (x.size != y.size)
#define are_equal(x, y) ((x).type == (y).type && (x).size == (y).size)



//Thing::Constructors:
//3 different types of Things which can be created:
inline Thing create_unbound()
{
	Thing x;
	x.type = UNBOUND;
	DBG(x.thing = (Thing *)666;)
	return x;
}

inline Thing create_node(nodeid v)
{
	Thing x;
	x.type = NODE;
	x.node = v;
	return x;
}

inline Thing create_list_bnode(nodeid v)
{
	Thing x;
	x.type = LIST_BNODE;
	x.node = v;
	return x;
}





//Thing::Update
//will perhaps want to assert that 'me' is an unbound variable
inline void make_this_bound(Thing *me, Thing *val)
{
	me->type = BOUND;
	me->thing = val;
}

//will perhaps want to assert that 'me' is a bound variable
inline void make_this_unbound(Thing * me)
{
	me->type = UNBOUND;
}

void make_this_offset(Thing &t, offset_t o) {
	t.type = OFFSET;
	t.offset = o;
}

void make_this_list(Thing &i0, size_t size)
{
	i0.type = LIST;
	i0.size = size;
}


#else


//Version B:
/***************************************************************/
/* oneword:
kinda like http://software-lab.de/doc/ref.html#cell but with bits in the pointees #todo more relevant link

 00 = var        // address left intact or zero
 01 = node       // we mostly just compare these anyway
010 = positive offset
110 = negative offset
 11 = list(size)
 gotta add list bnode
*/

typedef uintptr_t *Thing; // unsigned int that is capable of storing a pointer
#define databits(x) (((uintptr_t)x) & ~0b11)
#define typebits(t) ((uintptr_t)t & (uintptr_t)0b11)
static_assert(sizeof(uintptr_t) == sizeof(size_t), "damn");


//Thing::Constructors
static inline Thing create_unbound()
{
	return 0;
}

static inline Thing create_node(termid v)
{
	ASSERT(((uintptr_t)v & 0b11) == 0);
	return (Thing)((uintptr_t)v | 0b01);
}


//Thing::Update
static inline void make_this_bound(Thing * me, Thing * v)
{
	ASSERT(((uintptr_t)v & 0b11) == 0);
	*me = (Thing)v;
}

static inline void make_this_unbound(Thing * me)
{
	*me = 0;
}

void make_this_offset(Thing &t, offset_t o) {
	byte sign = 0;
	if (o < 0)
	{
		sign = 1;
		o = -o;
	}
	uintptr_t r = o;
	r = r << 1;
	r |= sign;
	r = r << 2;
	r |= 0b10;
	t = (Thing)r;
}

void make_this_list(Thing &i0, size_t size)
{
	i0 = (Thing)((size << 2) | 0b11);
}

//Thing::Access/Get
static inline offset_t get_offset(Thing x)
{
	uintptr_t xx = (uintptr_t)x;
	byte sign = (xx >> 1) & 0b10;
	offset_t val = xx >> 3;
	ASSERT(val);
	val -= (sign * val);
	return val;
}

static inline Thing* get_thing(Thing x)
{
	return (Thing*)x;
}

static inline termid get_term(Thing x)
{
	return (termid)databits(x);
}


//Thing::Measurement
static inline size_t get_size(Thing x)
{
	return (size_t)((uintptr_t)x >> 2);
}


//Thing::Comparison
//Could use != operator
static inline bool types_differ(Thing a, Thing b)
{
	return (((long)a ^ (long)b) & 0b11);
}

#define sizes_differ(x, y) (x != y)


//Properties?	Too weak in that it can include non-booleans
//Propositions?	Too weak in that it can include arity != 1
//Decisions?	Too weak in that it can include arity != 1
//Predicates?
//Measurements? Too weak in that it can include non-booleans
//		It would be suitable for numeric functions on the object
//		though.
//Judgements?	Too weak in that it can include non-booleans & arity != 1
	
//Unary boolean functions on the class
static inline bool is_bound(Thing x)
{
	return ((((uintptr_t)x & (uintptr_t)0b11) == 0) && x);
}

static inline bool is_unbound(Thing x)
{
	return x == 0;
}

#define is_var(thing) 		(typebits(thing) 	== 0b00)
#define is_node(thing) 		(typebits(thing) 	== 0b01)
#define is_offset(thing) 	(typebits(thing) 	== 0b10)
#define is_list(thing) 		(typebits(thing) 	== 0b11)

//General application of '=='; ostensibly general comparison
#define are_equal(x, y) (x == y)

//Thing::Access
static inline ThingType get_type(Thing x)
{
	if(is_bound(x))
		return BOUND;
	if(is_unbound(x))
		return UNBOUND;
	if(is_node(x))
		return NODE;
	if(is_list_bnode(x))
		return LIST_BNODE;
	if(is_list(x))
		return LIST;
	ASSERT(is_offset(x));
	return OFFSET;
}

/*******************************************************************/
//</oneword>
#endif //ndef oneword





inline void add_kbdbg_info(Thing &t, Markup markup)
{
	(void) t;
	(void) markup;
#ifdef KBDBG
	t.markup = markup;
#endif
}




//region types, globals, funcs

//Structures of Things

//Stores all the different Things in a rule
typedef vector<Thing> Locals;

//Stores subject/object pairs for the Euler path check.
typedef std::pair<Thing*,Thing*> thingthingpair;
typedef std::vector<thingthingpair> ep_t;


//Functions-types on Things
typedef function<bool()> coro;


//these are the same, why do we differentiate them? because they are logically different
//i guess we just don't get to say anything more specific in the type
//ah..yes
typedef function<bool(Thing*,Thing*)> pred_t;
typedef function<pred_t()> pred_gen;

typedef function<bool(Thing*,Thing*)> rule_t;
typedef function<rule_t()> rule_gen;


typedef function<bool(Thing*,Thing*, Thing*)> join_t;
typedef function<join_t()> join_gen;
//btw im using gen in the sense that its a lambda generating another lambda
typedef function<join_gen(nodeid, join_gen, pos_t , pos_t , Locals&)>  join_gen_gen;



//Permutations:

//this tells a join coro where to take the parameters to pass to a pred coro
//The subjects/objects in the body of a rule can be of 4 different types:
//HEAD_S: the subject of the head
//HEAD_O: the object of the head
//LOCAL:  a variable in the body but not in the head (local variable?)
//CONST:  a constant
enum PredParam {HEAD_S, HEAD_O, LOCAL, CONST};
//and these are the specialized join coros
//Maps subject predparam & object predparam pair to a specialized join_gen_gen
//for that combination.
typedef map<PredParam, map<PredParam, join_gen_gen>> Perms;
//This is filled in from a list of functions of type join_gen_gen in perms.cpp a
//s the first step of compilation of the kb, with the function: 
//make_perms_table()
Perms perms;
//This is filled out by the same make_perms_table() function with the string 
//versions of the PredParam enum literals.
map<PredParam, string> permname;




map<nodeid, vector<rule_t>> builtins;



map<string, string> log_outputString;

rule_t make_wildcard_rule(nodeid p);






//this is an intermediate structure before compilation
//need to come up with better names, maybe rule_t would be rule_coro?
struct Rule
{
	//Structure
	pquad head;	//Single quad as head/consequent
	pqlist body;	//List of quads as body/antecedents

	//Constructor
	//Standard constructor:
	Rule(pquad h, pqlist b):head(h), body(b){};
} ;

//Structure on rules
//maps preds to the list of rules where they are used in the head.
typedef map<nodeid, vector<Rule>> Rules;

//Globals
Rules rules;
Rules lists; // rules + query


//The structure holding the compiled KB:
#ifdef DEBUG
std::map<nodeid, pred_t> preds;
typedef map<nodeid, pos_t> locals_map;
#else
std::unordered_map<nodeid, pred_t> preds;
typedef unordered_map<nodeid, pos_t> locals_map;
#endif


//what does this represent
typedef map<nodeid, vector<pair<Thing, Thing>>> ths_t;



std::vector<ep_t*> eps_garbage;
vector<Locals*> consts_garbage;
vector<Locals*> locals_templates_garbage;
ths_t * ths_garbage;

//counters
//what is this measuring?
long steps = 0;

//number of unify coros made
//unifys are made on the fly when a unification needs to be done, thus
//counting the number of unify coros made counts the number of unifications
//done (attempted)
long unifys = 0;


//some forward declarations
coro unbound_succeed(Thing *x, Thing *y, Thing * origa, Thing * origb);
coro unify(Thing *, Thing *);
void check_pred(nodeid pr);
rule_t seq(rule_t a, rule_t b);
rule_t compile_rule(Rule r);
void build_in_rules();
void build_in_facts();

//endregion



//region succeed and fail



//yield once
static coro gen_succeed()
{
	EEE; //char entry = 0;
	return [entry]() mutable{
		switch(entry)
		{
		case 0:
			entry = LAST;
			return true;
		case_LAST:
			END
		}
	};
}

//For consistency would this be gen_succeed_with_args?
static join_t succeed_with_args()
{
	EEE; //char entry = 0;
	return [entry](Thing *Ds, Thing *Do, Thing* _) mutable{
		//why (void) these?
		(void)Ds;
		(void)Do;
		(void)_;
		switch(entry)
		{
		case 0:
			entry = LAST;
			steps++;
			return true;
		case_LAST:
			END
		}
		/*
		Macros resolve to:
		case 0:
			entry = 33;
			return true;
		case 33:
			{entry = 66; return false}; default: assert(false); 

		*/
	};
}

//Returns a function that returns the succeed_with_args function
//Wouldn't this really be gen_gen_succeed_with_args?

static join_gen succeed_with_args_gen()
{
	FUN;
	return []() {
		return succeed_with_args();
	};
}

#ifndef DEBUG

static bool fail()
{
	setproc("fail");
	TRACE(dout << "..." << endl;)
	return false;
}

static bool fail_with_args(Thing *_s, Thing *_o)
{
	(void)_s;
	(void)_o;
	setproc("fail_with_args");
	TRACE(dout << "..." << endl;)
	return false;
}

#define GEN_FAIL (fail)
#define GEN_FAIL_WITH_ARGS (fail_with_args)
#define UNIFY_FAIL(a,b) GEN_FAIL
#else

coro dbg_fail()
{
	byte entry = 0;
	return [entry]() mutable{
		setproc("dbg_fail lambda");
		TRACE_ENTRY;

		switch(entry)
		{
		case 0:
			entry = 66;
			return false;
		default:
			ASSERT(false);
		}
	};
}

pred_t dbg_fail_with_args()
{
	int entry = 0;
	return [entry](Thing *_s, Thing *_o) mutable{
		setproc("dbg_fail_with_args lambda");
		TRACE_ENTRY;

		(void)_s;
		(void)_o;

		switch(entry)
		{
		case 0:
			entry = 1;
			return false;
		default:
			ASSERT(false);
		}
	};
}

#define GEN_FAIL (dbg_fail())
#define GEN_FAIL_WITH_ARGS (dbg_fail_with_args())



#ifdef KBDBG

//Thing::Serializer::KBDBG
string kbdbg_str(const Thing * x)
{
	stringstream o;
	o << "[" << "\"" << x << "\""  << ", ";
	pos_t c=0;
	for (auto i: x->markup) {
		o << i;
		if (++c != x->markup.size())
			o << ", ";
	}
	o << "]";
	//assert(c);
	return o.str();
}


coro kbdbg_unify_fail(const Thing *a, const Thing *b)
{
	int entry = 0;
	return [entry, a, b]() mutable{
		switch(entry)
		{
		case 0:
			entry = 1;
			dout << "{\"type\":\"fail\", \"a\":" << kbdbg_str(a) << ", \"b\":" << kbdbg_str(b) << "}" << endl;
			return false;
		default:
			ASSERT(false);
		}
	};
}

#define UNIFY_FAIL(a,b) kbdbg_unify_fail(a,b)
#else
#define UNIFY_FAIL(a,b) GEN_FAIL


#endif // kbdbg
#endif // debug


//Not sure what this function does
inline void kbdbg_bind(const Thing *a, bool bind, const Thing *b)
{
	(void)a;
	(void)b;
	(void)bind;
//why isn't this surrounding the whole function?
#ifdef KBDBG
	dout << "{\"type\":\"";
	if(!bind) dout << "un";
	dout << "bind\", \"a\":" << kbdbg_str(a) << ", \"b\":" << kbdbg_str(b) << "}" << endl;
#endif
}


#ifndef KBDBG

#define UNIFY_SUCCEED(a,b) gen_succeed()

#else

//Not sure what this function does
static coro UNIFY_SUCCEED(const Thing *a, const Thing *b)
{
	EEE; //case entry = 0;
	return [entry, a ,b]() mutable{
		switch(entry)
		{
		case 0:
			entry = LAST;
			kbdbg_bind(a, true, b);
			return true;
		case_LAST:
			kbdbg_bind(a, false, b);
			entry = 66;
			return false;
		default:
			assert(false);
		}
	};
}


#endif


//endregion



//Thing::Serializer
//potentially suitable for tail-recursion optimization
string str(const Thing *_x)
{
	Thing x = *_x;
	switch (get_type(x)) {
		case BOUND: {
			const Thing *thing = get_thing(x);
			ASSERT(thing);
			return "var(" + str(thing) + ")";
		}


		case UNBOUND:
			return "var()";


		case NODE: {
			const nodeid node = get_node(x);
			ASSERT(node);
			return *dict[node].value;
		}


		case LIST_BNODE: {
			const nodeid node = get_node(x);
			ASSERT(node);
			return "~" + *dict[node].value + "~";
		}


		case LIST: {//btw with list bnodes, LIST Thing is just an optimization now
			const size_t size = get_size(x);
			stringstream r;
			r << "{" << size << " items}(";
			for (pos_t  i = 0; i < size; i++) {
				if (i != 0) r << " ";
				//not sure what this is doing; iterating over the list i guess, i just dont quite get the mechanism
				/*we are skipping the bnodes when printing the list, so this is
				 * the address of the LIST, its followed by first LIST_BNODE,
				 * then first actual value in the list, so +2 to get to the value,
				 * then jumping to i-th value(*2)*/
				r << str(_x + 2 + (i*2));
			}
			if (!size)
				r << " ";
			return r.str() + ")";
		}



		case OFFSET: {
			const offset_t offset = get_offset(x);
			stringstream r;
			r << "<offset ";
			if (offset >= 0)
				r << "+";
			r << offset << ">->";
			r << str(_x + offset);
			return r.str();
		}
	}
	ASSERT(false);
}


//Thing::Access
//not sure what this does
static Thing *getValue (Thing *_x) __attribute__ ((pure));

//This seems to be suitable for tail-recursion optimization.
static Thing *getValue (Thing *_x)
	/*
		# If this Variable is unbound, then just return this Variable.^M
		# Otherwise, if this has been bound to a value with unify, return the value.^M
		# If the bound value is another Variable, this follows the "variable chain"^M
		# to the end and returns the final value, or the final Variable if it is unbound.^M
		# For more details, see http://yieldprolog.sourceforge.net/tutorial1.html^M
		def getValue(self):^M
			if not self._isBound:^M
				return self^M

			result = self._value^M
			while isinstance(result, Variable):^M
				if not result._isBound:^M
					return result^M

				# Keep following the Variable chain.^M
				result = result._value^M

			return result^M
	*/
{

	ASSERT(_x);

	Thing x = *_x;

	//Is a bound variable, return the value of it's value.
	if (is_bound(x)) {
		//get the pointer
		Thing * thing = get_thing(x);
		ASSERT(thing);
		//and recurse
		return getValue(thing);
	}

	//Need to understand this whole offset thing better
	else if (is_offset(x))
	{
		//Thing of type offset is used for the 2nd or later occurrence
		// of a local variable in a
		//rule; it will store a value offset of type offset_t. 

		////This is the offset from the pointer to the Thing representing 
		////this instance of a local variable to the pointer to it's 
		////"representative", which will be labeled either bound or 
		////unbound.
		
		//its an offset from the address of the offset to where the 
		//value is
		
		//get the number
		const offset_t offset = get_offset(x);
		//add it to the current address
		Thing * z = _x + offset;
		
		
		//Why do we bind here? We already have _x as offset to z
		//this is an attempt at optimization so that the second time
		//we look at this, it will be a variable, which should be 
		//followed faster than the offset
		make_this_bound(_x, z);
		
		//and recurse
		return getValue(z);
	}
	//Is either an unbound variable or a value.
	else
		return _x;
}






/*  # If this Variable is bound, then just call YP.unify to unify this with arg.
	# (Note that if arg is an unbound Variable, then YP.unify will bind it to
	# this Variable's value.)
	# Otherwise, bind this Variable to YP.getValue(arg) and yield once.  After the
	# yield, return this Variable to the unbound state.
	# For more details, see http://yieldprolog.sourceforge.net/tutorial1.html */


//Thing::Update
//Thing::Unify
//coro; will change
//We know that 'me' will be an unbound var, and
//arg could be anything.
//should this have gen_ prefix?
//gen_unbound_uc?
coro unboundunifycoro(Thing * me, Thing *arg
#ifdef DEBUG
, Thing * origa, Thing * origb
#endif
)
{
		FUN;
		TRACE(dout << "!Bound" << endl;)
		Thing *argv = getValue(arg);
		TRACE(dout << "unify with [" << argv << "]" << str(argv) << endl;)

		//How do we end up unifying an unbound variable with itself?
		//Won't the variables that come in as the arguments necessarily
		//be different from the variables in rules that they will
		//potentially unify with? 
		/*dunno, you can look for where it comes up in traces
		{?x a man} => {?x a mortal}.
		fin.
		?x a mortal.
		fin.
		
		*/
		if (argv == me) {
			TRACE(dout << "argv == me" << endl;)
			//# We are unifying this unbound variable with itself, so leave it unbound.^M
#ifdef DEBUG
			//gen_unbound_succeed
			return unbound_succeed(me, argv, origa, origb);
#else
			//why gen_succeed(); vs succeed;?
			//because succeed needs to define entry externally
			//to itself and then capture it in to use for the 
			//coro switch. This allows entry to escape gen_succeed
			//when it returns the succeed coro, and will, in a
			//sense, exist outside of normal variable space, because
			//it will no longer be local to any function, nor will
			//it be global, but it will be accessible by the
			//succeed coro, and will in this sense be bound to it. 
			return gen_succeed();
#endif
		}
		else {
			TRACE(dout << "!= me" << endl;)

			EEE; //char entry = 0;
			return [me, entry, argv
			#ifdef DEBUG
			, origa, origb
			#endif
			]() mutable {
				setproc("var unify lambda");
				//TRACE(dout << "entry = " << (int)entry << endl)
				TRACE_ENTRY; //TRCENTRY
				switch (entry) {
				//A 2-step process:
				//on the first time around it will bind me
				//to argv:
				// me() -> me(argv)
				//why the brackets surrounding?
					case 0: {
						TRACE(dout << "binding [" << me << "]" << str(me) << " to [" << argv << "]" << str(argv) << endl;)
						ASSERT(is_unbound(*me));
						make_this_bound(me, argv);
						#ifdef KBDBG
						kbdbg_bind(origa, true, origb);
						#endif
						entry = LAST;
						return true;
					}
				//on the second time around it will unbind it,
				//and that's supposed to be it:
				//me(argv) -> me()
					case_LAST:
						TRACE(dout << "unbinding [" << me << "]" << str(me)/* << " from [" << argv << "]" << str(argv)*/ << endl;)
						//argv shouldnt be touched here anymore, it could be a gone constant on stack
						ASSERT(is_bound(*me));
						make_this_unbound(me);
						#ifdef KBDBG
						kbdbg_bind(origa, false, origb);
						#endif
				//with DEBUG, we check for reentry which would be a bug
						END
				}
			};
		}
	}


//Thing::Comparison
bool would_unify(Thing *this_, Thing *x_)
/*ep check is supposed to determine equality by seeing if the values would unify
* (but not actually doing the unifying assignment)*/
{
		FUN;
		const Thing me = *this_;
		const Thing x = *x_;
		//We're sure it won't be these? in the context of ep-checking, yes
		ASSERT(!is_offset(me));
		ASSERT(!is_offset(x));
		ASSERT(!is_bound(x));

		if (is_var(me))
			return true;// we must have been an unbound var
		else if (types_differ(me, x)) // in oneword mode doesnt differentiate between bound and unbound!
			return false;
		else if(is_node(me))
			return  are_equal(me, x);
		else
		{
			ASSERT(is_list(me));
			const auto size = get_size(me);
			if(size != get_size(x)) return false;
			for (pos_t  i = 0; i < size; i++)
				if (!would_unify(getValue(this_+1+(i*2)) , getValue(x_+1+(i*2))))
					return false;
			return true;
		}
		//dont list bnodes ever come up here?

	/*maybe we could do this function more functionally like return type_bits(me) &&...*/
}



#ifdef DEBUG

//#ifdef DEBUG is enough to ensure that origa and origb will be there?
//from (gen_)unboundunifycoro it's good.
//that's the only place it's called from; alright.
//is this just a more verbose, assertive, & argument-taking version of gen_succeed()?
//The only place this is called from is the (me == argv) case in
//(gen_)unboundunifycoro, so apparently we should have (x == y)

//gen_unbound_succeed
coro unbound_succeed(Thing *x, Thing *y, Thing * origa, Thing * origb)
{
	EEE;
	return [entry, x, y, origa, origb]() mutable {
		ASSERT(is_unbound(*x));
		setproc("unbound_succeed lambda");
		TRACE(dout << str(x) << " " << str(y) << endl);
		TRACE_ENTRY;
		switch (entry) {
			case 0:
				entry = LAST;
				kbdbg_bind(origa,true,origb);
				return true;
			case_LAST:
				kbdbg_bind(origa,false,origb);
				entry = 66;
				return false;
			default:
				assert(false);
		}
	};
}
//mm and if DEBUG is not defined?
//unboundunifycoro only needs it when DEBUG is defined.
#endif





//region sprint

//Thing::Serializer::Decorated
//Var::Serializer::Decorated
string sprintVar(string label, Thing *v){
	stringstream wss;
	wss << label << ": (" << v << ")" << str(v);
	return wss.str();
}

//Pred::Serializer::Decorated
string sprintPred(string label, nodeid pred){
	stringstream wss;
	wss << label << ": (" << pred << ")" << dict[pred];
	return wss.str();
}

//Thing::Serializer::Decorated
string sprintThing(string label, Thing *t){
	stringstream wss;
	wss << label << ": [" << t << "]" << str(t);
	return wss.str();
}


string sprintSrcDst(Thing *Ds, Thing *s, Thing *Do, Thing *o){
	stringstream wss;
	wss << sprintThing("Ds", Ds) << ", " << sprintThing("s",s) << endl;
	wss << sprintThing("Do", Do) << ", " << sprintThing("o",o);
	return wss.str();
}


//endregion







//region kb

//Garbage zone
/*
What kind of garbage do we have?

 * eps_garbage				vector<ep_t*>
 * consts_garbage			vector<Locals*>
 * locals_templates_garbage		vector<Locals*>


 typedef vector<Thing> Locals;
 typedef std::pair<Thing*,Thing*> thingthingpair;
 typedef std::vector<thingthingpair> ep_t;
*/

/*
Where does this garbage come from? Allocation of objects
*/
void free_garbage()
{
	for (auto x: eps_garbage)
	{
		ASSERT(!x->size());
		delete x;
	}
	eps_garbage.clear();

	for (auto x: consts_garbage)
		delete x;
	consts_garbage.clear();
	for (auto x: locals_templates_garbage)
		delete x;
	locals_templates_garbage.clear();

}


void free_garbage_nonassert()
{
	for (auto x: eps_garbage)
		delete x;
	eps_garbage.clear();
}







void add_rule(nodeid pr, const rule_t &x)
{
	//If the nodeid is not already in preds then make this
	//rule_t (pred_t?) its pred function. 
	if (preds.find(pr) == preds.end())
		preds[pr] = x;
	//Otherwise, sequence this pred function with what's
	//already in preds.
	else {
		TRACE(dout << "seq, nodeid: " << pr << "(" << dict[pr] << ")" << endl;)
		preds[pr] = seq(x, preds[pr]);
	}
}




//Check preds to see if the pred has already been compiled.
//Don't compile preds that have already been compiled. Return instead.
//Note: we shouldn't even be getting the same pred twice anyway

//Check builtins to see if the pred is a built-in. If we do find that
//node-id, then do add_rule for each pred_t in the associated
//std::vector<pred_t> in the builtins table. Return unless the
//builtin pred is 'rdftype'.

//If it was rdftype or is not in the builtins table, then check to
//see if it's in pred_index. If it's not, then return, otherwise,
//get the list of rules in which this node is the predicate for the
//head. For each rule in the list, compile the rule and add it to the
//rule for that predicate using add_rule.

//add_rule is simple, it just checks to see if the pred has already been
//compiled, if not, then the rule_t we pass it becomes the rule_t for
//that pred. If the pred is already partially compiled, then the
//rule_t we pass it just gets sequenced with what's already there (using seq).
void compile_pred(nodeid pr)
{
	FUN;
	TRACE(dout << dict[pr] << endl;)

	//we already did it?
	if (preds.find(pr) != preds.end())
		return;

	//builtins are ready to go
	if (builtins.find(pr) != builtins.end()) {
		for (auto b: builtins[pr]) {
			TRACE(dout << "builtin: " << dict[pr] << endl;)
			add_rule(pr, b);
		}
		/*//lets not shadow rdftype by builtins for now
		if (pr != rdftype && pr != rdfssubPropertyOf)
			return;*/
	}

	//rules need to be compiled and then added:
	if (rules.find(pr) != rules.end()) {
		for (auto r: rules.at(pr))
			add_rule(pr, compile_rule(r));
	}
	
	//rdfs:SubPropertyOf
	add_rule(pr, make_wildcard_rule(pr));

}



void check_pred(nodeid pr)
{
	FUN;
	if (rules.find(pr) == rules.end() && builtins.find(pr) == builtins.end()) {
		dout << "'" << dict[pr] << "' not found." << endl;
		preds[pr] = make_wildcard_rule(pr);// GEN_FAIL_WITH_ARGS;//wildcard_pred(pr);//
	}
 }



//Let's get a graphical visualization of this transformation.
Rules quads2rules(qdb &kb)
{
	FUN;
  //typedef map<nodeid, vector<Rule>> Rules;
	Rules result;

	//std::map<context,list of quads with that context>
	//std::map<string,pqlist>
	auto &quads = kb.first;

	//why only default context? because thats what we reason about

	auto it = quads.find(str_default);
	if (it != quads.end()) {
		//pqlist
		auto pffft = *it->second;

		//reverse the list of quads for this pred.
		//is it going now opposite to text-order?
		reverse(pffft.begin(), pffft.end());
		//pqlist
		for (pquad quad : pffft) {

			const string &s = *quad->subj->value, &p = *quad->pred->value, &o = *quad->object->value;
			TRACE(dout << quad->tostring() << endl);

			//here we add a fact, one for each triple in @default
			//typedef map<nodeid, vector<Rule>> Rules;
			result[dict[quad->pred]].push_back(Rule(quad, 0));

			if (p == implication) //then also, if this is a rule, i.e. the predicate is "=>":
			{
//hopefully the subject and object are graphs
				if (quads.find(o) != quads.end()) {
//the object of the implication is the head of the rule
					//Explode the head into separate quads and make a separate
					//rule for each of them with the same body
//because from now on our rules must only have one triple for their head

					//look for the body graph
					pqlist b = 0;
					if (quads.find(s) != quads.end())
						b = quads[s];

					//make a rule for each triple in the head
					for (pquad h : *quads.at(o)) {
						result[dict[h->pred]].push_back(Rule(h, b));
					}
				}
			}
		}
	}	
	return result;
}


void compile_kb(qdb &kb)
{
	FUN;
	//Cleanup
	//DEBUG:      std::map<nodeid, pred_t> preds;
	//otherwise:  std::unordered_map<nodeid, pred_t> preds;
	preds.clear();//the lambdas
	free_garbage();

	//These are globals of type Rules
	//typedef map<nodeid, vector<Rule>> Rules;
	rules = quads2rules(kb);
	lists = rules;//we dont have any query at this point
/*maybe this includes also lists in heads, while it should only have
lists in the default graph?*/

	for(auto x: builtins)
		compile_pred(x.first);

	for(auto x: rules)
		compile_pred(x.first);
}



//endregion











//region coros





coro unifjoin(Thing *a, Thing *b, coro c)
{
	FUN;
	TRACE(dout << "..." << endl;)
	EEE;
	coro uc;
	TRC(int call = 0;)
	return [a,b,c, uc, entry TRCCAP(call)]() mutable{
		setproc("unifjoin1");
		TRACE_ENTRY;
		TRC(call++;)

		switch(entry)
		{
		case 0:
			uc = unify(a,b);
			entry = LAST;
			while(uc()){
				ASSERT(call == 1);
				while(c()){
					ASSERT(call == 1);
					return true;
		case_LAST:;
					ASSERT(call == 2);
				};
			}
			END;
		}
	};
}



coro listunifycoro(Thing *a_, Thing *b_)
{
	FUN;

	const Thing a = *a_;
	const Thing b = *b_;

	//TRACE(dout << str(a_) << " X " << str(b_) << endl;)

	ASSERT(is_list(a));
	ASSERT(is_list(b));

	//gotta join up unifcoros of vars in the lists
	if(sizes_differ(a,b))
		return GEN_FAIL ;

	coro r = gen_succeed();

	for(int i = get_size(b)-1;i >= 0; i--)
	{
		r = unifjoin(a_+2+(i*2), b_+2+(i*2), r);
	}

	return r;
}


coro listunifycoro2(Thing *a_, Thing *b_)
{
	FUN;

	Thing a = *a_;
	Thing b = *b_;

	TRACE(dout << str(a_) << " X " << str(b_) << endl;)

	ASSERT(is_list_bnode(a));
	ASSERT(is_list_bnode(b));

	coro r = gen_succeed();


	while(true){
	auto i = a_ + 1;
	auto j = b_ + 1;

	r = unifjoin(i, j, r);
	
	a_ +=2;
	b_ +=2;
	a = *a_;
	b = *b_;
	
	if(get_node(a) == rdfnil && get_node(b) == rdfnil)
		return r;
	else if(get_node(a) == rdfnil || get_node(b) == rdfnil)
		return GEN_FAIL ;
	}
}
/*
	# If arg1 or arg2 is an object with a unify method (such as Variable or^M
	# Functor) then just call its unify with the other argument.  The object's^M
	# unify method will bind the values or check for equals as needed.^M
	# Otherwise, both arguments are "normal" (atomic) values so if they^M
	# are equal then succeed (yield once), else fail (don't yield).^M
	# For more details, see http://yieldprolog.sourceforge.net/tutorial1.html^M
	(returns an iterator)
*/

/*
in cppout it would be like:


int will_unify(

*/
coro unify(Thing *a_, Thing *b_){
	FUN;
	unifys++;

	//for logging
	DBG(Thing *origa = a_;)
	DBG(Thing *origb = b_;)


	TRACE(dout << str(a_) << " X " << str(b_) << endl;)


	if (a_ == b_) {//?
		TRACE(dout << "a == b" << endl;)

    	//orig, origb only exist #ifdef DEBUG
		//ifndef KBDBG then this will be gen_succeed();
		return UNIFY_SUCCEED(origa, origb);
	}

	//Get the "representative value" of a_
	a_ = getValue(a_);
	Thing a = *a_;

	//Should be the representative value, which will either be an
	//unbound variable or a literal, i.e. not a bound variable and
	//not an offset to a variable.
	ASSERT(!is_bound(a));
	ASSERT(!is_offset(a));


	//If a is an unbound variable	
	if (is_unbound(a))
		return unboundunifycoro(a_, b_
		#ifdef DEBUG
		,origa, origb
		#endif
		);


	b_ = getValue(b_);
	Thing b = *b_;

	//Should be the representative value, so unbound or literal.
	ASSERT(!is_bound(b));
	ASSERT(!is_offset(b));

	//a: literal; b: unbound variable
	//(val)?x
	//(list)?x	

	//only on this one is there order switching.
	if (is_unbound(b))
		return unboundunifycoro(b_, a_
		#ifdef DEBUG
		,origb, origa
		#endif
		);

	//Neither a nor b can be any of:
	//*bound variable	BOUND
	//*offset variable	OFFSET
	//*unbound variable	UNBOUND

	//Both a and b can be any of:
	//*node			NODE
	//*list			LIST
	//*list_bnode		LIST_BNODE

	//Check if they are the same.
	//If they don't have a size then this reduces to false==false right?
	//(a.type == b.type && a.size == b.size)
	if(are_equal(a,b)) {
		//Since are_equal succeeded we know they both have the same
		//type so we only need to do is_node/list for one of them.
		//If they are nodes:
		if (is_node(a)) {
		//why origa and origb?
		//ifndef KBDBG this will be gen_succeed();
			return UNIFY_SUCCEED(origa, origb);
		}
		//If they are lists:
		if (is_list(a)) {
			//TRACE(dout << "Both args are lists." << endl;)
			return listunifycoro(a_, b_);
		}
	}
	
	//Not sure i understand this one.
	if (is_list(a) || is_list(b))
	{
		auto xx = a_;
		auto yy = b_;
		if (is_list(*xx))
			xx += 1;
		if (is_list(*yy))
			yy += 1;
		if (is_list_bnode(*xx) && is_list_bnode(*yy))
			return listunifycoro2(xx,yy);
	}

	//Otherwise fail. Why here?
	TRACE(dout << "Fail. origa:[" << origa << "] origb:[" << origb << "] a:["<< a_ << "]" << str(a_) << " b:[" << b_ << "]" << str(b_) << endl;)
	return UNIFY_FAIL(origa, origb);
}

rule_t seq(rule_t a, rule_t b){
	FUN;
	TRACE(dout << ".." << endl;)
	EEE; //char entry = 0;
	TRC(int call = 0;)
	return [a, b, entry TRCCAP(call)](Thing *Ds, Thing *Do) mutable{
		setproc("seq1");
		TRC(call++;)
		TRACE(dout << "call: " << call << endl;)

		switch(entry){
		case 0:
			entry = 1;
			while(a(Ds, Do)){
				TRACE(dout << "MATCH A." << endl;)
				return true;
		case 1: ;
			}
			entry = LAST;
			while(b(Ds, Do)){
				TRACE(dout << "MATCH B." << endl;)
				return true;
		case_LAST: ;
			}
			TRACE(dout << "SWITCH DONE." << endl;)
			END
		}
		TRACE(dout << "Why are we here?" << endl;)
		ASSERT(false);
	};
}


/*
one join function, joins "just pass" query s and o down between them,
each join calls one pred,
so we are only concerned with permuting the two params to the pred,
and these can be either: s, o, or a local var, or a const
a join captures two indexes into the locals/consts table, which it may or may not use
*/


#include "perms.cpp"



PredParam maybe_head(PredParam pp, pquad head, nodeid x)
{
#ifndef KBDBG
	if (head) {
		assert(head->subj && head->object);
		if (x == dict[head->subj])
			return HEAD_S;
		if (x == dict[head->object])
			return HEAD_O;
	}
#endif
	return pp;
}

//Locals::Access
//return term's PredParam and possibly also its index into the corresponding vector
PredParam find_thing (nodeid x, pos_t  &index, locals_map &lm, locals_map &cm)
{
	auto it = lm.find(x);
	if (it != lm.end()) {
		index = it->second;
		return LOCAL;
	}
	else {
		index = cm.at(x);
		return CONST;
	}
}

#ifdef KBDBG
PredParam kbdbg_find_thing (unsigned long part, pos_t  &index, Locals &locals)
{
	pos_t  r = 0;
	for(auto i: locals)
	{
		if (i.markup.size() == 1 && i.markup.at(0) == part) {
			index = r;
			return LOCAL;
		}
		r++;
	}
	assert(false);
}
#endif


//find thing in locals or consts
Thing &fetch_thing(nodeid x, Locals &locals, Locals &consts, locals_map &lm, locals_map &cm)
{
	pos_t  i;
	auto pp = find_thing(x, i, lm, cm);
	if (pp == LOCAL)
		return locals[i];
	else if (pp == CONST)
		return consts[i];
	else
		assert(false);
}




//Locals::Serializer
void print_locals(Locals &locals, Locals &consts, locals_map &lm, locals_map &cm, pquad head)
{
	(void)head;
	dout << endl << "locals map: \n nodeid\t\tpos\t\tthing\t\tkbdbg" << endl;
	for (auto x: lm)
	{
		dout << " " << *dict[x.first].value << "\t\t";
		dout << x.second << "\t\t";
		dout << str(&locals.at(x.second));
#ifdef KBDBG
		"\t\t" << kbdbg_str(&locals.at(x.second))  srtsrt
#endif
		dout << endl;
	}
#ifdef KBDBG
	dout << "locals: (pos, thing, kbdbg)" << endl;
	for (pos_t  i = 0; i<locals.size(); i++)
		dout << " " << i << "     " << str(&locals.at(i)) << "     " << kbdbg_str(&locals.at(i)) << endl;
#endif
	if (cm.size()) {
		dout << "consts:" << endl;
		for (auto x: cm)
			dout << *dict[x.first].value << " : : " << x.second << "  --> " << str(&consts.at(x.second)) << endl;
	}
}


/*the two funcs look into Rules lists
those should contain the kb triples and possibly also the query triples*/
bool islist(nodeid x)
{
	FUN;
	//hrmm
	//For each Rule in lists[rdfrest]:
	//for all rdfrest triples

	//for all rules with rdfrest as the pred in the head
	for (auto i: lists[rdfrest]) {
		//pquad
		auto h = i.head;
		TRACE(dout << h->subj->tostring() << " vs " << dict[x].tostring() << endl);

		//if our nodeid is a subject of rdfrest, its a list bnode
		if (dict[h->subj] == x)
				return true;
	}
	return false;
}

//Locals::Constructor (Partial)
/*the first nodeid in the map is the bnode*/
/*the second nodeid in the map is the object related to this bnode by
  rdffirst*/
//This map keeps the content of the list but loses the ordering; essentially
//turns it into a set.
map<nodeid,nodeid> get_list(nodeid n) {
	FUN;
	ASSERT(n);

	map<nodeid,nodeid>  r;

	while(true) {
		//Find the rule where this bnode is the subj of rdffirst:
		for (auto rule: lists[rdffirst]) {
			//node -> nodeid
			if (dict[rule.head->subj] == n) {
				//hrmm, b0 rdffirst rdfnil ?
				//returns an empty r
				if (dict[rule.head->object] == rdfnil)
					return r;

				//Map the bnode to the object of the rdffirst
				//triple that it appears in.
				r[n] = dict[rule.head->object];

		//Find the rule where this bnode is the subj of rdfrest:
				for (auto rule: lists[rdfrest]) {
					if (dict[rule.head->subj] == n) {
		//If the object of the triple is rdfnil, we return a one-pair
		//map.
						if (dict[rule.head->object] == rdfnil)
							return r;
		//Otherwise set n to the node of the object, which should
		//be a continuation of this list.
		//Pseudo-recursion induced by the outer while loop and
		//switching out n. Taken as a recursive function it
		//implements its own tail-recursion optimization.
						n = dict[rule.head->object];
						break;
					}
				}
			//Should probably have another break here?
			//I guess the idea of leaving it is that it if the
			//new node came after this node in the list, then
			//by not breaking here we continue to traverse the
			//list and don't have to start from the beginning,
			//and if it's not there then we start over and it'll
			//be in the beginning.
			}
		}
	}
	//It should have no capacity to reach this point
}





//think {?a x ?a. (?a) x b} => b x b
//locals: var (thats the ?a) | list header (size 1) | offset - 2 (pointing to the first var)  | nil 
//consts:  b (node - a constant)
//a const in a list wont go into consts but stay in the list:
//{?a x ?a. (?a b) x b} => b x b 
//locals: var (thats the ?a) | list header (size 2) | offset - 2 (pointing to the first var) | list bnode ("size" 1) | node b | nil 
//consts: the node b


//Locals::Constructor
//Input: head, body
//To fill out: locals, consts, lm, cm
void make_locals(Locals &locals, Locals &consts, locals_map &lm, locals_map &cm, pquad head, pqlist body)
{
	FUN;

  //queue to load up our lists into.
	std::queue<toadd> lq;
  //	TRACE(dout << "head:" << op->format(head) << endl);

	/* Function definition region */
	//We make 3 function which we then apply later, so in reading this make_locals function,
	//treat this as if they're just some functions and skip past them until needed.




	//As long as there's still terms in lq, pop the 1st one off the list and 
	auto expand_lists = [&lq, &locals, &lm]() {
		setproc("expand_lists");
		while (!lq.empty()) {
			//Pop the first node off of lq into ll.
			toadd ll = lq.front();
			lq.pop();

			//Grab the nodeid from the toadd.
			nodeid l = ll.first;

			//First item: bnode
			//2nd item: object related to that bnode by
			//rdffirst.
			//map<nodeid,nodeid> get_list(nodeid n)
			auto lst = get_list(l);

			//Make a thing
			Thing i0; // list size item, a sort of header / pascal string (array?) style thing
#ifdef KBDBG
			//Add the markup from the toadd to the thing.
			add_kbdbg_info(i0, ll.second);

			//What's this do
			unsigned long list_part = 0;
#endif
			//Why the size?

			//This will make the value of the Thing i0 the
			//number of items in the list, and put it in locals.
			//lm[l] = locals.size();
			make_this_list(i0, lst.size());
			locals.push_back(i0);
			lm[l] = locals.size()-1; // register us in the locals map

			//For each item in the list,
			for (auto list_item: lst) {
				nodeid bnode_id = list_item.first;
				nodeid li = list_item.second;
				TRACE(dout << "item..." << dict[bnode_id] << " : " << dict[li] << endl;)

#ifdef KBDBG

				Markup m = ll.second;
				m.push_back(list_part++);
#endif
				//Create a Thing for the bnode of the item
				//and put it in locals.

				//we add bnodes that simulate rdf list structure
				Thing bnode = create_list_bnode(bnode_id);
				locals.push_back(bnode);


				//Create a Thing for the list item and put
				//it in locals.
				Thing t;
				if (li < 0) {
					TRACE(dout << "its a var" << endl);
					auto it = lm.find(li); //is it already in locals?
					if (it == lm.end()) { //no? 
						MSG("create a fresh var")
						t = create_unbound();
						lm[li] = locals.size();
					}
					else { //yes? just point to it
					//hrmm
						make_this_offset(t, ofst(it->second, locals.size()));
					}
				}
				else { //its a node
					t = create_node(li);
					if (islist(li))
						#ifdef KBDBG
						lq.push(toadd(li, m));
						#else
						lq.push(toadd(li, {}));
						#endif
				}

#ifdef KBDBG
				add_kbdbg_info(t, m);
#endif
				locals.push_back(t);

			}

			//final nil
			Thing nil = create_node(rdfnil);
			locals.push_back(nil);

		}
	};

	//replace NODEs whose terms are lists with OFFSETs. expand_lists left them there.
	auto link_lists = [&locals, &lm]() {
		//For each Thing in locals:
		for (pos_t  i = 0; i < locals.size(); i++) {
			Thing &x = locals[i];
			if (is_node(x) && islist(get_node(x))) {
				make_this_offset(x, ofst(lm.at(get_node(x)), i));
			}
		}
	};


	//typedef map<nodeid, pos_t> locals_map;
	//typedef vector<Thing> Locals;

	//Why do we pass var to this when we pass xx to it and we could
	//just check x < 0
	//make a Thing out of our node (toadd) xx
	auto add_node = [](bool var, toadd xx, Locals &vec, locals_map &m) {
		setproc("add_node");

		//Make a blank Thing 
		Thing t;

		//add the Markup from xx to t.
		add_kbdbg_info(t, xx.second);

		//Get the nodeid of our toadd		
		nodeid x = xx.first;
//		TRACE(dout << "termid:" << x << " p:" << dict[x->p] << "(" << x->p << ")" << endl;)

		//Check to see if the termid is already in the map (locals/consts).
		//If it's not then add it.
		auto it = m.find(x);
		if (it == m.end()) {
			//This will give the position in vec that the Thing
			//will go, and m will map termid's to their position in
			//vec.
			m[x] = vec.size();

			//Create a Thing for this nodeid and push it to the back
			//of vec.
			//If it's a var it'll be unbound ofc. Bound variables
			//only happen during query.
			if(var)
				t = create_unbound();
			//If it's not a var it'll be a node, remember we're
			//not handling lists here.
			else
				t = create_node(x);
			
			//add the Markup from xx to t.
			//mm I think we did that already.
			//yea I think this is redundant.
			//we did it but for a different t
			//anyway, just dont worry about kbdbg
			add_kbdbg_info(t, xx.second);

			//Push the thing into our Locals vec.
			//We should have the equation:
			// t = vec[m[x]]
			vec.push_back(t);
		}
		//Are we normally not expecting the else condition?
//We only make offsets if KBDBG is defined?
//What are we doing if it's not KBDBG and the var's already in there?
//with kbdbg, every single occurence of a var in the rule has to have its
//own representation in locals, we cant just re-use the same position
#ifdef KBDBG
		else
		{
			//hrmm
			make_this_offset(t, ofst(it->second, vec.size()));
			//I think this would also be redundant.
			add_kbdbg_info(t, xx.second);
			vec.push_back(t);
		}
#endif
	};


	/* Execution region */ 

	//typedef vector<unsigned long> Markup;
	//typedef std::pair<nodeid, Markup> toadd;
	//Should call this nodes maybe?
	//We're going to put every node (subj/obj) used by the rule into this
	//vector.
	vector<toadd> terms;


	//Need to understand this kbdbg stuff

	//Store the value of the global before we modify it
	unsigned long old_kbdbg_part = kbdbg_part;

	//Make a toadd for both the subject and object for each term in the 
	//rule (both head & body), and push these into terms vector.
	//Increment kbdbg_part for each node added to terms, place this
	//value into a vector, and set that as the Markup for the toadd.
	//What's the Markup doing?
	if (head) {
		terms.push_back(toadd(dict[head->subj], {kbdbg_part++}));
		terms.push_back(toadd(dict[head->object], {kbdbg_part++}));
	}
	if(body)
	for (pquad bi: *body) {
		terms.push_back(toadd(dict[bi->subj], {kbdbg_part++}));
		terms.push_back(toadd(dict[bi->object], {kbdbg_part++}));
	}
	//TRACE(dout << "terms.size:" << terms.size() << endl);

	//For all our terms (toadds) in terms, if the term is not
	//a variable or a list, then "add_node(false, xx, locals, lm)".
	//If the term is a variable, then "add_node(true, xx, locals, lm)".
	//If it's a list, then push it to lq to be processed later.
	//std::pair<nodeid,Markup>
	for (toadd xx: terms) {
		nodeid x = xx.first;
		//If not a variable & not a list, then we'll make a 'constant' thing, i.e. add_node(false,...)
		//only says it's a list if it's in the head
		//no it definitely says its a list if its in @default
		//hmm
		if (x > 0 && !islist(x)) {

//islist() only tells us its a list if it's in the head?

//what's with consts & cm?
//not sure this whole KBDBG switch going on here
#ifndef KBDBG
			//force rule s and o into locals for now
		//If it's not a var, not a list, and is in the head, then
		//put it in locals. Why? //i guess just so i didnt have to complicate or make permutations of the rule lambda where it unifies the rule arguments against this
		//Why would we have !head? //for query top level
			if (head && (x == dict[head->subj] || x == dict[head->object]))
				add_node(false, xx, locals, lm);
		//If it's not a var, not a list, and is not in the head, then
		//put it in consts. Why?
			else
				add_node(false, xx, consts, cm);
#else
    //And why #ifdef KBDBG they both go into locals?
			add_node(false, xx, locals, lm);
#endif
		}
		//Is a variable, so we'll make a variable thing, i.e. add_node(true,...)
		else if (x < 0)
			add_node(true, xx, locals, lm);

		//Is a list, we'll push it to lq and save it for expand_lists() and link_lists()
		//only says it's a list if it's in the head
		else if (x > 0 && islist(x))
			lq.push(xx);
		else
			assert(false);
	}

	expand_lists();
	link_lists();



  //What are these?
	kbdbg_part_max = kbdbg_part;
	kbdbg_part = old_kbdbg_part;

	TRACE(dout << "kbdbg_part:" << kbdbg_part << ", kbdbg_part_max:" << kbdbg_part_max << endl);
	TRACE(print_locals(locals, consts, lm, cm, head);)
}







join_gen compile_body(Locals &locals, Locals &consts, locals_map &lm, locals_map &cm, pquad head, pqlist body)
{
	FUN;
	join_gen jg = succeed_with_args_gen();
	if(body)
	{

  //qlist
	auto b2 = *body;
	reverse(b2.begin(), b2.end());
#ifdef KBDBG
	auto max = kbdbg_part_max;
#endif

	//For every term in the body, check the pred to see if there's a head
	//with this pred. If not we'll set the pred-function for that pred to
	//fail_with_args. 
	for (pquad bi: b2)
	{
	  //Make sure the pred is there, if not, make a wild-card rule for it. Hrmm
    //Need to think about what order we're compiling our rules/preds in and how
    //this will impact the order of execution.
    //This will have some preds calling the wildcard rule twice, once at the beginning of
    //their execution and once at the end. This adds a wild-card rule if the pred isn't
    //there, but it might only not be there because we just haven't gotten to it yet.
    //When we do get around to it, it will now have a wildcard rule there already. We'll
    //sequence it up with the rest of the rules in compile_pred, and then at the end of
    //compile_pred, we add a wildcard rule onto the beginning.
    //Really there probably shouldn't be a wildcard rule added to the beginning in compile_pred
    //unless there's nothing in the pred at the end of compilation. We should probably do
    //wildcard rule at the end, i.e. check if the property matches and *then* check if the
    //subproperty matches.
		/*check_pred doesnt look if we already compiled it but if we have it in kb or builtins*/
		check_pred(dict[bi->pred]);



		//set up the subject and object
		pos_t  i1, i2;//s and o positions
		//k? "key"
		PredParam sk, ok;

		{
			nodeid s = dict[bi->subj];
			nodeid o = dict[bi->object];
#ifdef KBDBG
			(void)lm;
			(void)cm;
			ok = maybe_head(kbdbg_find_thing(--max, i2, locals), head, o);
			TRACE(dout<<"max:"<<max<<endl);
			sk = maybe_head(kbdbg_find_thing(--max, i1, locals), head, s);
			TRACE(dout<<"max:"<<max<<endl);
#else
			(void) locals;


			sk = maybe_head(find_thing(s, i1, lm, cm), head, s);
			ok = maybe_head(find_thing(o, i2, lm, cm), head, o);
#endif
		}

		TRACE(dout <<"perm " << permname.at(sk) << " " << permname.at(ok) << endl );

		jg = perms.at(sk).at(ok)(dict[bi->pred], jg, i1, i2, consts);
		//typedef function<join_gen(pred_gen, join_gen, pos_t , pos_t , Locals&)>
	}
	TRACE(dout << "kbdbg_part:" << kbdbg_part << ", kbdbg_part_max:" << kbdbg_part_max << endl);
	}
	
	return jg;
}





/*Take a list of subject/object pairs, and see if any pair would unify with the arguments s and o.
 * 					///shouldn't we check ep prior to running suc & ouc?
					i dunno, this is how we do it now but im not sure on the reasons
					i don't think it makes a functional difference but i think would be faster to ep-check prior to running the coros
					we'll work that out later
          the ep-check really just needs to see if the pred has been called with the same args twice
          i dont really think so, it does would_unify, err right, lets say same == would_unify
          it needs to see if the pred has been called with any s/o that would_unify with the current s/o
          im still sketchy on how this actually works, if the current s/o would_unify with any s/o that you previously
          called with, then the results of executing the pred with the current s/o would be equivalent to the results
          of that previous application, so you ep-out,
			at least thats the high level description

          think each time you call a pred, it makes a node in a tree, labeled with the actual spo it gets called with
          that pred will call other preds, and they'll be child-nodes
          every time you call a pred with a particular s/o, you check all of this node's ancestors to see if one of its
          ancestors is the same pred with some s/o that would_unify with the current s/o.
          so if you ask A B C,
          and inside that Z B C,
          A and Z dont unify,
          if you ask ?Anything B C,
          and later Z B C,
          Z would already be being returned by the first call,

          correspondingly for Z B C followed by ?Anything B C
          ^i dont think would_unify corresponds with this
          err no this wouldnt make sense, the second call is more general
          think of it like, there is a more general query up the tree
					  so now we dont go into a more specific query

					  but if its the other way around we do

*/
bool find_ep(ep_t *ep, /*const*/ Thing *s, /*const*/ Thing *o)
{
	FUN;
	s = getValue(s);
	o = getValue(o);
	TRACE(dout << ep->size() << " ep items:" << endl);
	//thingthingpair
	for (auto i: *ep)
	{
	  //Thing*
		auto os = i.first;
		auto oo = i.second;
		TRACE(dout << endl << " " << str(os) << "    VS     " << str(s) << endl << str(oo) << "    VS    " << str(o) << endl;)

		if (would_unify(os,s))
		{
			//TRACE(dout << ".." << endl);
			if(would_unify(oo,o)) {
				TRACE(dout << endl << "EP." << endl;)
				return true;
			}
		}
			TRACE(dout << endl << "---------------------" << endl);

	}
	return false;
}

//typedef std::pair<Thing*, Thing*>
//typedef std::vector<thingthingpair> ep_t
//Why the garbage collection stuff?
//well..we allocate it, we have to free it
ep_t * new_ep()
{
	ep_t *ep = new ep_t();
	eps_garbage.push_back(ep);
	return ep;
}


rule_t compile_rule(Rule r)
{
	FUN;

	//What do each of these do?
	//should draw a picture of the data structure(s)
	//maps from nodeids to indexes into locals/consts


	//typedef map<nodeid, pos_t> locals_map;
	//typedef unordered_map<nodeid, pos_t> locals_map;
	locals_map lm, cm;
	//i'm not familiar with how this works

	//these will be needed after this func is over so we allocate them on the heap
	//typedef vector<Thing> Locals;
	Locals &locals_template = *new Locals();
	locals_templates_garbage.push_back(&locals_template);//and register them for garbage collection

	Locals *consts_ = new Locals();
	consts_garbage.push_back(consts_);
	Locals &consts = *consts_;

	/*
	Structures to fill out:
	-------------------------
	locals_template:	    vector<Thing>         Locals
	consts:			          vector<Thing>         Locals
	lm:			              map<nodeid, pos_t>    locals_map
	cm:			              map<nodeid, pos_t>    locals_map

	Input:	//why not just send it the Rule? see how its used in query()
	-------------------------
	r.head:			pquad
	r.body:			pqlist
	*/
	make_locals(locals_template, consts, lm, cm, r.head, r.body);


	join_gen jg = compile_body(locals_template, consts, lm, cm, r.head, r.body);

	pos_t  hs, ho; // indexes of head subject and object in locals
#ifdef KBDBG
	kbdbg_find_thing(kbdbg_part++, hs, locals_template);
	kbdbg_find_thing(kbdbg_part++, ho, locals_template);
	TRACE(dout << "kbdbg_part:" << kbdbg_part << ", kbdbg_part_max:" << kbdbg_part_max << endl);
	kbdbg_part = kbdbg_part_max;
#else
	//ignoring key, because head s and o go into locals always
	find_thing(dict[r.head->subj], hs, lm, cm);//sets hs
	find_thing(dict[r.head->object], ho, lm, cm);
#endif

	EEE;
	join_t j;
	coro suc, ouc;
	TRC(int call = 0;)
	ep_t *ep = new_ep();
	//ep is one per rule just as locals_template and consts
	
	//where to memcpy locals from and what length
	auto locals_data = locals_template.data();
	auto locals_bytes = locals_template.size() * sizeof(Thing);
	Thing * locals=0; //to be malloced inside the lambda
	const bool has_body = r.body && r.body->size(); // or is it a fact? (a rule without any conditions)

	return [has_body, locals_bytes, locals_data, ep, hs, ho, locals ,&consts, jg, suc, ouc, j, entry TRCCAP(call) TRCCAP(r)](Thing *s, Thing *o) mutable {
		setproc("rule");
		TRC(++call;)
//		TRACE(dout << op->formatr(r) << endl;)
		TRACE(dout << "call=" << call << endl;)
		switch (entry) {
			case 0:

				/*optimization: this could happen in a different thread.
				http://www.drdobbs.com/parallel/writing-lock-free-code-a-corrected-queue/210604448
				 //lets first try to find some offtheshelf solution, boost? sure
				cpu affinity should be set.*/
				locals = (Thing*)malloc(locals_bytes);
				memcpy(locals, locals_data, locals_bytes);
				/*also, since when a pred is invoked, all rules are gone thru,
				 * we should try (as long as this isnt too memory-intensive) allocating per-pred,
				 * or allocating as late as possible,
				 * either by adding one more var between the s/o parameters and their locals counterparts,
				 * or
				 * leveraging would_unify
				 * -would_unify and unify could be made to work together with minimal overhead
				 */



				//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
				ASSERT(hs < locals_bytes / sizeof(Thing));
				ASSERT(ho < locals_bytes / sizeof(Thing));

				suc = unify(s, &locals[hs]); // try to match head subject
				while (suc()) {
					TRACE(dout << "After suc() -- " << endl;)
					//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
					ASSERT(call == 1);

					ouc = unify(o, &locals[ho]);
					while (ouc()) {
						TRACE(dout << "After ouc() -- " << endl;)
						//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
						ASSERT(call == 1);

						o = getValue(o);
						s = getValue(s);
						/*i dunno*/
						if (find_ep(ep, s, o)) {
							if (has_body) {
								steps++;
								goto end;
							}
						}
						if (has_body ) ep->push_back(thingthingpair(s, o));

						j = jg();
						while (j(s, o, locals)) {
							TRACE(dout << "After c0() -- " << endl;)
							//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)

							if(has_body) {
								ASSERT(ep->size());
								ep->pop_back();
							}

							TRACE(dout << "MATCH." << endl;)
							entry = LAST;
							return true;

				case_LAST:;
							TRACE(dout << "RE-ENTRY" << endl;)
							if (has_body ) ep->push_back(thingthingpair(s, o));
						}

						if(has_body) {
							ASSERT(ep->size());
							ep->pop_back();
						}
				end:;
					}
				}

				TRACE(dout << "DONE." << endl;)
				free(locals);
				END
		}
	};
}



//endregion











//region interface



pnode thing2node(Thing *t_, qdb &r) {
	t_ = getValue(t_);

	auto t = *t_;

	if (is_list(t))
	{/*this here needs to call create_list_triples
		const string head = listid();
		for (size_t i = 0; i < get_size(t); i++) {
			auto x = (t_ + 1 + (i*2));
			r.second[head].emplace_back(thing2node(x, r));
		}
		*/
		return mkbnode(pstr("FU"));
	
	}

	if (is_node(t) || is_list_bnode(t))
		return std::make_shared<node>(dict[get_node(t)]);

	//dout << "thing2node: Wtf did you send me?, " << str(t_) << endl;
	assert(is_unbound(t));
	return mkiri(pstr("?V?A?R?"));
}


void add_result(qdb &r, Thing *s, Thing *o, nodeid p)
{
	FUN;
	r.first["@default"]->push_back(
		make_shared<quad>(
			quad(
				thing2node(s, r),
				std::make_shared<node>(dict[p]),
				thing2node(o, r)
			)
		)
	);
}






yprover::yprover ( qdb kb)  {
	make_perms_table();
	build_in_facts();
	build_in_rules();
	compile_kb(kb);
}


yprover::~yprover()
{
	free_garbage();
}


void yprover::thatsAllFolks(int nresults){
	dout << "That's all, folks, " << nresults << " results." << endl;
	dout << unifys << " unifys, " << steps << " steps." << endl;
	steps_ = steps;
	unifys_ = unifys;
}

#ifdef KBDBG

/*kbdbg fell to the wayside, it needs to be adapted to bnodes in lists and to builtins*/

void print_kbdbg_part(stringstream &o, termid t, unsigned long part)
{
	o << "[";
	if (islist(t)) {
		o << "\"( \",";
		unsigned long p = 0;
		auto lst = op->get_dotstyle_list(t);
		
		for (auto i: lst)
		{
			print_kbdbg_part(o, i, p++);
			o << ",\" \",";
		}
		o << "\")\"";
	}
	else
		o << "\"" << dstr(t->p, true) << "\"";
	o << "]";
}

void print_kbdbg_term(stringstream &o, termid t, unsigned long &part)
{
	print_kbdbg_part(o, t->s, part++);
	o << ",\" " << dstr(t->p, true) << " \",";
	print_kbdbg_part(o, t->o, part++);
}

void print_kbdbg_termset(stringstream &o, prover::termset b, unsigned long &part)
{
	size_t i = 0;
	for (auto bi: b) {
		print_kbdbg_term(o, bi, part);
		if (i++ != b.size() - 1)
			o << ", \". \", ";
	}
}


void print_kbdbg(prover::termset query)
{
	unsigned long part = 0;
	for (auto rs: rules) {
		for (auto rule: rs.second) {
			stringstream o;
			auto h = rule->head;
			o << "\"{\",";
			print_kbdbg_term(o, h, part);
			o << ",\"}\"";
			auto b = rule->body;
			if (b&&b.size()) {
				o << ",\" <= {\",";
				print_kbdbg_termset(o, b, part);
				o << ",\"}\"";
			}
			o << ",\"\\n\"";
			dout << "[" << o.str() << "]" << endl;

		}
	}
	stringstream o;
	print_kbdbg_termset(o, query, part);
	dout << "[" << o.str() << "]" << endl;
}
#endif


Rules add_ruleses (Rules &a, Rules b)
{
	FUN;
	Rules r;
	for (auto aa: a)
		for (auto x: aa.second)
			r[aa.first].push_back(x);

	for (auto bb: b)
		for (auto x: bb.second)
			r[bb.first].push_back(x);

	return r;
}


void yprover::query(qdb& goal){
	FUN;
	results.clear();

	//die if the query is empty
	auto qit = goal.first.find("@default");
	if (qit == goal.first.end())
		return;
	//pqlist
	auto q = qit->second;

#ifdef KBDBG
	print_kbdbg(q);
#endif
	//Initialize 

	//Reset the global steps & unifys
	//Why not turn these into member variables of yprover.
	//because then all the functions where they are touched and subsequently everything would have to be in yprover
	steps = 0;
	unifys = 0;


	int nresults = 0; 

	locals_map lm, cm;
	Locals locals, consts;




	dout << KGRN << "COMPILE QUERY" << KNRM << endl;

	//here we combine the two Rules maps, and lists will 
	//be used by get_list and islist deep inside make_locals
	//get_list or something should then remove the triples of the internalized lists from the query
	
  //Adds the rules from the query to the rules from the kb? just for the lists
	lists = add_ruleses(rules, quads2rules(goal));
	make_locals(locals, consts, lm, cm, 0, q);
	join_gen jg = compile_body(locals, consts, lm, cm, 0, q);
	join_t coro = jg();

	dout << KGRN << "RUN" << KNRM << endl;
	//here we invoke satan
	while (coro( (Thing*)666,(Thing*)666, locals.data() )) {

    //Returned true, so found a result: the rest of this loop is handling the result.
		nresults++;
		dout << KCYN << "RESULT " << KNRM << nresults << ":";
		qdb r;
		r.first["@default"] = mk_qlist();

		//go over the triples of the query to print them out
		//*q  :: qlist
		//i   :: pquad
		for(auto i: *q)
		{
			Thing *s = &fetch_thing(dict[i->subj], locals, consts, lm, cm);
			Thing *o = &fetch_thing(dict[i->object], locals, consts, lm, cm);

			//TRACE(dout << sprintThing("Subject", s) << " Pred: " << i->pred->tostring() << " "  << sprintThing("Object", o) << endl;)

			//lets try to get the original names of unbound vars
			//did we succeed at this yet? i think so
			Thing n1, n2;
			if (is_unbound(*s)) {
				s = &n1;
				n1 = create_node(dict[i->subj]);
			}
			if (is_unbound(*o)) {
				o = &n2;
				n2 = create_node(dict[i->object]);
			}

			dout << str(getValue(s)) << " " << i->pred->tostring() << " "  << str(getValue(o)) << endl;

			add_result(r, s, o, dict[i->pred]);
		}

		results.emplace_back(r);

		if (result_limit && nresults == result_limit) {
			dout << "STOPPING at " << KRED << nresults << KNRM << " results."<< endl;
			free_garbage_nonassert();
			goto out;
		}

	}
	thatsAllFolks(nresults);

  //what does this do//this is a goto label
	out:;


	if (log_outputString.size()) {
		dout << "log#outputString:" << endl;
		for (const auto x:log_outputString)
			dout << x.first << ": " << x.second << endl;
		log_outputString.clear();
	}

}

//endregion


//region builtins
//hows this?
//maybe we should put this compilation stuff with the rest of the compilation stuff


//Outer vector: list
//Inner vector: triples of nodeids
void add_facts(vector<vector<nodeid>> facts)
{
	//std::map<nodeid,std::vector<std::pair<Thing*,Thing*>>>
	ths_t &ths = *new ths_t;
	ths_garbage = &ths;///.push_back(ths);

	//Map each pred to a list of it's subject/object pairs
	for (auto f: facts)
		ths[f[1]].push_back({
			create_node(f[0]),
			create_node(f[2])});
	
	coro suc, ouc;
	//For each pred in the ths:
	//std::pair<nodeid,std::vector<std::pair<Thing*,Thing*>>>
	for (auto ff:ths)
	{
		//std::vector<std::pair<Thing*, Thing*>>
		auto &pairs = ff.second;
		EEE; //char entry = 0;
		pos_t  pi = 0;

		//Map each pred to a coro on it's subject/object
		builtins[ff.first].push_back([suc,ouc,pi,pairs,entry](Thing *s_, Thing *o_)	mutable{
			switch(entry)
			{
			case 0:
				//hrmm.. where is pi being incremented? looks 
				//like this just does pi=0 then exits.
				/*yea looks like you found a bug
this thing looks pretty suboptimal btw...buti guess it should get the job done*/
				if (pi < pairs.size())//fixed?
				{			
				//Generate a unify coro for the subject; 
				//on the fly.
				//Then run it.
					suc = unify(s_,&pairs[pi].first);
					while(suc()){
					//Again for the object
						ouc = unify(o_,&pairs[pi].second);
						while(ouc())
						{
						//If both succeed then yield
						//true.
						//not quite
							entry = LAST;
							

							return true;
				/*
				entry = LAST;
				 so...wanna fix this?
			*/
			case_LAST:;
						} 
					}
				}
			END;
			}
		});
	} 
}
	



/*
 * RDF - http://www.w3.org/TR/lbase/#using - where the ?y(?x) thing comes from
 */


void build_in_facts()
{
	add_facts({

//rdfsLiteral rdfssubClassOf rdfsResource
//rdflangString rdfssubClassOf rdfsLiteral
//rdfHTML rdfssubClassOf rdfsLiteral
		{rdfAlt,                          rdfssubClassOf,    rdfsContainer},
		{rdfBag,                          rdfssubClassOf,    rdfsContainer},
		{rdfsContainerMembershipProperty, rdfssubClassOf,    rdfProperty},
		{rdfsDatatype,                    rdfssubClassOf,    rdfsClass},
		{rdfSeq,                          rdfssubClassOf,    rdfsContainer},
		{rdfXMLLiteral,                   rdfssubClassOf,    rdfsLiteral},
		{rdfsisDefinedBy,                 rdfssubPropertyOf, rdfsseeAlso},

//rdfsResource rdftype rdfsClass ?
//rdfsClass rdftype rdfsClass ?
//rdfsLiteral rdftype rdfsClass
//rdfsDatatype rdftype rdfsClass
//rdflangString rdftype rdfsDatatype
//rdfHTML rdftype rdfsDatatype
//rdfProperty rdftype rdfsClass
		{rdfXMLLiteral,                   rdftype,           rdfsDatatype},
		{rdffirst,                        rdftype,           owlFunctionalProperty},
		{rdfrest,                         rdftype,           owlFunctionalProperty},
		{rdfnil,                          rdftype,           rdfList},

		{rdfscomment,                     rdfsdomain,        rdfsResource},
		{rdfscomment,                     rdfsrange,         rdfsLiteral},
		{rdfsdomain,                      rdfsdomain,        rdfProperty},
		{rdfsdomain,                      rdfsrange,         rdfsClass},
		{rdffirst,                        rdfsdomain,        rdfList},
		{rdffirst,                        rdfsrange,         rdfsResource},
		{rdfsisDefinedBy,                 rdfsdomain,        rdfsResource},
		{rdfsisDefinedBy,                 rdfsrange,         rdfsResource},
		{rdfslabel,                       rdfsdomain,        rdfsResource},
		{rdfslabel,                       rdfsrange,         rdfsLiteral},
		{rdfsmember,                      rdfsdomain,        rdfsContainer},
		{rdfsmember,                      rdfsrange,         rdfsResource},
		{rdfobject,                       rdfsdomain,        rdfStatement},
		{rdfobject,                       rdfsrange,         rdfsResource},
		{rdfpredicate,                    rdfsdomain,        rdfStatement},
		{rdfpredicate,                    rdfsrange,         rdfProperty},
		{rdfsrange,                       rdfsdomain,        rdfProperty},
		{rdfsrange,                       rdfsrange,         rdfsClass},
		{rdfrest,                         rdfsdomain,        rdfList},
		{rdfrest,                         rdfsrange,         rdfList},
		{rdfsseeAlso,                     rdfsdomain,        rdfsResource},
		{rdfsseeAlso,                     rdfsrange,         rdfsResource},
		{rdfssubClassOf,                  rdfsdomain,        rdfsClass},
		{rdfssubClassOf,                  rdfsrange,         rdfsClass},
		{rdfssubPropertyOf,               rdfsdomain,        rdfProperty},
		{rdfssubPropertyOf,               rdfsrange,         rdfProperty},
		{rdfsubject,                      rdfsdomain,        rdfStatement},
		{rdfsubject,                      rdfsrange,         rdfsResource},
		{rdftype,                         rdfsdomain,        rdfsResource},
		{rdftype,                         rdfsrange,         rdfsClass},
		{rdfvalue,                        rdfsdomain,        rdfsResource},
		{rdfvalue,                        rdfsrange,         rdfsResource}

	});
}
/*
rdfs:subPropertyOf is a partial order:
{} => {?P rdfs:subPropertyOf ?P}                                                              reflexive
{?p1 rdfs:subPropertyOf ?p2. ?p2 rdfs:subPropertyOf ?p3} => {?p1 rdfs:subPropertyOf ?p3}      transitive
{?p1 rdfs:subPropertyOf ?p2. ?p2 rdfs:subPropertyOf ?p1} => {?p1 == ?p2}                      anti-symmetric

*/

//https://www.w3.org/TR/rdf-schema/#ch_subpropertyof
//{?P1 @is rdfs:subPropertyOf ?P2. ?S ?P1 ?O} => {?S ?P2 ?O}.
//
//{?P3 @is rdfs:subPropertyOf rdfs:subPropertyOf. ?P1 ?P3 ?P2} => {?P1 @is rdfs:subPropertyOf ?P2}
//{?P4 @is rdfs:subPropertyOf rdfs:subPropertyOf. ?P3 ?P4 rdfs:subPropertyOf} => {?P3 @is rdfs:subPropertyOf rdfs:subPropertyOf}
//fixed point

//{?P3 @is rdfs:subPropertyOf ?P1. ?S ?P3 ?O}=> {?S ?P1 ?O}
rule_t make_wildcard_rule(nodeid pr)
{
	FUN;
	EEE;
	MSG("..")

	Thing p1 = create_unbound();
	Thing p2 = create_node(pr);
	pred_t sub, p1wildcard, p1lambda;
	nodeid p1p = 0;
	
	//old? //thats just for assert
	//hrm
	//I'm a bit confused about ep-check here. 
	//We ep-out if we repeat a (subject, object, rule) tuple.//subject, pred, object..err..well
	//you could say it both ways
	//Ok so the wildcard rule will be made for each pred
	//We can do that in compile_pred

  //If a pred gets called during its own execution with the same s/o as originally, then we ep out
  //same defined as if we can unify s with orig_s and o with orig_o; if we can then execution of
  //this pred with s and o will be equivalent to execution with orig_s and orig_o, which we're already doing.
  //So, each pred should have its own ep-table, and if execution of a pred ends up calling the same pred,
  //then the 2nd instance should use the same ep-table as the original.

  //That should handle all our ep and wildcard problems.

	ep_t *ep = new_ep();
	DBG(Thing old[2]);
	
	return [entry,ep,DBGC(old) p1,p1p,p2,sub,p1wildcard,p1lambda](Thing *s, Thing *o) mutable {
		setproc("wildcard");
		TRACE_ENTRY;

		switch(entry){
		case 0:

			DBG(old[0] = *s);
			// i need to figure out floobits text navigation
			if (find_ep(ep, s, o))
				DONE;
			ep->push_back(thingthingpair(s, o));

			//quite sure its there since its a rdf builtin; should be, at least. it was there last friday.
			sub = ITEM(preds,rdfssubPropertyOf); 
			while (sub(&p1, &p2))
			{
				ASSERT(is_bound(p1));
				{
					Thing *p1v = get_thing(p1);
					ASSERT(is_node(*p1v));
					p1p = get_node(*p1v);
				}

				if (!has(preds, p1p))
					preds[p1p] = make_wildcard_rule(p1p);

				p1lambda = ITEM(preds, p1p);
				while(p1lambda(s, o))//the recursion will happen here
				{
					entry = LAST;
					return true;
		case_LAST:;
				}
			}
			ASSERT(is_unbound(p1));
			ASSERT(are_equal(old[0], *s));
			ASSERT(ep->size());
			ep->pop_back();
			END;
		}
	};
}
	




//under construction
void build_in_rules()
{



	/*some commonly used vars*/
	EEE; //char entry = 0;
	coro suc, suc2, ouc;
	Thing *s = nullptr, *o = nullptr;
	Thing ss;
	//Thing oo;
	Thing *r = nullptr;
	
	Thing a,b;
	a = create_unbound();
	b = create_unbound();
	
	/*ep_t *ep = new ep_t();
	eps.push_back(ep)*/

	//pred_t :: function<bool(Thing*,Thing*)>
	pred_t p1, p2;


	//Thing c_rdfsType = create_node(op->make(rdftype));
	//Thing c_rdfsResource = create_node(op->make(rdfsResource));
	//Thing c_rdfssubClassOf = create_node(op->make(rdfssubClassOf));





	//rdfs:Resource(?x)
	/*<HMC_a> koo7: you mean if one queries "?x a rdf:Resource" should they get every known subject as a result?
	<HMC_a> the answer would be yes. :-)
	<koo7> HMC_a, every known subject and object, right?
	<koo7> or....every nodeid in the kb thats not in the pred position...where do we draw the line?
	<HMC_a> well, when i say "known subject" i don't really mean everything in the subject position, i mean every node useable as a subject (non-literal)
	<koo7> ok
	<koo7> what do you mean non-literal?
	<HMC_a> you wouldn't bind each int as a result, for example
	<HMC_a> if you returned "0 a Resource" "1 a Resource" "2 a Resource"..... this would be a bit of a problem ;-)
	<koo7> yeah, so everything that explicitly appears in the kb
	<koo7> traverse lists too
	<HMC_a> yes remember that lists are logically just triples as well...
	<koo7> err well wouldnt that mean the bnodes that rdf lists are made up of?
	<HMC_a> so any node name that appears within a list is in the object position of some rdf:first triple
	<HMC_a> yes, the bnode names as well*/

	/*builtins[rdftype].push_back([a, b, c_rdfsResource, entry, suc, suc2, ouc, s, o, p1](Thing *s_, Thing *o_) mutable {
		switch (entry) {
			case 0:
				o = getValue(o_);
				ASSERT(!is_offset(*o));
				ouc = unify(o, &c_rdfsResource);
				while (ouc())
				{
					s = getValue(s_);
					if(!is_unbound(*s))
					{
						entry = 1;
						return true;
					}
			case 1:
					if(is_unbound(*s))
					{
						for (auto &x: preds)
						{
							p1 = x.second;//a coro-permanent copy
							while(p1(&a, &b))
							suc = unify(s, &a);
							while(suc())
							{
								entry = 2;
								return true;
								case 2:;
							}
							suc2 = unify(s, b);
							while(suc2())
							{
								entry = 3;
								return true;
								case 3:;
							}
						}

					}
				}
				entry = 66;
				return false;
				END
		}
	});*/





	//todo rdfs:Class(?y) implies (?y(?x) iff rdf:type(?x ?y))
	//...nothing to implement?



	// https://www.w3.org/TR/rdf-schema/#ch_domain
	// {?Pred @has rdfs:domain ?Class. ?Instance ?Pred ?Whatever} => {?Instance a ?Class}.
	// rdfs:domain(?x ?y) implies ( ?x(?u ?v)) implies ?y(?u) )
	{
		Thing whatever = create_unbound();
		Thing pred = create_unbound();
		pred_t domain_pred, pred_coro;
		Thing pred_val;
		nodeid pred_nodeid = 0;

		/*this one might still need adding ep check*///really
		//Each pred should have its own ep-table. If execution of this pred ends up calling the same pred again,
		//the new instance should use the same ep-table.
		//We could have a single ep-table for the whole kb. 
		// * Start with an empty graph.
		// * Each triple in a query would make a graph-node, carrying the structure of the triple.
		// * When a triple in the query gets called, it executes rules sequentially. For each rule,
		//   make the children of the graph-node for the triple be the triples in the body, substituted
		//   with the HEAD_S and HEAD_O that result from unifying the query triple with the head triple.
		// * When you want to ep-check anywhere in the query, you just follow the ancestors of a node.
		


		/* Why are we doing domain for rdftype?
		builtins are just grouped by their pred
		just as kb rules are grouped into pred_t's
		rdftype is the predicate of the head of the rule
		rule has head s rdftype o */

	builtins[rdftype].push_back([entry,domain_pred,pred,pred_val,pred_nodeid,pred_coro,whatever](Thing *instance, Thing *cls) mutable {
		setproc("domainImpliesType");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			domain_pred = ITEM(preds,rdfsdomain);
			while (domain_pred(&pred, cls))
			{
				{
					ASSERT(is_bound(pred));
					Thing *pred_val = get_thing(pred);
					//how do we know it's not another bound var good q.......... i guess it wouldnt if a rule returned it
					//at least i put the assert there:) we can test it
					//yeah hm in the long run we should get the floobits session and tmux on one machine i guess
					//so isnt that a bug
					/*i dunno if we're supposed to allow rules to imply this*/
					//if it's  a semantic restriction it should probably be handled more fully
					//but in the typesystem, not here..at least thats my guess 
					/*anyway good catch
					 * tests/rdf/domainImpliesType-tricky
					*/
					ASSERT(is_node(*pred_val));
					nodeid pred_nodeid = get_node(*pred_val);

					//(So if the pred is not there, )a subproperty of it might still satisfy,
					//but we don't have a pred to run, so make a wildcard rule
					if (!has(preds, pred_nodeid))
						preds[pred_nodeid] = make_wildcard_rule(pred_nodeid);
					//If the pred is there, use that. This will need the wildcard rule to be added to the pred during compile_kb
					pred_coro = ITEM(preds, pred_nodeid);
				}
				
				ASSERT(is_unbound(whatever));
				while(pred_coro(instance, &whatever))
				{
					entry = LAST;
					return true;
		case_LAST:;
				}
				ASSERT(is_unbound(whatever));
			}
			return false;
			END;
		}
	});
	}

//	tests/rdf/domainImpliesType passess


//K so should we package these up into one builtin?
	//hell no!
	// {?Pred @has rdfs:range ?Class. ?Whatever ?Pred ?Instance} => {?Instance a ?Class}.
	// rdfs:range(?x ?y) implies ( ?x(?u ?v)) implies ?y(?v) )
//Alright how does that look
	//like a lot of duplicated code hehe yep lol
/*  {
	Thing whatever = create_unbound();
	Thing pred = create_unbound();

  builtins[rdftype].push_back([entry,pred,p1,p2,whatever](Thing *instance, Thing *cls) mutable {
    setproc("rangeImpliesType");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			range_pred = ITEM(preds,rdfsrange);
			while (range_pred(&pred, cls))
			{
				{
					ASSERT(is_bound(pred));
					Thing *pred_val = get_thing(pred);
					//how do we know it's not another bound var good q.......... i guess it wouldnt if a rule returned it
					//at least i put the assert there:) we can test it
					//yeah hm in the long run we should get the floobits session and tmux on one machine i guess
					//so isnt that a bug
					//if it's  a semantic restriction it should probably be handled more fully
					//but in the typesystem, not here..at least thats my guess

					ASSERT(is_node(*pred_val));
					nodeid pred_nodeid = get_node(*pred_val);

					if (!has(preds, pred_nodeid))
						preds[pred_nodeid] = make_wildcard_rule(pred_nodeid);
					pred_coro = ITEM(preds, pred_nodeid);
				}
				ASSERT(is_unbound(whatever));
				while(pred_coro(&whatever,instance))
				{
					entry = LAST;
					return true;
		case_LAST:;
				}
				ASSERT(is_unbound(whatever));
			}
			return false;
			END;
		}

  });
  }

*/



	//rdfs:subClassOf(?x ?y) implies (forall (?u)(?x(?u) implies ?y(?u))
	//{?sub rdfs:subClassOf ?sup. ?something a ?sub} => {?something a ?sup}.
	{
		Thing sub = create_unbound();
		pred_t subclassof_pred, type_coro;

	builtins[rdftype].push_back([entry,sub,subclassof_pred,type_coro](Thing *something, Thing *sup) mutable {
		setproc("XasupIfXasub");
		TRACE_ENTRY;
		switch(entry){
		case 0:
			subclassof_pred = ITEM(preds,rdfssubClassOf);
			while (subclassof_pred (&sub, sup))
			{
				ASSERT(is_bound(sub));
				type_coro = ITEM(preds, rdftype);
				while(type_coro(something, &sub))
				{
					entry = LAST;
					return true;
		case_LAST:;
				}
			}
			ASSERT(is_unbound(sub));
			END;
		}
	});
	}


	//rdfs:Class(?x) implies ( rdfs:subClassOf(?x ?x) and rdfs:subClassOf(?x rdfs:Resource) )

	//(rdfs:subClassOf(?x ?y) and rdfs:subClassOf(?y ?x)) implies  "?x == ?y" <-- how to handle?



	/*
19:18 < HMC_a> stoopkid: in rdf there is a tricky caveat, all possible lists are assumed to exist
19:19 < HMC_a> so there are lists that begin with ?x regardless of the value of ?x or state pf the kb....
19:20 < HMC_a> this may seem strange, but it is indeed how cwm, euler, et al (racer, pwlim, jena, the lot) do work
19:24 < HMC_a> stoopkid: well technically it is to match McCarthy's theory of arrays, but that is just minutia really
19:27 < HMC_a> stoopkid: suffice to say that this is a very old soundness-of-closure detail that survived through and was inherited into rdf
19:28 < HMC_a> it is a tricky caveat conceptually, but actually very easy to implement for us :-)
19:46 < HMC_a> stoopkid: it returns with ?l and ?x left as unbound vars!  It returns "?l rdf:first ?x"... in the "result graph" this is a top level statement, so the vars are considered as existential!  So it returns exactly a statement interpreted as "there exists a list l and a node x such that x is the rdf:first of l" which is ofc tautologicaly true regardless of what the kb state is. ;-)
19:46 < HMC_a> our universal vars like ?foo are also bnodes, remember
19:47 < HMC_a> all vars are bnodes, all bnodes are vars! XD
19:47 < stoopkid> ok so it just returns the existential vars, it doesn't return the infinite solution set, gotcha
19:48 < HMC_a> the ones we write like ?foo are universals in subformulae and existential at top level (we use "quantifier alternation" instead of tricky @forall type crap and skolem rewrites and such... mostly just "because we can and it is easier"... ("we can" because we are ordinal and do not allow nesting of formulae at the term rewrite))
19:48 < HMC_a> so nothing rszeno said was wrong wrt more general reasoner semantics, we are just playing on our constraints there wrt skolem et al
19:49 < HMC_a> and "skipping around" some of the more complex semantic details that would occur if we cared for more general formula interpretation :-)
19:49 < HMC_a> anyway
19:51 < stoopkid> now, i'm still iffy on calling vars universals, it seems to me that they are always existential and that it's the {}=>{} that makes for universal quantification
19:51 < HMC_a> the ones we write like _:foo are existentials, and similarly ones like ?foo at a top level in a syntactic unit are considered existential as a sort of special case, and there are some other "tricky" special cases that "sneak in" like variables that appear in a head but not a body end up as something like "doesn't matter which" quantifier in a sense, but again these are all details of our particular reasoner, not of more general rdf.... HEH
19:51 < HMC_a> 19:56 < stoopkid> now, i'm still iffy on calling vars universals, it seems to me that they are always existential and that it's the {}=>{} that makes for universal quantification
19:51 < HMC_a> right, you're not exactly wrong at all...
19:52 < HMC_a> how you are looking at it is sound for our reasoner
19:52 < HMC_a> but is not sound for rdf reasoning in general! hehe
19:53 < HMC_a> in more general rdf reasoning you might run into statements that are structured arbitrarily, where the corresponding predicate fol expression would have arbitrary implication and quantifier placement
19:53 < HMC_a> we do *not* allow arbitrary implication structures *or* arbitrary quantifier placement!! hehe
19:54 < stoopkid> i see
19:54 < HMC_a> well, more accurately, we allow them syntactically but do not reason over them directly in our semantics! :-)
19:55 < HMC_a> we only take "outer" implications (not nested) in our inference chaining, and we determine quantifiers (universal or existential) by placement relative to these
19:56 < HMC_a> someone could ofc write, in our logic, an interpretation that *does* consider things like nested implications, arbitrary quantifications, and a math:sum that will do factoring of objects to bind subject lists...
19:57 < HMC_a> but we do none of these things in our "core" rewrite semantics. :-)
*/

	//if the subject is bound, and bound to a list just take it's first item.
	// If it is bound to something that is not a list, fail.
	// If it is unbound, do a trivial yield (no new binding first).
	// Why the trivial yield on the unbound variable?
	builtins[rdffirst].push_back(
			[entry, ouc, s, o](Thing *s_, Thing *o_) mutable {
				setproc("rdffirst");
				switch (entry) {
					case 0:
						s = getValue(s_);
						o = getValue(o_);
						TRACE(dout << str(s) << " #first " << str(o) << endl);
						if (is_unbound(*s))
							ouc = gen_succeed();
						else if (is_list(*s))
							ouc = unify(o, s + 2);
						else if (is_list_bnode(*s))
							ouc = unify(o, s + 1);
						else
							ouc = GEN_FAIL;
						entry = LAST;
						while (ouc())
						{
							return true;
							case_LAST:;
						}
						END
				}
			}
	);

	builtins[rdfrest].push_back(
			[entry, ouc, s, o, r](Thing *s_, Thing *o_) mutable {
				setproc("rdfrest");
				switch (entry) {
					case 0:
						s = getValue(s_);
						o = getValue(o_);
						TRACE(dout << str(s) << " #rest " << str(o) << endl);
						if (is_list(*s))
							r = s+3;
						else if(is_list_bnode(*s))
							r = s+2;
						else {
	
							entry = 66;
							return false;
						}

						ouc = unify(o, r);
						entry = LAST;
						while (ouc())
						{
							return true;
							case_LAST:;
						}
						END
				}
			}
	);








/*
<HMC_a> koo7: for the moment I'm less concerned about getting rdfs going and more interested in facilities like log:outputString and math:sum and etc
<HMC_a> really even just those two would be enough to get some useful results out of the fuzzer, lol :-)
<HMC_a> http://www.w3.org/2000/10/swap/doc/CwmBuiltins <-- "not quite specs" XD
<HMC_a> so math:* and string:* should be pretty straightforward, list:* nearly so...
<koo7> as on list, list:append seems a misnomer
<HMC_a> koo7: you mean it is more aptly called "concat"? ;-)
<koo7> yeah something like that
<HMC_a> I don't think you're the first to mention it, hehe
* HMC_a shrugs
<koo7> alright
<koo7> its fully namespaced, after all
<HMC_a> sure, and I'm not against doubling up on some builtins later... maybe in the end we have list:append, tau:append, and tau:concat, with tau:append taking just a subject of a pair list of a list and a literal and doing an actual "append" and the other two both being "concat"...
<HMC_a> but we want list:append to be there and to match cwm, in any case, so that any existing logic "out there" that calls upon cwm's list:append will do the right thing
 */



	/*
	 * @prefix math: <http://www.w3.org/2000/10/swap/math#>.
	 */


	//sum: The subject is a list of numbers. The object is calculated as the arithmentic sum of those numbers.

	//why not call it pred or pred_iri
	string bu = "http://www.w3.org/2000/10/swap/math#sum";
	auto bui = dict.set(mkiri(pstr(bu)));

	builtins[bui].push_back(
			[r, bu, entry, ouc, s, ss](Thing *s_, Thing *o_) mutable {
				//TRACE_ENTRY?
				switch (entry) {
					case 0: {

						s = getValue(s_);
				//At this point s should not be either a bound
				//var or an offset.

						ss = *s;
				//At this point neither s nor ss should be either 				//bound vars or offsets.


				//Is this an error? //we just dont unify
						if (!is_list(ss)) {
							dout << bu << ": " << str(s) << " not a list" << endl;
							DONE;
						}


						long total = 0;
						const size_t size = get_size(ss);
						for (pos_t  i = 0; i < size; i++) {
							Thing item = *(s + 1 + i);
				//Make sure it's a node. Is this an error if it's not?
							if (!is_node(item)) {
								dout << bu << ": " << str(&item) << " not a node" << endl;
								DONE;
							}
				
							node p = dict[get_node(item)];
				//Make sure it's an XSD_INTEGER. Is this an error if it's not?
							if (p.datatype != XSD_INTEGER) {
								dout << bu << ": " << p.tostring() << " not an int" << endl;
								DONE;
							}

				//Convert the text value to a long, and add
				//it to the total.
							total += atol(ws(*p.value).c_str());
						}

				//Convert the total sum to a text value.
						std::stringstream ss;
						ss << total;

				//Create an XSD_INTEGER node & corresponding Thing for the total
						r = new(Thing);
						*r = create_node(dict[mkliteral(pstr(ss.str()), pstr("XSD_INTEGER"), 0)]);

				//Unify the input object with the Thing now representing the total.
						ouc = unify(o_, r);
					}
						while (ouc()) {
							TRACE(dout << "MATCH." << endl;)
							entry = LAST;
							return true;
					case_LAST:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
				//No longer need the Thing representing the total. hmm
				//How about the node representing the total?
						delete (r);
						END;
				}
			}
	);


	/*
	 * @prefix log: <http://www.w3.org/2000/10/swap/log#>.
	 * */

/*
//outputString	The subject is a key and the object is a string, where the strings are to be output in the order of the keys. See cwm --strings in cwm --help.
<koo7> Dump :s to stdout ordered by :k whereever { :k log:outputString :s }
<koo7> so this means it waits until the query is done?
<HMC_a> yes, it caches up the output strings until the end
<HMC_a> then sorts them by subject
<HMC_a> then prints
 */
	bu = "http://www.w3.org/2000/10/swap/log#outputString";
	bui = dict.set(mkiri(pstr(bu)));
	builtins[bui].push_back(
			[bu, entry](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0: {
						auto s = getValue(s_);
						Thing s2 = *s;
						if (!is_node(s2)) {
							dout << bu << ": " << str(s) << " not a node" << endl;
							DONE;
						}
						auto o = getValue(o_);
						Thing o2 = *o;
						if (!is_node(o2)) {
							dout << bu << ": " << str(o) << " not a node" << endl;
							DONE;
						}
						auto sss = dict[get_node(s2)].tostring();
						auto ooo = dict[get_node(o2)].tostring();

						log_outputString[sss] = ooo;
						entry = LAST;
						return true;
					}
					case_LAST:;
						END;
				}
			}
	);
}





//endregion









//https://www.w3.org/TR/rdf-schema/#ch_type
//http://wifo5-03.informatik.uni-mannheim.de/bizer/SWTSGuide/carroll-ISWC2004.pdf

//Unicode

//XML Schema
//http://www.w3.org/TR/2009/CR-xmlschema11-2-20090430/

//LBase
//http://www.w3.org/TR/lbase/

//RDF
//http://www.w3.org/2011/rdf-wg/wiki/Main_Page
//http://www.w3.org/TR/2014/NOTE-rdf11-primer-20140225/
//http://www.w3.org/TR/2013/WD-rdf11-mt-20130409/
//http://www.w3.org/TR/rdf11-new/
//http://www.w3.org/TR/rdf11-concepts/
//http://www.w3.org/TR/rdf-syntax-grammar/
//http://www.w3.org/TR/2014/NOTE-rdf11-datasets-20140225/

//N-Quads
//http://www.w3.org/TR/2014/REC-n-quads-20140225/

//N-Triples
//http://www.w3.org/TR/n-triples/

//JSON

//JSON-LD
//http://www.w3.org/TR/json-ld/

//Notation 3
//

//RIF
//http://www.w3.org/standards/techs/rif#w3c_all
//http://www.w3.org/TR/rif-dtb/
//http://www.w3.org/TR/2013/REC-rif-dtb-20130205/
//http://sourceforge.net/p/eulersharp/discussion/263032/thread/d80e9aa6/

//Cwm Builtins
//http://www.w3.org/2000/10/swap/doc/CwmBuiltins   	--< HMC_a_> not all but most
//which?

//DTLC
//http://rbjones.com/rbjpub/logic/cl/tlc001.htm
//http://ceur-ws.org/Vol-354/p63.pdf

//OWL
//http://www.w3.org/TR/owl2-overview/

//make our semantics conform to them! ^














/*log:equalTo a rdf:Property;
True if the subject and object are the same RDF node (symbol or literal).
Do not confuse with owl:sameAs.
A cwm built-in logical operator, RDF graph level.
*/


	///std::sort(myList.begin(), myList.end(), [](int x, int y){ return std::abs(x) < std::abs(y); });
	///sort(facts.begin(), facts.end(), [](auto a, auto b) { return a[1] < b[1]; });





	/*
	//@prefix list: <http://www.w3.org/2000/10/swap/list#>.
	//list last item
	bu = "http://www.w3.org/2000/10/swap/list#last";
	bui = dict.set(mkiri(pstr(bu)));
	builtins[bui].push_back(
			[bu, entry, ouc](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0: {
						auto s = getValue(s_);
						Thing s2 = *s;
						if (!is_list(s2)) {
							dout << bu << ": " << str(s) << " not a list" << endl;
							DONE;
						}

						auto size = get_size(s2);
						if (size == 0) DONE;
						ouc = unify(s + size, o_);
					}
						while (ouc()) {
							entry = LAST;
							return true;
							case_LAST:;
						}
						END;
				}
			}
	);
	 */

	/*
	//nope
	//item in list
	bu = "http://www.w3.org/2000/10/swap/list#in";
	bui = dict.set(mkiri(pstr(bu)));
	builtins[bui].push_back(
			[bu, entry, ouc](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0: {
						auto s = getValue(s_);
						Thing s2 = *s;
						if (!is_node(s2)) {
							dout << bu << ": " << str(s) << " not a node" << endl;
							DONE;
						}
						auto t = get_term(s2);

						auto o = getValue(o_);
						Thing o2 = *o;
						if (!is_list(o2)) {
							dout << bu << ": " << str(o) << " not a list" << endl;
							DONE;
						}

						const auto size = get_size(o2);
						//?bool found = false;

						for (size_t i = 0; i < size; i++) {
							Thing item = *(o + 1 + i);
							if (is_node(item)) {
								if (t == get_term(item)) {
									entry = LAST;
									return true;
								}
							}
						}
					}
					case_LAST:;
						END;
				}
			}
	);
	 */


//#ifdef cppout
/*
lets forget builtins for now
persistent vars needed:
entry

per rule:
headsub headsuc

body items:
body becomes nested whiles calling predxxx instead of the join-consing thing

+more locals


*/
/* i made some things slightly different than how we do them in our lambdas
	 * instead of returning bools, we indicate being done by setting entry to -1
	 * callee state obviously has to be kept explicitly, not hidden with a lambda*/
fstream out;

string predname(nodeid x)
{
	stringstream ss;
	ss << "cpppred" << x;
	return ss.str();
}


string param(PredParam key, pos_t thing_index, pos_t rule_index)
{
	stringstream ss;
	if (key == HEAD_S)
	ss << "s";
	if (key == HEAD_O)
	ss << "o";
	if (key == LOCAL)
	ss << "(&state.locals[" << thing_index << "])";
	if (key == CONST)
	ss << "(&consts" << rule_index << "[" << thing_index << "])";
	return ss.str();
}

string things_literals(const Locals &things)
{
	stringstream ss;
	ss << "{";
	pos_t i = 0;
	for (Thing t: things) {
		if (i++ != 0) ss << ", ";
		ss << "Thing((ThingType)" << t.type << ", " << t.node << ")";
	}
	ss << "}";
	return ss.str();
}

void cppout_pred(string name, vector<Rule> rules)
{
	out << "void " << name << "(cpppred_state &state";
	if (name != "query") out << ", Thing *s, Thing *o";
	out << "){\n";
	for (pos_t i = 0; i < rules.size(); i++) {
		if (rules[i].head && rules[i].body && rules[i].body->size())
			out << "static ep_t ep" << i << ";\n";



		auto &r = rules[i];
		locals_map lm, cm;
		Locals locals_template;
		Locals consts;
		make_locals(locals_template, consts, lm, cm, r.head, r.body);




		out << "static Locals consts" << i << " = " << things_literals(consts) << ";\n";
	}

	int label = 0;

	out << "switch(state.entry){\n";

	out << "case "<<label++ << ":\n";

	size_t max_body_len = 0;
	for (auto rule:rules) {
		if (rule.body && max_body_len < rule.body->size())
			max_body_len = rule.body->size();
	}

	out << "state.states.resize(" << max_body_len << ");\n";

	int i = 0;
	//Rule rule
	for (auto rule:rules)
	{
		out << "//rule " << i << ":\n";
		//out << "// "<<<<":\n";
		out << "case " << label << ":\n";

		label++;
		out << "state.entry = " << label << ";\n";

		locals_map lm, cm;
		Locals locals_template;
		Locals consts;
		make_locals(locals_template, consts, lm, cm, rule.head, rule.body);

		out << "state.locals = " << things_literals(locals_template) << ";\n";

		if (rule.head) {
			pos_t hs, ho; // indexes of head subject and object in locals
			find_thing(dict[rule.head->subj], hs, lm, cm);//sets hs
			find_thing(dict[rule.head->object], ho, lm, cm);

			out << "state.suc = unify(s, &state.locals[" << hs << "]);\n";
			out << "if(state.suc()){\n";
			out << "state.ouc = unify(o, &state.locals[" << ho << "]);\n";
			out << "if(state.ouc()){\n";
		}
		if (rule.head && rule.body && rule.body->size()) {
			out << "if (!find_ep(ep" << i << ", s, o)){\n";
			out << "ep" << i << ".push_back(thingthingpair(s, o));\n";
		}

		if(rule.body) {
			//	reverse(b2.begin(), b2.end());
			size_t j = 0;
			for (pquad bi: *rule.body) {
				out << "//body item" << j << "\n";

				stringstream ss;
				ss << "state.states[" << j << "]";
				string substate = ss.str();

				out << "state.states[" << j << "] = cpppred_state();\n";
				out << "do{\n";

				//	check_pred(dict[bi->pred]);

				//set up the subject and object
				pos_t i1, i2;//s and o positions

				nodeid s = dict[bi->subj];
				nodeid o = dict[bi->object];

				PredParam sk, ok;

				sk = maybe_head(find_thing(s, i1, lm, cm), rule.head, s);
				ok = maybe_head(find_thing(o, i2, lm, cm), rule.head, o);

				out << predname(dict[bi->pred]) << "(" << substate << ", " <<
					param(sk, i1, i) << ", " << param(ok, i2, i) << ");\n";

				out << "if(" << substate << ".entry == -1) break;\n";
				j++;
			}
		}

		out << "return;\n";
		out << "case " << label++ << ":;\n";
		out << "state.entry = -1;\n";

		if(rule.body)
			for (pos_t closing = 0; closing < rule.body->size(); closing++)
				out << "}while(true);\n";

		if (rule.head && rule.body && rule.body->size())
				out << "}\nASSERT(ep" << i << "->size());\nep->pop_back();";

		if (rule.head) {
			out << "}\n"
					"state.ouc();//unbind\n"
					"}\n"
					"state.suc();//unbind\n";
		}
		i++;
	}
	out << "}}\n\n";

}

//void cpploop(size_t &label, size_t j, pquad bi, Rule r, Locals &lm, Locals &cm)


void yprover::cppout(qdb &goal)
{
	FUN;

	out.open("out.cpp", fstream::out);
	out << "#include \"globals.cpp\"\n";
	out << "#include \"univar.cpp\"\n";
	out << "struct cpppred_state;\n";
	out << "struct cpppred_state {\n"
		"int entry=0;\n"
		"vector<Thing> locals;\n"
		"coro suc,ouc;\n"
		"vector<cpppred_state> states;\n};\n";



	for(auto x: rules) {
		cppout_pred(predname(x.first), x.second);
	}



	auto qit = goal.first.find("@default");
	if (qit == goal.first.end())
		return;
	lists = add_ruleses(rules, quads2rules(goal));



	cppout_pred("query", {Rule(0, qit->second)});

	out << "#include \"cppmain.cpp\"\n";

}







//#endif
