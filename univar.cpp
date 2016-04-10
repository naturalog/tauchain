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


extern int result_limit ;

bool have_builtins = true;

map<nodeid, string> cppdict;

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









//region Thing

//for kbdbg
typedef vector<string> Markup;
typedef std::pair<nodeid, Markup> toadd;
Markup kbdbg_stack;


void kbdbgp(string s, unsigned long x)
{
	stringstream ss;
	ss << s;
	ss << x;
	kbdbg_stack.push_back(ss.str());
}
void kbdbgp(string s)
{
	kbdbg_stack.push_back(s);
}
void kbdbgpop()
{
	kbdbg_stack.pop_back();
}










enum ThingType {BOUND, NODE, OFFSET, LIST, LIST_BNODE, UNBOUND};
const vector<string> ThingTypeNames =
		{"BOUND", "NODE", "OFFSET", "LIST", "LIST_BNODE", "UNBOUND"};




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
	Markup markup; //a stack of string identifying where in the reconstructed kb text (by print_kbdbg) this Thing is
#endif

	Thing(ThingType type_, offset_t thing_) : type(type_), offset(thing_) {/*dout<<str(this) << endl;*/};
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
bool is_unbound(Thing thing)	{return (get_type(thing) == UNBOUND);}
#define is_bound(thing)		(get_type(thing) == BOUND)
#define is_list(thing)		(get_type(thing) == LIST)
#define is_list_bnode(thing)(get_type(thing) == LIST_BNODE)
#define is_node(thing)		(get_type(thing) == NODE)
#define is_var(thing)		(get_type(thing) == BOUND || get_type(thing) == UNBOUND)


//Thing::Comparison
#define types_differ(x, y) (x.type != y.type)
#define sizes_differ(x, y) (x.size != y.size)
bool are_equal(const Thing &x, const Thing &y) {return ((x).type == (y).type && (x).size == (y).size);}



//Thing::Constructors:
//3 different types of Things which can be created:
Thing create_unbound()
{
	Thing x;
	x.type = UNBOUND;
	DBG(x.thing = (Thing *)666;)
	return x;
}

Thing create_node(nodeid v)
{
	Thing x;
	x.type = NODE;
	x.node = v;
	return x;
}

Thing create_list_bnode(nodeid v)
{
	Thing x;
	x.type = LIST_BNODE;
	x.node = v;
	return x;
}



//Thing::Update
//will perhaps want to assert that 'me' is an unbound variable
void make_this_bound(Thing *me, const Thing *val)
{
	me->type = BOUND;
	me->thing = (Thing*)val;
}

//will perhaps want to assert that 'me' is a bound variable
void make_this_unbound(Thing * me)
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

/*oneword2:
<maybekoo2> so, preallocator thread can pre-resolve all offsets into variables
<maybekoo2> unbound shouldnt be identified by all zeroes but something like xxx01
<maybekoo2> so, not the same typebits pattern as bound
<maybekoo2> so, getValue only has to do one cmp instead of 3
getValue:
 00 = bound at runtime, at compile/alloc time also offset
unify:
 01 = unbound
 10 = node
 11 = list bnode(containing only remaining list size)

// hrm gotta run a couple of tests, might as well see if we can cut down the generated object code size with the right patterns? probably not
0100 = positive offset
1100 = negative offset
//gotta >> 2 , then mask off the lowest bit


 
 unify is still a bitch, could we use a switch?
 (on some aggregate value computed/masked from the two things
*/



typedef uintptr_t *Thing; // unsigned int that is capable of storing a pointer
#define databits(x) (((uintptr_t)x) & ~0b11)
#define typebits(t) ((uintptr_t)t & (uintptr_t)0b11)
static_assert(sizeof(uintptr_t) == sizeof(size_t), "damn");


//Thing::Constructors
static Thing create_unbound()
{
	return 0;
}

static Thing create_node(nodeid v)
{
	ASSERT(((uintptr_t)v & 0b11) == 0);
	return (Thing)(((uintptr_t)v<<2) | 0b01);
}

static Thing create_list_bnode(nodeid v)
{
	ASSERT(((uintptr_t)v & 0b11) == 0);
	ASSERT(false);
	return (Thing)((uintptr_t)v | 0b01);
}


//Thing::Update
static void make_this_bound(Thing * me, const Thing * v)
{
	ASSERT(((uintptr_t)v & 0b11) == 0);
	*me = (Thing)v;
}

static void make_this_unbound(Thing * me)
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
static offset_t get_offset(Thing x)
{
	uintptr_t xx = (uintptr_t)x;
	byte sign = (xx >> 1) & 0b10;
	offset_t val = xx >> 3;
	ASSERT(val);
	val -= (sign * val);
	return val;
}

static  Thing* get_thing(Thing x)
{
	return (Thing*)x;
}

static  nodeid get_node(Thing x)
{
	return (nodeid)(((uintptr_t)x)>>2);
}


//Thing::Measurement
static  size_t get_size(Thing x)
{
	return (size_t)((uintptr_t)x >> 2);
}


//Thing::Comparison
//Could use != operator
static  bool types_differ(Thing a, Thing b)
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
static  bool is_bound(Thing x)
{
	return ((((uintptr_t)x & (uintptr_t)0b11) == 0) && x);
}

static  bool is_unbound(Thing x)
{
	return x == 0;
}

static bool is_var(Thing thing) {return typebits(thing) == 0b00;}
#define is_node(thing) 		(typebits(thing) 	== 0b01)
#define is_offset(thing) 	(typebits(thing) 	== 0b10)
#define is_list(thing) 		(typebits(thing) 	== 0b11)
bool is_list_bnode(const Thing &thing) {FUN;MSG("TODO");return false;}

//General application of '=='; ostensibly general comparison
#define are_equal(x, y) (x == y)

//Thing::Access
static  ThingType get_type(Thing x)
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


 bool is_nil(const Thing* x)
{
	return is_node(*x) && (get_node(*x) == rdfnil);
}


bool is_listey(Thing *x)
{
	return is_list(*x) || is_list_bnode(*x) || is_nil(x);
}

Thing * next_item(Thing *&x)
{
	if (is_list(*x))
		x++;
	if (is_list_bnode(*x))
	{
		x += 2;
		return x - 1;
	}
	if (is_nil(x))
		return 0;
	assert(false);
}			
                                    


 void add_kbdbg_info(Thing &t, Markup markup)
{
	(void) t;
	(void) markup;
#ifdef KBDBG
	t.markup = markup;
#endif
}

//endregion


//region typedefs, globals, forward declarations

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
Rules lists_rules; // rules + query but what graphs does this include?
/*maybe this includes also lists in heads, while it should only have
lists in the default graph?*///lists in heads sounds like something we need in it too

typedef vector<pair</*bnode*/nodeid,/*first*/nodeid>> List;
typedef map</*bnode*/nodeid, pair</*first*/nodeid,/*rest*/nodeid>> Lists;
Lists lists;

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
		//why (void) these?//they are explicitly unused parameters
		(void)Ds;
		(void)Do;
		(void)_;
		switch(entry)
		{
		case 0:
			entry = LAST;
			//steps++;
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


void kbdbg_markup_str(ostream &o, Markup m)
{
	pos_t c=0;
	o << "[";
	for (auto i: m) {
		o << "\""<<i<<"\"";
		if (++c != m.size())
			o << ", ";
	}
	o << "]";
}

//Thing::Serializer::KBDBG
string kbdbg_str(const Thing * x)
{
	stringstream o;
	o << "{\"pointer\":" << "\"" << x << "\""  << ", \"markup\":";
	kbdbg_markup_str(o, x->markup);
	o << "}";
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
 void kbdbg_bind(const Thing *a, bool bind, const Thing *b)
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





//region Thing stuff

string nodestr(nodeid n)
{
	if (has(cppdict, n))
		return cppdict[n];
	else
		return *dict[n].value;
}

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
			return nodestr(node);
		}


		case LIST_BNODE: {
			const nodeid node = get_node(x);
			ASSERT(node);
			return "~" + nodestr(node) + "~";
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
		default:
			return "banana";
	}
	ASSERT(false);
}


//Thing::Access
//not sure what this does
static Thing *getValue (Thing *_x) __attribute__ ((pure));



#define getValue_profile
#ifdef getValue_profile
int getValue_BOUNDS = 0;
int getValue_OFFSETS = 0;
int getValue_OTHERS = 0;
#endif



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


/*
	if (is_unbound(x) || is_node(x))
	{
		#ifdef getValue_profile
			getValue_OTHERS++;
		#endif
		return _x;
	}
	else
	if (is_bound(x))
		{
		
			#ifdef getValue_profile
				getValue_BOUNDS++;
			#endif

			Thing * thing = get_thing(x);
			ASSERT(thing);
			return getValue(thing);
	
		}
	}
	else
	{
		ASSERT(is_offset(x));
		#ifdef getValue_profile
			getValue_OFFSETS++;
		#endif
		const offset_t offset = get_offset(x);
		Thing * z = _x + offset;
		return getValue(z);
	}

*/

/*
	if (!is_offset(x))
	{
		if (!is_bound(x))
		{
			#ifdef getValue_profile
				getValue_OTHERS++;
			#endif
			return _x;
		}
		else
		{
		
			#ifdef getValue_profile
				getValue_BOUNDS++;
			#endif

			Thing * thing = get_thing(x);
			ASSERT(thing);
			return getValue(thing);
	
		}
	}
	else
	{
		ASSERT(is_offset(x));
		#ifdef getValue_profile
			getValue_OFFSETS++;
		#endif
		const offset_t offset = get_offset(x);
		Thing * z = _x + offset;
		return getValue(z);
	}
	assert(false);

*/
	

	//Is a bound variable, return the value of it's value.
	if (is_bound(x)) {
		#ifdef getValue_profile
		getValue_BOUNDS++;
		#endif
		//get the pointer
		Thing * thing = get_thing(x);
		ASSERT(thing);
		//and recurse
		return getValue(thing);
	}
	else if (!is_offset(x))
	{	//Is either an unbound variable or a value.
		#ifdef getValue_profile
		getValue_OTHERS++;
		#endif
		return _x;
	}
	else
	{	
		ASSERT(is_offset(x));
		#ifdef getValue_profile
		getValue_OFFSETS++;
		#endif
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
		//TRACE(dout << "unify with [" << argv << "]" << str(argv) << endl;)



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
						#ifdef KBDBG
						kbdbg_bind(origa, true, origb);
						#endif

						TRACE(dout << "binding [" << me << "]" << str(me) << 
							" to [" << argv << "]" << str(argv) << endl;)
						ASSERT(is_unbound(*me));
						make_this_bound(me, argv);
						entry = LAST;
						return true;
					}
				//on the second time around it will unbind it,
				//and that's supposed to be it:
				//me(argv) -> me()
					case_LAST:
						#ifdef KBDBG
						kbdbg_bind(origa, false, origb);
						#endif

						TRACE(dout << "unbinding [" << me << "]" << str(me)/* << " from [" << argv << "]" << str(argv)*/ << endl;)
						//argv shouldnt be touched here anymore, it could be a gone constant on stack
						ASSERT(is_bound(*me));
						make_this_unbound(me);
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

	//this_ = getValue(this_);

	const Thing me = *this_;
	const Thing x = *x_;
	//We're sure it won't be these? in the context of ep-checking, yes
	//We're asserting these because we assume we've done a getValue() prior to calling
	//we could just do the getValue() here and then we don't have to worry about doing it
	//prior to calling, but i guess we have to getValue() for that anyway
	
	 
	ASSERT(!is_offset(me));
	ASSERT(!is_offset(x));
	ASSERT(!is_bound(x));
	
	if(is_var(me) && is_var(x))
/*was:var,is:node => not ep*/

/*so, if i was called with a var and now im being called with a node, its not an ep, its more specific
i dunno

so, if we consider two cases where in the first one we call a pred with a node (in the subject or object, it doesn't matter), and the second one we call it with a var
both cases the pred will try to run over all the rules
the case of var will unify with everything that the case with the node will unify with
ah, but it might not be able to reach ground unless you let it call it with more specific cases
right, so your idea is right i guess, at the very at least it still doesn't allow infloops and shouldn't add any incorrect semantics, just perhaps redundancy at worst

*/

	//if (is_unboud(me) && is_unbound(x))

//	if (is_var(me))
		return true;// we must have been an unbound var
/*was:var,is:node => ep*/



//node would be the same
	else if (is_node(me))
		return are_equal(me, x);
//list would change the way var changed, i.e. vars in the list could only "unify" with vars



//shouldnt getValue the items here!
	else if (is_list(me)) {
		if (is_list(x)) {
			if (get_size(*this_) != get_size(x))
				return false;
			/*
			const auto size = get_size(me);
			for (pos_t  i = 0; i < size; i++)
				if (!would_unify(getValue(this_+1+(i*2)) , getValue(x_+1+(i*2))))
					return false;
			return true;
			*/

			x_++;
		}
		this_++;
	}
	if ((is_list_bnode(*this_) || is_nil(this_)) && (is_list_bnode(*x_) || is_nil(x_))) {
		do {
			if (is_nil(this_)) {
				if (is_nil(x_))
					return true;
				return false;
			}
			if (is_nil(x_))
				return false;

			if (!would_unify(getValue(this_+1), getValue(x_+1)))
				return false;
			this_+=2;
			x_+=2;
		} while (true);
	}
//and this would stay the same
//what else can me be anyway?
//value of anything can be either an unbound var, a node, or list stuff.
//so this is for example trying to unify a list with a node
	else if (types_differ(me, x)) // in oneword mode doesnt differentiate between bound and unbound!
		return false;
	assert(false);
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

//endregion



#include "sprint.cpp"




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

	kbdbgp("pred",pr);

	//builtins are ready to go
	if (have_builtins)
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
		for (pos_t i = rules.at(pr).size(); i > 0; i--)
		{
			kbdbgp("rule",i-1);
			add_rule(pr, compile_rule(rules.at(pr)[i-1]));
			kbdbgpop();
		}
	}
	
	//rdfs:SubPropertyOf
	if (have_builtins)
		add_rule(pr, make_wildcard_rule(pr));

	kbdbgpop();

}



void check_pred(nodeid pr)
{
	FUN;
	if (rules.find(pr) == rules.end() && builtins.find(pr) == builtins.end()) {
		dout << "'" << dict[pr] << "' not found." << endl;
		if (have_builtins)
			preds[pr] = make_wildcard_rule(pr);
		else
			preds[pr] = GEN_FAIL_WITH_ARGS;
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

		//reverse the list of quads in this graph
		//is it going now opposite to text-order?
		//reverse(pffft.begin(), pffft.end());
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

//massiively reworked for much simplicity, speed and correctness
void collect_lists() {
	FUN;
	MSG("...");
	lists.clear();

	//Rule rule: vector<Rule>
	for (auto rule: lists_rules[rdffirst])
		lists[dict[rule.head->subj]].first = dict[rule.head->object];
	for (auto rule: lists_rules[rdfrest])
		lists[dict[rule.head->subj]].second = dict[rule.head->object];
}


qdb*mykb;

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
	lists_rules = rules;//we dont have any query at this point
	collect_lists();

	kbdbgp("kb");

	mykb = &kb;

	if (have_builtins)
		for(auto x: builtins)
			compile_pred(x.first);

	for(auto x: rules)
		compile_pred(x.first);
		
	kbdbgpop();
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


	while(true) {
		auto v1 = a_ + 1;
		auto v2 = b_ + 1;

		r = unifjoin(v1, v2, r);

		a_ += 2;
		b_ += 2;
		a = *a_;
		b = *b_;

		if (is_nil(a_) || is_nil(b_)) {
			if (is_nil(a_) && is_nil(b_))
				return r;
			return GEN_FAIL;//one is shorter
		}
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


coro unify(Thing *a_, Thing *b_){
	FUN;
	unifys++;

	//for logging
	DBG(Thing *origa = a_;)
	DBG(Thing *origb = b_;)


	TRACE(dout << str(a_) << " X " << str(b_) << endl;)


	if (a_ == b_) {//i added this one of my own invention btw
	//i'll need to think of which cases this covers but it looks good
		TRACE(dout << "a == b" << endl;)
	    	//orig, origb only exist #ifdef DEBUG
		//ifndef KBDBG then this will be gen_succeed();
		return UNIFY_SUCCEED(origa, origb);
	}

	//Get the "representative value" of a_//also here we getValue too
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
		return unboundunifycoro(b_, a_//origa?
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
	if (is_list(a) || is_list(b)) {
		auto xx = a_;
		auto yy = b_;
		if (is_list(*xx))
			xx += 1;
		if (is_list(*yy))
			yy += 1;
		if (is_list_bnode(*xx) && is_list_bnode(*yy))
			return listunifycoro2(xx, yy);
		if (
				(is_node(*xx) && (get_node(*xx) == rdfnil))
				||
				(is_node(*yy) && (get_node(*yy) == rdfnil))
				)
		{
			if (
					(is_node(*xx) && (get_node(*xx) == rdfnil))
					&&
					(is_node(*yy) && (get_node(*yy) == rdfnil))
					)
				return UNIFY_SUCCEED(origa, origb);
			else
				return UNIFY_FAIL(origa, origb);
		}
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
PredParam kbdbg_find_thing (pos_t  &index, Locals &locals)
{
	pos_t  r = 0;
	for(auto i: locals)
	{
		if (i.markup == kbdbg_stack) {
			index = r;
			return LOCAL;
		}
		r++;
	}	
	dout << "want: ";
	kbdbg_markup_str(dout, kbdbg_stack);
	dout << endl << "got:" << endl;
	for(auto i: locals)
	{
		kbdbg_markup_str(dout, i.markup);
		dout << endl;
	}
	dout << "." << endl;
	
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
	dout << locals.size() << ", " << consts.size() << endl;
	dout << endl << "locals map: \n nodeid\t\tpos\t\tthing\t\tkbdbg" << endl;
	for (auto x: lm)
	{
		dout << " " << *dict[x.first].value << "\t\t";
		dout << x.second << "\t\t";
		dout << str(&locals.at(x.second));
#ifdef KBDBG
		dout << "\t\t" << kbdbg_str(&locals.at(x.second));
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
	return has(lists, x);
}

List get_list(nodeid n)
{
	FUN;
	ASSERT(n);
	MSG(n);
	List r;
	while (n != rdfnil) {
		nodeid first = lists.at(n).first;
		r.push_back(pair</*bnode*/nodeid,/*first*/nodeid>(n, first));
		n = lists.at(n).second;
	}
	return r;
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
void make_locals(Locals &locals, Locals &consts, locals_map &lm, locals_map &cm, pquad head, pqlist body, bool head_goes_into_locals=true)
{
	FUN;

  //queue to load up our lists into.
	std::queue<toadd> lq;
  	//TRACE(dout << "head:" << format(head) << endl);

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
				//m.push_back(list_part++);
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
	//unsigned long old_kbdbg_part = kbdbg_part;

	//Make a toadd for both the subject and object for each term in the 
	//rule (both head & body), and push these into terms vector.
	//Increment kbdbg_part for each node added to terms, place this
	//value into a vector, and set that as the Markup for the toadd.
	//What's the Markup doing?
	if (head) 
	{
		kbdbgp("head");
		kbdbgp("subject");
		terms.push_back(toadd(dict[head->subj], kbdbg_stack));
		kbdbgpop();
		kbdbgp("object");
		terms.push_back(toadd(dict[head->object], kbdbg_stack));
		kbdbgpop();
		kbdbgpop();
	}
	
	if(body)
	{

	kbdbgp("body");
	
	unsigned long i=0;	
	for (pquad bi: *body) 
	{
		kbdbgp("item",i++);
		kbdbgp("subject");
		terms.push_back(toadd(dict[bi->subj], kbdbg_stack));
		kbdbgpop();
		kbdbgp("object");
		terms.push_back(toadd(dict[bi->object], kbdbg_stack));
		kbdbgpop();
		kbdbgpop();
	}
	kbdbgpop();
	
	}
	
	TRACE(dout << "terms.size:" << terms.size() << endl);

	//For all our terms (toadds) in terms, if the term is not
	//a variable or a list, then "add_node(false, xx, locals, lm)".
	//If the term is a variable, then "add_node(true, xx, locals, lm)".
	//If it's a list, then push it to lq to be processed later.
	//std::pair<nodeid,Markup>



	for (toadd xx: terms) 
	{
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
			if (head_goes_into_locals && head && (x == dict[head->subj] || x == dict[head->object]))
				add_node(false, xx, locals, lm);
		//If it's not a var, not a list, and is not in the head, then
		//put it in consts. Why?
			else
				add_node(false, xx, consts, cm);
#else
			//And why #ifdef KBDBG they both go into locals? simplicity
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

	auto max = b2.size();
	kbdbgp("body");


	//For every term in the body, check the pred to see if there's a head
	//with this pred. If not we'll set the pred-function for that pred to
	//fail_with_args. 
	for (pquad bi: b2)
	{
	
		kbdbgp("item",--max);
	
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
			
			kbdbgp("subject");
			sk = maybe_head(kbdbg_find_thing(i1, locals), head, s);
			kbdbgpop();
			kbdbgp("object");
			ok = maybe_head(kbdbg_find_thing(i2, locals), head, o);
			kbdbgpop();
			
#else
			(void) locals;


			sk = maybe_head(find_thing(s, i1, lm, cm), head, s);
			ok = maybe_head(find_thing(o, i2, lm, cm), head, o);
#endif
		}

		TRACE(dout <<"perm " << permname.at(sk) << " " << permname.at(ok) << endl );

		jg = perms.at(sk).at(ok)(dict[bi->pred], jg, i1, i2, consts);
		//typedef function<join_gen(pred_gen, join_gen, pos_t , pos_t , Locals&)>
		
		kbdbgpop();
		
	}

	kbdbgpop();

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
          ancestors is the same pred with some s/o that are_equivalent with the current s/o.

are_equivalent(unbound u, unbound v) = true
are_equivalent(node a, node b) = node a == node b
are_equivalent(list a, list b) = list_equal(a,b)




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


//or say s & o are vars bound to other things

//The rule will get pointers to s/o as args

//find_ep(ep,s,o);

//This will be passing these pointers to find_ep which will accept them as pointers
#define EPDBG(x) 

bool find_ep(ep_t *ep, /*const*/ Thing *s, /*const*/ Thing *o)
{
	FUN;
	//hrm
	//wouldn't this overwrite the pointer to the Things with a pointer to their values? sure
	//wouldn't this carry on beyond this ep-check though? no
	//they won't be the same pointers? i.e. s/o here will be different s/o pointer-objects than
	//those in the rule that called it, just pointing to the same place? yes
	
	//if we move getValue() inside of would_unify we don't need to do this though
	//we should be moving stuff out of inner loops, not into them, fair enough
	
	s = getValue(s);
	o = getValue(o);
	
	ASSERT(!is_offset(*s));
	ASSERT(!is_offset(*o));
	//what about !is_bound

	EPDBG(dout << endl << endl << ep->size() << " ep items." << endl);
	//thingthingpair
	for (auto i: *ep)
	{
	  //Thing*
		auto os = i.first;
		auto oo = i.second;
		ASSERT(!is_offset(*os));
		ASSERT(!is_offset(*oo));
		//what about !is_bound
		
		//TRACE(dout << endl << " epitem " << str(os) << "    VS     " << str(s) << endl << str(oo) << "    VS    " << str(o) << endl;)
		EPDBG(dout << endl << " epcheck " << str(os) << "    VS     " << str(s) << endl << " epcheck " << str(oo) << "    VS    " << str(o) << endl;)

		bool rs = false;
		bool ro = false;

		auto suc = unify(s, os);
		while(suc())
		    rs = true;

		auto ouc = unify(o, oo);
		while(ouc())
		    ro = true;

		if (rs && ro)
		{
			EPDBG(dout << endl << " epcheck " <<  "EP." << endl;)
			return true;
		}
		
		
		/*
		if (would_unify(os,s))
		{
			//TRACE(dout << ".." << endl);
			if(would_unify(oo,o)) {
				EPDBG(dout << endl << " epcheck " <<  "EP." << endl;)
				dout << endl << "EP!" << endl;
				return true;
			}
		}
		*/
		EPDBG(dout << endl <<  " epcheck " << "---------------------" << endl);

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
	kbdbgp("head");
	kbdbgp("subject");
	kbdbg_find_thing(hs, locals_template);
	kbdbgpop();
	kbdbgp("object");
	kbdbg_find_thing(ho, locals_template);
	kbdbgpop();
	kbdbgpop();
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

	Thing *sv, *ov;
	/*
	Thing st, ot;
	Thing * const stp = &st;
	Thing * const otp = &ot;
	*/
/*added that yesterday^im mulling over this myself, where do we getValue and why, ...also do we want to be passing
s and o in params..but thats for cppout... but the getValues and interplay with unify (and ep ofc)
i have no idea */


	return [ov, sv,/* st, ot, stp, otp,*/ has_body, locals_bytes, locals_data, ep, hs, ho, locals ,&consts, jg, suc, ouc, j, entry TRCCAP(call) TRCCAP(r)](Thing *s, Thing *o) mutable {
		setproc("rule");
		TRC(++call;)
//		TRACE(dout << op->formatr(r) << endl;)
		TRACE(dout << "call=" << call << endl;)
		switch (entry) {
			case 0:

				/*optimization: this could happen in a different thread.
				  http://liblfds.org/
				 https://github.com/facebook/folly
				 * http://moodycamel.com/blog/2013/a-fast-lock-free-queue-for-c++
				http://www.drdobbs.com/parallel/writing-lock-free-code-a-corrected-queue/210604448
				 http://www.boost.org/doc/libs/1_54_0/doc/html/boost/lockfree/spsc_queue.html
				 //lets first try to find some offtheshelf solution, boost? sure
				 https://isocpp.org/wiki/faq/cpp11-language-concurrency
				 
				cpu affinity should be set.*/
				locals = (Thing*)malloc(locals_bytes);
				memcpy(locals, locals_data, locals_bytes);
				/*also, since when a pred is invoked, all rules are gone thru,
				 * we should try (as long as this isnt too memory-intensive) allocating per-pred/on some granularity.

				 * allocating as late as possible:
				 * either by adding one more var between the s/o parameters and their locals counterparts,
				 * or
				 * leveraging would_unify
				 * -would_unify and unify could be made to work together with minimal overhead
				 * would_unify, maybe_unify, surely_unify?
				 */



				//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
				ASSERT(hs < locals_bytes / sizeof(Thing));
				ASSERT(ho < locals_bytes / sizeof(Thing));
				
				// i mean up here
				
				//i *think* we can safely move these
				
			
				ov = getValue(o);
				sv = getValue(s);
						
				//and here i feel like the ep-check should be before
				//the unifys

				if (find_ep(ep, sv, ov)) {
					goto end;
				}else{
					/*
					if(is_var(*ov)){
						ot = create_unbound();
					}else if(is_node(*ov)){
						ot = create_node(ov->node);
					}else{ASSERT(false);}

					if(is_var(*sv)){
						st = create_unbound();
					}else if(is_node(*sv)){
						st = create_node(sv->node);
					}else{ASSERT(false);}
					*/
					ep->push_back(thingthingpair(sv,ov));	
				}
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

						steps++;
						if (!(steps&0b11111111111111111))
							dout << steps << " steps." << endl;

						j = jg();
						while (j(sv, ov, locals)) {
							TRACE(dout << "After c0() -- " << endl;)
							//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)

							ASSERT(ep->size());
							ep->pop_back();
					

							TRACE(dout << "MATCH." << endl;)
							entry = LAST;
							return true;

			case_LAST:;
							TRACE(dout << "RE-ENTRY" << endl;)
							ep->push_back(thingthingpair(sv, ov));
						}

				
			
				
					}
				}


				ASSERT(ep->size());
				ep->pop_back();
				
			end:;



				TRACE(dout << "DONE." << endl;)
				free(locals);
			END
		}
	};
}



//endregion











//region kbdbg
#ifdef KBDBG


void print_kbdbg_part(stringstream &o, pnode n)
{
	o << "{\"text\":";


/*	
if (islist(n)) {
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
else*/
	
	o << "\"" << dstr(dict[n], true) << "\"" << ", \"markup\":";
	kbdbg_markup_str(o, kbdbg_stack);
	o << "}";
}

void print_kbdbg_term(stringstream &o, pquad t)
{
	kbdbgp("subject");
	print_kbdbg_part(o, t->subj);
	kbdbgpop();

	o << ",\" " << dstr(dict[t->pred], true) << " \",";
	
	kbdbgp("object");	
	print_kbdbg_part(o, t->object);
	kbdbgpop();

}

void print_kbdbg_termset(stringstream &o, pqlist b)
{
	size_t i = 0;
	for (auto bi: *b) {
		kbdbgp("item",i);
		print_kbdbg_term(o, bi);
		if (i++ != b->size() - 1)
			o << ", \". \", ";
		kbdbgpop();
	}
}


void print_kbdbg(pqlist query)
{
	kbdbgp("kb");
	for (auto rs: rules) {
		kbdbgp("pred", rs.first);
		unsigned long ruleid = 0;
		for (auto rule: rs.second) {
			kbdbgp("rule", ruleid);
			kbdbgp("head");
			stringstream o;
			pquad h = rule.head;
			o << "\"{\",";
			print_kbdbg_term(o, h);
			o << ",\"}\"";
			kbdbgpop();
			kbdbgp("body");			
			auto b = rule.body;
			if (b&&b->size()) {
				o << ",\" <= {\",";
				print_kbdbg_termset(o, b);
				o << ",\"}\"";
			}
			kbdbgpop();
			o << ",\"\\n\"";
			dout << "{\"type\":\"kb\", \"value\":[" << o.str() << "]}" << endl;
	
			ruleid++;
			kbdbgpop();		
		}
		kbdbgpop();
	}
	kbdbgpop();
	kbdbgp("query");
	kbdbgp("body");			
	stringstream o;
	print_kbdbg_termset(o, query);
	dout << "{\"type\":\"query\", \"value\":[" << o.str() << "]}" << endl;
	kbdbgpop();
	kbdbgpop();
}
#endif
//endregion




//region interface



pnode thing2node(Thing *t, qdb &r) {
	FUN;
	if (is_list(*t))
	{
		t += 1;//jump to the first bnode
		pnode first = 0;
		pnode previous = 0;
		while(true) {
			assert(is_list_bnode(*t) || is_node(*t));
			nodeid n = get_node(*t);
			assert((n == rdfnil || is_list_bnode(*t)));
			pnode bn = thing2node(t, r);

			if (!first)first = bn;
			if (previous) {
				r.first["@default"]->push_back(
						make_shared<quad>(
								quad(
										previous,
				std::make_shared<node>(dict[rdfrest]),
						bn
				)
				)
				);
			}

			if (n == rdfnil)
				break;

			previous = bn;

			Thing *v = getValue(t + 1);

			r.first["@default"]->push_back(
					make_shared<quad>(
							quad(
									bn,
			std::make_shared<node>(dict[rdffirst]),
					thing2node(v, r)
			)
			)
			);
			t += 2;
		}
		assert(first);
		return first;
	
	}

	if (is_node(*t) || is_list_bnode(*t))
		return std::make_shared<node>(dict[get_node(*t)]);


	MSG(str(t));
	//dout << "thing2node: Wtf did you send me?, " << str(t_) << endl;
	assert(is_unbound(*t));
	return mkiri(pstr("?V?A?R?"));
	assert(false);
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
	FUN;
	MSG("...");
	make_perms_table();
	build_in_facts();
	build_in_rules();
	compile_kb(kb);
}


yprover::~yprover()
{
	free_garbage();
}


//arg1: kb, arg2: query
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

#ifdef KBDBG
	print_kbdbg(qit->second);
#endif

	//Reset the global steps & unifys
	//Why not turn these into member variables of yprover.
	//because then all the functions where they are touched and subsequently everything would have to be in yprover.
	steps = 0;
	unifys = 0;
	int nresults = 0;


	locals_map lm, cm;
	Locals locals, consts;

	dout << KGRN << "COMPILE QUERY" << KNRM << endl;
	//here we combine the two Rules maps, and lists will
	//be used by get_list and islist deep inside make_locals
	//get_list or something should then remove the triples of the internalized lists from the query

	auto gr = quads2rules(goal);
	//Adds the rules from the query to the rules from the kb? just for the lists
	lists_rules = add_ruleses(rules, gr);
	collect_lists();

	qlist q;
	for (auto qu: *qit->second) {
		MSG(qu->auto_added << ":" << qu);
		if (!qu->auto_added)
			q.push_back(qu);
	}
	shared_ptr<qlist> pq = make_shared<qlist>(q);

	kbdbgp("query");

	make_locals(locals, consts, lm, cm, 0, pq);
	join_gen jg = compile_body(locals, consts, lm, cm, 0, pq);

	kbdbgpop();


	join_t coro = jg();

	dout << KGRN << "RUN" << KNRM << endl;

	//invoke Satan our Lord
	while (coro( (Thing*)666,(Thing*)666, locals.data() )) {

		//Returned true, so found a result: the rest of this loop is handling the result.
		nresults++;
		dout << KCYN << "RESULT " << KNRM << nresults << ":";
		qdb r;
		r.first["@default"] = mk_qlist();

		//go over the triples of the query to print them out
		//*q  :: qlist
		//i   :: pquad

		for(auto i: q)
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

			s = getValue(s);
			o = getValue(o);

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

	dout << "That's all, folks, " << nresults << " results." << endl;

  //what does this do//this is a goto label
	out:;

	dout << unifys << " unifys, " << steps << " steps." << endl;
	steps_ = steps;
	unifys_ = unifys;


	if (log_outputString.size()) {
		dout << "log#outputString:" << endl;
		for (const auto x:log_outputString)
			dout << x.first << ": " << x.second << endl;
		log_outputString.clear();
	}

}

//endregion


#include "builtins.cpp"

#include "cppout.cpp"
