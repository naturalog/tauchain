#include <functional>
#include <unordered_map>
#include <queue>
#include "univar.h"
#include <string.h>

using namespace std;
using namespace old;

typedef intptr_t offset_t;//ptrdiff_t

extern int result_limit ;

#define FUN setproc(__FUNCTION__);

#define EEE char entry = 0
const char LAST = 33;

#ifdef DEBUG
#define DBG(x) x
#define TRC(x) x
#define TRCCAP(x) ,x
//#define ITEM(x,y) x.at(y)
#define ITEM(x,y) x[y]
#define ASSERT assert
#define case_LAST case LAST
#define END entry = 66; return false; default: ASSERT(false);
#else
#define DBG(x)
#define TRC(x)
#define TRCCAP(x)
#define ITEM(x,y) x[y]
#define ASSERT(x)
#define case_LAST default
#define END return false;
#endif

#ifdef NEW
#define oneword
#endif

#define is_unbound(x) (!is_bound(x))
enum ThingType {BOUND, NODE, OFFSET, LIST, UNBOUND};

#ifndef oneword

/*so, the idea behind this new data structuring is that each little thing doesnt have to be allocated separately,
we can put them in one big array per rule, and we can initialize such locals array from a template simply by memcpy
most importantly, this is just an attempt at optimization, it isnt a change needed for the correct functioning

	UNBOUND, 	// unbound var
	BOUND,  	// bound var
	NODE, 		// nodeid - atom
	LIST, 		// has size, is followed by its items
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
	ThingType type;
	union {
		Thing *thing;     // for bound var
		old::termid term; // for node
		size_t size;      // for list
		offset_t offset;
	};
};

#define get_type(thing) ((thing).type)
#define get_term(thing) ((thing).term)
#define get_size(thing) ((thing).size)
#define get_thing(ttttt) ((ttttt).thing)
#define get_offset(thing) ((thing).offset)
#define is_offset(thing) (get_type(thing) == OFFSET)
#define is_bound(thing)  (get_type(thing) == BOUND)
#define is_var(thing)  (get_type(thing) == BOUND || get_type(thing) == UNBOUND)
#define is_list(thing)  (get_type(thing) == LIST)
#define is_node(thing)  (get_type(thing) == NODE)
#define types_differ(x, y) (x.type != y.type)
#define sizes_differ(x, y) (x.size != y.size)
#define are_equal(x, y) (x.type == y.type && x.size == y.size)

static inline Thing create_bound(Thing * val)
{
	Thing x;
	x.type = BOUND;
	x.thing = val;
	return x;
}

static inline void make_this_unbound(Thing * me)
{
	me->type = UNBOUND;
}


#else

typedef unsigned char byte;
typedef uintptr_t *Thing;
static_assert(sizeof(uintptr_t) == sizeof(size_t), "damn");

/* oneword:
kinda like http://software-lab.de/doc/ref.html#cell

 00 = var        // address left intact or zero
 01 = node       // we mostly just compare these
010 = positive offset
110 = negative offset
 11 = list(size)
*/

static inline Thing create_bound(Thing * val)
{
	ASSERT(((size_t)val & 0b11) == 0);
	return (Thing)val;
}

static inline void make_this_unbound(Thing * me)
{
	*me = 0;
}

static inline offset_t get_offset(Thing x)
{
	long xx = (long)x;
	byte sign = (xx >> 1) & 2;
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
	return (termid)(((uintptr_t)x) & ~0b11);
}

static inline bool types_differ(Thing a, Thing b)
{
	return (((long)a ^ (long)b) & 0b11);
}

#define sizes_differ(x, y) (x != y)

static inline bool is_bound(Thing x)
{
	return (((x & 0b11) == 0) && x);
}

#define is_var(thing) (!(thing & 0b11))
#define is_node(thing) ((thing & 0b11) == 1)
#define is_offset(thing) ((thing & 0b11) == 0b10)
#define is_list(thing) ((thing & 0b11) == 0b11)
#define are_equal(x, y) (x == y)

static inline ThingType get_type(Thing x)
{
	if(is_bound(x))
		return BOUND;
	if(is_unbound(x))
		return UNBOUND;
	if(is_list(x))
		return LIST;
	if(is_node(x))
		return NODE;
	if(is_offset(x))
		return OFFSET;
}

#endif



//region types, globals, funcs


typedef vector<Thing> Locals;

typedef std::pair</*const*/ Thing*,/*const*/ Thing*> thingthingpair;
typedef std::vector<thingthingpair> ep_t;

typedef function<bool()> coro;
typedef function<bool(Thing*,Thing*)> pred_t;
typedef function<pred_t()> pred_gen;
typedef function<bool(Thing*,Thing*)> rule_t;
typedef function<rule_t()> rule_gen;
typedef function<bool(Thing*,Thing*, Thing*)> join_t;
typedef function<join_t()> join_gen;
//btw im using gen in the sense that its a lambda generating another lambda
typedef function<join_gen(nodeid, join_gen, size_t, size_t, Locals&)>  join_gen_gen;

enum PredParam {HEAD_S, HEAD_O, LOCAL, CONST};
typedef map<PredParam, map<PredParam, join_gen_gen>> Perms;



Perms perms;
map<PredParam, old::string> permname;
map<nodeid, vector<size_t>> pred_index;
#ifdef DEBUG
std::map<old::nodeid, pred_t> preds;
typedef map<old::termid, size_t> locals_map;
#else
std::unordered_map<old::nodeid, pred_t> preds;
typedef unordered_map<old::termid, size_t> locals_map;
#endif
old::prover *op;
std::vector<ep_t*> eps;
vector<Locals*> constss;
vector<Locals*> locals_templates;
long steps = 0;
long unifys = 0;


coro unbound_succeed(Thing *x, Thing *y);
coro unify(Thing *, Thing *);
void check_pred(old::nodeid pr);
rule_t compile_rule(termid head, prover::termset body);
pred_t compile_pred(old::nodeid pr);





//endregion



//region succeed and fail



//yield once
static coro gen_succeed()
{
	EEE;
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

static join_t succeed_with_args()
{
	EEE;
	return [entry](Thing *Ds, Thing *Do, Thing* _) mutable{
		(void)Ds;
		(void)Do;
		(void)_;
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

static join_gen succeed_with_args_gen()
{
	return []() {
		return succeed_with_args();
	};
}

#ifndef DEBUG

static bool fail()
{
	setproc(L"fail");
	TRACE(dout << "..." << endl;)
	return false;
}

static bool fail_with_args(Thing *_s, Thing *_o)
{
	(void)_s;
	(void)_o;
	setproc(L"fail_with_args");
	TRACE(dout << "..." << endl;)
	return false;
}

#define GEN_FAIL (fail)
#define GEN_FAIL_WITH_ARGS (fail_with_args)

#else

coro dbg_fail()
{
	int entry = 0;
	return [entry]() mutable{
		setproc(L"dbg_fail lambda");
		TRACE(dout << "..." << endl;)

		switch(entry)
		{
		case 0:
			entry = 666;
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
		setproc(L"dbg_fail_with_args lambda");
		TRACE(dout << "..." << endl;)

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

#endif





//endregion




wstring str(const Thing *_x)
{
	Thing x = *_x;
	switch (get_type(x)) {
		case BOUND: {
			const Thing *thing = get_thing(x);
			ASSERT(thing);
			return L"var(" + str(thing) + L")";
		}
		case UNBOUND:
			return L"var()";
		case NODE: {
			const termid term = get_term(x);
			ASSERT(term);
			return op->format(term);
		}
		case LIST: {
			const size_t size = get_size(x);
			wstringstream r;
			r << L"{" << size << L"}(";
			for (size_t i = 0; i < size; i++) {
				if (i != 0) r << " ";
				r << str(_x + 1 + i);
			}
			if (!size)
				r << " ";
			return r.str() + L")";
		}
		case OFFSET: {
			const offset_t offset = get_offset(x);
			wstringstream r;
			r << L"{";
			if (offset >= 0)
				r << L"+";
			r << offset << L"}->";
			r << str(_x + offset);
			return r.str();
		}
	}
	ASSERT(false);
}

static Thing *getValue (Thing *_x) __attribute__ ((pure));
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

	if (is_bound(x)) {
		Thing * thing = get_thing(x);
		ASSERT(thing);
		return getValue(thing);
	}
	else if (is_offset(x))
	{
		const offset_t offset = get_offset(x);
		Thing * z = _x + offset;
		/*
		#ifdef oneword
		*_x = z;
		#else
		_x->type = BOUND;
		_x->thing = z;
		#endif
		*/
		return getValue(z);
	}
	else
		return _x;
}






/*  # If this Variable is bound, then just call YP.unify to unify this with arg.
	# (Note that if arg is an unbound Variable, then YP.unify will bind it to
	# this Variable's value.)
	# Otherwise, bind this Variable to YP.getValue(arg) and yield once.  After the
	# yield, return this Variable to the unbound state.
	# For more details, see http://yieldprolog.sourceforge.net/tutorial1.html */
function<bool()> unboundunifycoro(Thing * me, Thing *arg)
{
		TRACE(dout << "!Bound" << endl;)
		Thing *argv = getValue(arg);
		TRACE(dout << "value=" << argv << "/" << str(argv) << endl;)

		if (argv == me) {
			TRACE(dout << "argv == me" << endl;)
			//# We are unifying this unbound variable with itself, so leave it unbound.^M
#ifdef DEBUG
			return unbound_succeed(me, argv);
#else
			return gen_succeed();
#endif
		}
		else {
			TRACE(dout << "argv != me" << endl;)

			EEE;
			return [me, entry, argv]() mutable {
				setproc(L"unify lambda 2");
				TRACE(dout << "im in ur var unify lambda, entry = " << entry << ", argv=" << argv << "/" <<
					  str(argv) << endl;)
				switch (entry) {
					case 0: {
						TRACE(dout << "binding " << me << "/" << str(me) << " to " << argv << "/" << str(argv) << endl;)
						ASSERT(is_unbound(*me));
						Thing n = create_bound(argv);
						*me = n;
						entry = LAST;
						return true;
					}
					case_LAST:
						TRACE(dout << "unbinding " << me << "/" << str(me) << endl;)
						ASSERT(is_bound(*me));
						make_this_unbound(me);
						END
				}
			};
		}
	}


bool would_unify(Thing *this_, Thing *x_)
/*ep check is supposed to determine equality by seeing if the values would unify
* (but not actually doing the unifying assignment)*/
{
		FUN;
		const Thing me = *this_;
		const Thing x = *x_;
		ASSERT(!is_offset(me));
		ASSERT(!is_offset(x));
		ASSERT(!is_bound(x));
		if (is_var(me))
			return true;// we must have been an unbound var
		else if (types_differ(me, x)) // in oneword mode doesnt differentiate between bound and unbound!
			return false;
		else if(is_node(me))
		{
			/*const auto this_term = get_term(this);
			const auto x_term = get_term(x);
			TRACE(dout << op->format(this_term) << " =?= " << op->format(x_term) << endl;)*/
			bool r = are_equal(me, x);
			ASSERT(op->_terms.equals(get_term(me), get_term(x)) == r);
			return r;
		}
		else
		{
			ASSERT(is_list(me));
			const auto size = get_size(me);
			if(size != get_size(x)) return false;
			for (size_t i = 1; i <= size; i++) 
				if (!would_unify(getValue(this_+i),getValue(x_+i)))
					return false;
			return true;
		}
}



#ifdef DEBUG
coro unbound_succeed(Thing *x, Thing *y)
{
	EEE;
	return [entry, x, y]() mutable {
		ASSERT(is_unbound(*x));
		setproc(L"unbound_succeed lambda");
		TRACE(dout << x << " " << y << "entry:" << entry << endl);
		switch (entry) {
			case 0:
				entry = LAST;
				return true;
			case_LAST: END
		}
	};
}
#endif



//region sprint

wstring sprintVar(wstring label, Thing *v){
	wstringstream wss;
	wss << label << ": (" << v << ")" << str(v);
	return wss.str();
}

wstring sprintPred(wstring label, old::nodeid pred){
	wstringstream wss;
	wss << label << ": (" << pred << ")" << old::dict[pred];
	return wss.str();
}

wstring sprintThing(wstring label, Thing *t){
	wstringstream wss;
	wss << label << ": [" << t << "]" << str(t);
	return wss.str();
}

wstring sprintSrcDst(Thing *Ds, Thing *s, Thing *Do, Thing *o){
	wstringstream wss;
	wss << sprintThing(L"Ds", Ds) << ", " << sprintThing(L"s",s) << endl;
	wss << sprintThing(L"Do", Do) << ", " << sprintThing(L"o",o);
	return wss.str();
}


//endregion



//region kb

void free_eps()
{
	for (auto x: eps)
	{
		ASSERT(!x->size());
		delete x;
	}
	eps.clear();

	for (auto x: locals_templates)
		delete x;
	locals_templates.clear();

	for (auto x: constss)
		delete x;
	constss.clear();
}

void free_eps_nonassert()
{
	for (auto x: eps)
		delete x;
	eps.clear();
}

void compile_kb()
{
	FUN
	TRACE(dout << "# of rules: " << op->heads.size() << endl;)
	pred_index.clear();
	preds.clear();
	free_eps();

	//old::prover --> pred_index (preprocessing step)
	for (int i = op->heads.size()-1; i >= 0; i--)
	{
		old::nodeid pr = op->heads[i]->p;
		TRACE(dout << "adding rule for pred [" << pr << "] " << old::dict[pr] << "'" << endl;)
		pred_index[pr].push_back(i);
	}

	//pred_index --> preds (compilation step)
	for(auto x: pred_index){
		reverse(x.second.begin(), x.second.end());
		TRACE(dout << "Compling pred: " << old::dict[x.first] << endl;)
		preds[x.first] = compile_pred(x.first);
	}
}




//endregion











//region coros





coro unifjoin(Thing *a, Thing *b, coro c)
{
	FUN;
	TRACE(dout << "..." << endl;)
	EEE;
	coro uc;
	TRC(int round = 0;)
	return [a,b,c, uc, entry TRCCAP(round)]() mutable{
		setproc(L"unifjoin lambda");
		TRACE(dout << "entry = " << entry << endl;)
		TRC(round++;)

		switch(entry)
		{
		case 0:
			uc = unify(a,b);
			entry = LAST;
			while(uc()){
				ASSERT(round == 1);
				while(c()){
					ASSERT(round == 1);
					return true;
		case_LAST:;
					ASSERT(round == 2);
				};
				ASSERT(round == 2);
			}
			ASSERT(round == 2);
			END;
		}
	};
}



coro listunifycoro(Thing *a_, Thing *b_)
{
	setproc(L"listunifycoro");
	TRACE(dout << "..." << endl;)

	const Thing a = *a_;
	const Thing b = *b_;

	ASSERT(is_list(a));
	ASSERT(is_list(b));

	//gotta join up unifcoros of vars in the lists
	if(sizes_differ(a,b))
		return GEN_FAIL ;

	coro r = gen_succeed();

	for(int i = get_size(b);i > 0; i--)
	{
		r = unifjoin(a_+i, b_+i, r);
	}

	return r;
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

	if (a_ == b_) {//?
		TRACE(dout << "a == b" << endl;)
		return gen_succeed();
	}

	//TRACE(dout << "a:[" << a_ << "]" << getValue(a_) << endl;)

	a_ = getValue(a_);
	Thing a = *a_;

	ASSERT(!is_bound(a));

	if (is_unbound(a))
		return unboundunifycoro(a_,b_);

	b_ = getValue(b_);

	Thing b = *b_;
	ASSERT(!is_bound(b));

	if (is_unbound(b))
		return unboundunifycoro(b_, a_);

	if(are_equal(a,b)) {
		if (is_node(a)) {
					ASSERT(op->_terms.equals(get_term(a), get_term(b)));
			return gen_succeed();
		}

		if (is_list(a)) {
			TRACE(dout << "Both args are lists." << endl;)
			return listunifycoro(a_, b_);
		}
	}

	//# Other combinations cannot unify. Fail.
	TRACE(dout << "Some non-unifying combination. Fail.(" << a_ << "," << b_ << ")" << endl;)
	return GEN_FAIL;
}

rule_t seq(rule_t a, rule_t b){
	setproc(L"seq");
	TRACE(dout << ".." << endl;)
	EEE;
	TRC(int round = 0;)
	return [a, b, entry TRCCAP(round)](Thing *Ds, Thing *Do) mutable{
		setproc(L"seq lambda");	
		TRC(round++;)
		TRACE(dout << "round: " << round << endl;)
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



pred_t compile_pred(old::nodeid pr)
{
	FUN;
	rule_t y;
	vector<size_t> rs = pred_index.at(pr);
	TRACE(dout << "# of rules: " << rs.size() << endl;)
	bool first = true;
	//compile each rule with the pred in the head, seq them up
	for (auto r: rs) {
		rule_t x
				=
				//op->bodies[r].size()
				//?
				compile_rule(op->heads[r], op->bodies[r])
		//:
		/*fact(op->heads[r])*/;
		if (first) {
			first = false;
			TRACE(dout << "first, nodeid: " << pr << "(" << old::dict[pr] << ")" << endl;)
			y = x;
		}
		else {
			TRACE(dout << "seq, nodeid: " << pr << "(" << old::dict[pr] << ")" << endl;)
			y = seq(x, y);
		}
	}
	ASSERT(!first);
	return y;
}



void check_pred(old::nodeid pr)
{
	FUN;
	rule_t y;
	auto it = pred_index.find(pr);
	if (it == pred_index.end()) {
		dout << "Predicate '" << old::dict[pr] << "' not found." << endl;
		preds[pr] = GEN_FAIL_WITH_ARGS;
	}
	/*else return [pr]() mutable {
		setproc(L"pred lookup");
		TRACE(dout << "nodeid: " << pr << endl;)
		return preds.at(pr);
	};*/
 }




/*
one join function, joins "just pass" query s and o down between them,
each join calls one pred,
so we are only concerned with permuting the two params to the pred,
and these can be either: s, o, or a local var, or a const
a join captures two indexes into the locals/consts table, which it may or may not use
*/


#include "perms.cpp"




bool islist(termid t)
{
	ASSERT(t);
	return *old::dict[t->p].value == L".";
}


//return term's PredParam and possibly also its index into the corresponding vector
PredParam thing_key (termid x, size_t &index, locals_map &lm, locals_map &cm, termid head)
{
	if (head)
		ASSERT(head->s && head->o);
	if (head && x == head->s) {
		index = lm.at(head->s);
		return HEAD_S;
	}
	else if (head && x == head->o) {
		index = lm.at(head->o);
		return HEAD_O;
	}
	else {
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
}

//find thing in locals or consts by termid
Thing &find_thing(termid x, Locals &locals, Locals &consts, locals_map &lm, locals_map &cm)
{
	size_t i;
	auto pp = thing_key(x, i, lm, cm, 0);
	if (pp == LOCAL)
		return locals[i];
	else if (pp == CONST)
		return consts[i];
	else
		assert(false);
}


void print_locals(Locals &locals, Locals &consts, locals_map &lm, locals_map &cm, termid head)
{
	(void)head;
	TRACE(dout << "locals:" << endl);
	for (auto x: lm)
		dout << op->format(x.first) << " : : " << x.second << "  --> " << str(&locals.at(x.second)) << endl;
	TRACE(dout << "consts:" << endl);
	for (auto x: cm)
		dout << op->format(x.first) << " : : " << x.second << "  --> " << str(&consts.at(x.second)) << endl;
}


void make_offset(Locals &locals, Thing &t, size_t pos) {
	t.type = OFFSET;
	t.offset = pos - locals.size();
}// we are getting a strange warning from asan here



void make_locals(Locals &locals, Locals &consts, locals_map &lm, locals_map &cm, termid head, prover::termset body)
{
	FUN;
	std::queue<termid> lq;

	auto expand_lists = [&lq, &locals, &lm]() {
		setproc("expand_lists");
		while (!lq.empty()) {
			termid l = lq.front();
			lq.pop();
			auto lst = op->get_dotstyle_list(l);
			Thing i0; // list size item
			i0.type = LIST;
			i0.size = lst.size();
			lm[l] = locals.size();
			locals.push_back(i0);
			for (auto li: lst) {
				TRACE(dout << "item..." << endl;)
				Thing t;
				if (li->p < 0) { //its a var
					auto it = lm.find(li); //is it already in locals?
					if (it == lm.end()) { //no? create a fresh var
						t.type = UNBOUND;
						t.thing = (Thing *) 666;
						lm[li] = locals.size();
					}
					else { //just point to it
						make_offset(locals, t, it->second); 
					}
				}
				else { //its a node
					t.type = NODE;
					t.term = li;
					if (islist(li))
						lq.push(li);
				}
				locals.push_back(t);
			}
		}
	};

	//replace NODEs whose terms are lists with OFFSETs. expand_lists left them there.
	auto link_lists = [&locals, &lm]() {
		for (size_t i = 0; i < locals.size(); i++) {
			Thing &x = locals[i];
			if (x.type == NODE && islist(x.term)) {
				x.type = OFFSET;
				x.offset = lm.at(x.term) - i;
			}
		}
	};

	auto add_var = [](old::termid x, Locals &vec, locals_map &m) {
		setproc("add_var");		
		TRACE(dout << "termid:" << x << " p:" << old::dict[x->p] << "(" << x->p << ")" << endl;)
		Thing t;
		t.type = UNBOUND;
		t.thing = (Thing *) 666;
		auto it = m.find(x);
		if (it == m.end()) {
			m[x] = vec.size();
			vec.push_back(t);
		}

	};

	auto add_node = [](old::termid x, Locals &vec, locals_map &m) {
		setproc("add_node");
		TRACE(dout << "termid:" << x << " p:" << old::dict[x->p] << "(" << x->p << ")" << endl;)
		Thing t;
		t.type = NODE;
		t.term = x;
		auto it = m.find(x);
		if (it == m.end()) {
			m[x] = vec.size();
			vec.push_back(t);
		}

	};

	vector<termid> terms;
	TRACE(dout << "head:" << op->format(head) << endl);

	if (head) {
		terms.push_back(head->s);
		terms.push_back(head->o);
	}
	for (termid bi: body) {
		terms.push_back(bi->s);
		terms.push_back(bi->o);
	}
	TRACE(dout << terms.size() << "terms." << endl);
	for (termid x: terms) {
		if (x->p > 0 && !islist(x)) {
			//force rule s and o into locals for now
			if (head && (x == head->s || x == head->o))
				add_node(x, locals, lm);
			else
				add_node(x, consts, cm);
		}
		else if (x->p < 0)
			add_var(x, locals, lm);
	}
	for (termid x: terms)
		if (x->p > 0 && islist(x))
			lq.push(x);

	expand_lists();
	link_lists();

	print_locals(locals, consts, lm, cm, head);

}



join_gen compile_body(Locals &consts, locals_map &lm, locals_map &cm, termid head, prover::termset body)
{
	FUN;

	join_gen jg = succeed_with_args_gen();

	//for (int i = body.size() - 1; i >= 0; i--) {
		//termid &bi = body[i];
	auto b2 = body;
	reverse(b2.begin(), b2.end());
	for (termid bi: b2)
	{
		check_pred(bi->p);
		termid s = bi->s;
		termid o = bi->o;
		size_t i1, i2;
		PredParam sk = thing_key(s, i1, lm, cm, head);
		PredParam ok = thing_key(o, i2, lm, cm, head);
		TRACE(dout <<"perm " << permname.at(sk) << " " << permname.at(ok) << endl );
		jg = perms.at(sk).at(ok)(bi->p, jg, i1, i2, consts);
		//typedef function<join_gen(pred_gen, join_gen, size_t, size_t, Locals&)>
	}
	return jg;
}






bool find_ep(ep_t *ep, /*const*/ Thing *s, /*const*/ Thing *o)
{
	FUN;
	s = getValue(s);
	o = getValue(o);
	TRACE(dout << ep->size() << " ep items:" << endl);
	for (auto i: *ep) 
	{
		TRACE(dout << "---------------------" << endl);
		auto os = i.first;
		auto oo = i.second;
		TRACE(dout << endl << str(s) << "    VS     " << str(os) << endl << str(o) << "    VS    " << str(oo) << endl;)
		if (would_unify(os,s))
		{
			TRACE(dout << ".." << endl);
			if(would_unify(oo,o)) {
				TRACE(dout << "EP." << endl;)
				return true;
			}
		}
	}
	ep->push_back(thingthingpair(s, o));
	/*TRACE(dout << "paths:" << endl;
	for (auto ttp: *ep)
		  dout << ttp.first << " " << ttp.second << endl;)*/
	return false;
}




rule_t compile_rule(termid head, prover::termset body)
{
	FUN;

	locals_map lm, cm;
	Locals &locals_template = *new Locals();
	locals_templates.push_back(&locals_template);
	Locals &locals_template = *new Locals();
	locals_templates.push_back(&locals_template);
	/*LocalsQueue &locals_queue = *new LocalsQueue();
	locals_queues.push_back(&locals_template);
	 http://www.drdobbs.com/parallel/writing-lock-free-code-a-corrected-queue/210604448
	 http://man7.org/linux/man-pages/man3/alloca.3.html
	 http://www.boost.org/doc/libs/1_39_0/libs/pool/doc/index.html

	 */

	Locals *consts_ = new Locals();
	constss.push_back(consts_);
	Locals &consts = *consts_;

	make_locals(locals_template, consts, lm, cm, head, body);
	join_gen jg = compile_body(consts, lm, cm, head, body);

	size_t hs, ho;
	thing_key(head->s, hs, lm, cm, head);
	thing_key(head->o, ho, lm, cm, head);

	EEE;
	join_t j;
	coro suc, ouc;
	TRC(int round = 0;)
	ep_t *ep = new ep_t();
	eps.push_back(ep);

	auto locals_data = locals_template.data();
	auto locals_bytes = locals_template.size() * sizeof(Thing);
	Thing * locals=0;
	bool has_body = body.size();

	return [has_body, locals_bytes, locals_data, ep, hs, ho, locals ,&consts, jg, suc, ouc, j, entry TRCCAP(round)](Thing *s, Thing *o) mutable {
		setproc(L"rule coro");
		TRC(round++;)
		TRACE(dout << "round=" << round << endl;)
		switch (entry) {
			case 0: 

				locals = (Thing*)malloc(locals_bytes);
				memcpy(locals, locals_data, locals_bytes);
			
				//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
				ASSERT(hs < locals_bytes / sizeof(Thing));
				ASSERT(ho < locals_bytes / sizeof(Thing));

				suc = unify(s, &locals[hs]);
				while (suc()) {
					TRACE(dout << "After suc() -- " << endl;)
					//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
					ASSERT(round == 1);

					ouc = unify(o, &locals[ho]);
					while (ouc()) {
						TRACE(dout << "After ouc() -- " << endl;)
						//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
						ASSERT(round == 1);

						if ((steps != 0) && (steps % 1000000 == 0)) (dout << "step: " << steps << endl);
							++steps;

						if (has_body && find_ep(ep, s, o)) {
							goto end;
						}

						j = jg();
						while (j(s, o, locals)) {
							TRACE(dout << "After c0() -- " << endl;)
							//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)

							entry = LAST;
							return true;
							TRACE(dout << "MATCH." << endl;)
				case_LAST:;
							TRACE(dout << "RE-ENTRY" << endl;)
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




void thatsAllFolks(int nresults){
	dout << "That's all, folks, " << nresults << " results." << endl;
	dout << unifys << " unifys, " << steps << " steps." << endl;
}



pnode thing2node(Thing *t_, qdb &r) {
	t_ = getValue(t_);

	auto t = *t_;

	if (is_list(t))
	{
		const wstring head = listid();
		for (size_t i = 1; i <= get_size(t); i++) {
			auto x = (t_ + i);
			r.second[head].emplace_back(thing2node(x, r));
		}
		return mkbnode(pstr(head));
	}

	if (is_node(t))
		return std::make_shared<old::node>(old::dict[get_term(t)->p]);

	dout << "thing2node: Wtf did you send me?, " << str(t_) << endl;
	
	assert(false);
}


void add_result(qdb &r, Thing *s, Thing *o, old::nodeid p)
{
	r.first[L"@default"]->push_back(
		make_shared<old::quad>(
			old::quad(
				thing2node(s, r),
				std::make_shared<old::node>(old::dict[p]),
				thing2node(o, r)
			)
		)
	);
}



















yprover::yprover ( qdb qkb, bool check_consistency)  {
	TRACE(dout << "constructing old prover" << endl;)
	op = new old::prover(qkb, false);
	make_perms();
	compile_kb();
	if(check_consistency) dout << "consistency: mushy" << endl;
}


yprover::~yprover()
{
	free_eps();
	TRACE(dout << "deleting old prover" << endl;)
	delete op;
}

void yprover::query(const old::qdb& goal){
	FUN;
	dout << KGRN << "compile query." << KNRM << endl;
	results.clear();
	const old::prover::termset g = op->qdb2termset(goal);
	int nresults = 0;
	//TRACE(dout << sprintPred(L"Making pred",pr) << "..." << endl;)
	locals_map lm, cm;
	Locals locals, consts;

	make_locals(locals, consts, lm, cm, 0, g);
	join_gen jg = compile_body(consts, lm, cm, 0, g);

	//TRACE(dout << sprintPred(L"Run pred: ",pr) << " with " << sprintVar(L"Subject",s) << ", " << sprintVar(L"Object",o) << endl;)
	// this is weird, passing the args over and over

	join_t coro = jg();

	dout << KGRN << "run." << KNRM << endl;

	while (coro((Thing*)666,(Thing*)666,locals.data())) {
		nresults++;
		dout << KCYN << L"RESULT " << KNRM << nresults << ":";
		qdb r;
		r.first[L"@default"] = old::mk_qlist();

		for(auto i: g)
		{
			Thing &s = find_thing(i->s, locals, consts, lm, cm);
			Thing &o = find_thing(i->o, locals, consts, lm, cm);
			dout << str(getValue(&s)) << " " << old::dict[i->p] << " "  << str(getValue(&o)) << endl;
			TRACE(dout << sprintThing(L"Subject", &s) << " Pred: " << old::dict[i->p] << " "  << sprintThing(L"Object", &o) << endl;)
			add_result(r, &s, &o, i->p);
		}

		results.emplace_back(r);

		if (result_limit && nresults == result_limit) {
			dout << "STOPPING at " << KRED << nresults << KNRM << " results."<< endl;
			free_eps_nonassert();
			goto out;
		}

	}
	thatsAllFolks(nresults);
	out:;
}

//endregion

#ifdef notes65465687


/*so it seems we have 3 variants to start with:
 1: parameterless joins, with extra rule lambda and unify's,
 2: all joins have two parameters, there are permutations,
 3: joins have only as many parameters as they need, there are even more permutations
so i guess we start with 1?
*/





/*
bool ep_check(Thing *a, Thing *b){
	List *l1 = dynamic_cast<List*>(a);
	List *l2 = dynamic_cast<List*>(b);
	if(l1&&l2){
		if(l1->nodes.size() == l2->nodes.size()){
			for(size_t i=0;i<l1->nodes.size();i++){
				if(!ep_check(l1->nodes[i],l2->nodes[i])){
					return false;
				}
			}
			return true;
		}else{
			return false;
		}
	}else if(!l1 && l2){
		return false;
	}else if(l1 && !l2){
		return false;
	}else{
		return a->ep_value == b->ep_value;
	}
}
*/



			bool hit = false;

			switch(entry)
			{
			case 0:

				TRACE(dout << "check ep table" << endl;)
				for (auto x: ep[pr])
				{
					TRACE(dout << x.first->ep_value << "=?=" << Ds->ep_value << endl;)
					TRACE(dout << x.second->ep_value << "=?=" << Do->ep_value << endl;)
					/*auto ucs = generalunifycoro(x.first, Ds);
					while(ucs())
					{
						auto uco = unify(x.second, Do);
						while(uco())
							hit = true;
					}*/
					//while(x.first == Ds)
					//	while(x.second == Do)
					hit = ep_check(x.first,Ds) && ep_check(x.second,Do);
					//hit = (x.first->ep_value == Ds->ep_value) && (x.second->ep_value == Do->ep_value);

					if(hit)
						break;
				}
				if(hit)
				{
					TRACE(dout << "ep! ep!" << endl;)
					entry = 666;
					return false;
				}

				//add:
				index = ep[pr].size();
				TRACE(dout << "store " << Ds->ep_value << " " << Do->ep_value << endl;)
				ep[pr].push_back(thingthingpair(Ds, Do));
				//ep[pr].push_back(thingthingpair(Ds->getValue(), Do->getValue()));

				//loop over pred results:
				while(y(Ds, Do))
				{
					entry = 1;
					return true;
			case 1: ;
				}

				//remove:
				ASSERT(ep[pr].size() > index);
				{
					auto xxx = ep[pr][index];
					TRACE(dout << "erase " << xxx.first << " " << xxx.second << endl);
				}

				ep[pr].erase(ep[pr].begin()+index);
				entry = 666;
				return false;
			default: ;
				ASSERT(false);
			}
		};
	};
}







	/*
	function<bool()> varunifycoro(Thing *arg)
	{
		if (type == BOUND)
			return boundunifycoro(arg);
		else if (type == UNBOUND)
			return unboundunifycoro(arg);
		else
			ASSERT(false);
	}
	*/


	/*
		function<bool()> boundunifycoro(Thing *arg){
			setproc(L"boundunifycoro");
			//TRACE(dout << this << "/" << this->str() << " unifcoro arg=" << arg << "/" << arg->str() <<  endl;)
			TRACE(dout << "isBound: " << this << ", " << this->getValue() << endl;)
			TRACE(dout << "arg: " << arg << "/" << arg->str() << endl;)
			return unify(this, arg);
		}
	*/



/*
Arranging switch statement cases by probability of occurrence improves performance when the
switch statement is translated as a comparison chain; this arrangement has no negative impact when
the statement is translated as a jump table.

In general, use prototypes for all functions.
Prototypes can convey additional information to the compiler that might enable more aggressive
optimizations.

http://stackoverflow.com/questions/6434549/does-c11-add-the-c99-restrict-specifier-if-not-why-not

artificial
This attribute is useful for small inline wrappers that if possible should appear during debugging as a unit. Depending on the debug info format it either means marking the function as artificial or using the caller location for all instructions within the inlined body.

The preferred type for array indices is ptrdiff_t.
Application
This optimization applies to:
• 32-bit software
• 64-bit software
Rationale
Array indices are often used with pointers while doing arithmetic. Using ptrdiff_t produces more
portable code and will generally provide good performance.

In if statements, avoid long logical expressions that can generate dense conditional branches that violate the guideline described in “Density of Branches” on page 126. Listing 1. Preferred for Data that Falls Mostly Within the Range if (a <= max && a >= min && b <= max && b >= min) If most of the data falls within the range, the branches will not be taken, so the above code is preferred. Otherwise, the following code is preferred. Listing 2. Preferred for Data that Does Not Fall Mostly Within the Range if (a > max || a < min || b > max || b < min)

Assembly/Compiler Coding Rule 4. (MH impact, MH generality) Near calls must be matched with
near returns, and far calls must be matched with far returns. Pushing the return address on the stack
and jumping to the routine to be called is not recommended since it creates a mismatch in calls and
returns.

When using ld, include the following command line option:
-Bsymbolic
If using gcc to build a library, add this option to the command-line:
-Wl,-Bsymbolic



*/


/*
	wstring dbgstr() const
	{
		static int d = 0;
		if (d++ == 15) {
			d = 0;
			return L"break";
		}
		wstringstream r;
		r << "[" << this << "]";
		switch (type)
		{
			case UNBOUND:
				r << L"var()";
				break;
			case BOUND:
				ASSERT(thing);
				r << L"var(" << thing << L")";
				break;
			case NODE:
				ASSERT(term);
				r << op->format(term);
				break;
			case LIST: {
				r << L"{" << size << L"}(";
				for (size_t i = 1; i < size; i++) {
					if (i != 0) r << " ";
					r << (this + i)->dbgstr();
				}
				if(!size)
					r << " ";
				r << L")";
				break;
			}
			case OFFSET:
				wstringstream r;
				r << L"{";
				if (offset >= 0)
					r << L"+";
				r << offset << L"}->";
				r << (this + offset)->dbgstr();
				break;
		}
		return r.str();
	}*/



#endif
