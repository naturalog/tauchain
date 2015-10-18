#include <functional>
#include <unordered_map>
#include <queue>
#include "univar.h"

using namespace std;
using namespace old;

#define FUN setproc(__FUNCTION__);

#define EEE char entry = 0;
#define LAST 33

#ifdef DEBUG
#define ITEM(x,y) x.at(y)
#define ASSERT assert
#define case_LAST case LAST:
#define END entry = 66; return false; default:	ASSERT(false);
#else
#define ITEM(x,y) x[y]
#define ASSERT(x)
#define case_LAST default:
#define END return false;
#endif


extern int result_limit ;


class Thing;
typedef vector<Thing> Locals;

typedef std::pair</*const*/ Thing*,/*const*/ Thing*> thingthingpair;
typedef std::vector<thingthingpair> ep_t;

typedef function<bool()> coro;
typedef function<bool(Thing*,Thing*)> pred_t;
typedef function<pred_t()> pred_gen;
typedef function<bool(Thing*,Thing*)> rule_t;
typedef function<rule_t()> rule_gen;
typedef function<bool(Thing*,Thing*, Locals&)> join_t;
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

coro unbound_succeed(Thing *x, Thing *y);
coro unify(Thing *, Thing *);
void check_pred(old::nodeid pr);
rule_t compile_rule(termid head, prover::termset body);
pred_t compile_pred(old::nodeid pr);









//region succeed and fail



//yield once
coro gen_succeed()
{
	EEE
	return [entry]() mutable{
		switch(entry)
		{
		case 0:
			entry = LAST;
			return true;
		case_LAST:END
		}
	};
}

join_t succeed_with_args()
{
	EEE
	return [entry](Thing *Ds, Thing *Do, Locals _) mutable{
		(void)Ds;
		(void)Do;
		(void)_;
		switch(entry)
		{
		case 0:
			entry = LAST;
			return true;
		case_LAST:END
		}
	};
}

join_gen succeed_with_args_gen()
{
	return []() {
		return succeed_with_args();
	};
}

#ifndef DEBUG

bool fail()
{
	setproc(L"fail");
	TRACE(dout << "..." << endl;)
	return false;
}

bool fail_with_args(Thing *_s, Thing *_o)
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


enum ThingType {UNBOUND, BOUND, NODE, LIST, OFFSET};
class Thing {
public:
	ThingType type;
	union {
		Thing *thing;
		old::termid term;
		size_t size;
		signed long offset;
	};
	wstring str() const
	{
		switch (type)
		{
			case UNBOUND:
				return L"var()";
			case BOUND:
				ASSERT(thing);
				return L"var(" + thing->str() + L")";
			case NODE:
				ASSERT(term);
				return op->format(term);
			case LIST: {
				wstringstream r;
				r << L"{" << size << L"}(";
				for (size_t i = 0; i < size; i++) {
					if (i != 0) r << " ";
					r << (this + 1 + i)->str();
				}
				if(!size)
					r << " ";
				return r.str() + L")";
			}
			case OFFSET:
				wstringstream r;
				r << L"{";
				if (offset >= 0)
					r << L"+";
				r << offset << L"}->";
				r << (this + offset)->str();
				return r.str();
		}
		ASSERT(false);
	}/*
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


	Thing *getValue () 
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
		if (type == BOUND) {
					ASSERT(thing);
			return thing->getValue();
		}
		else if (type == OFFSET)
		{
			ASSERT(offset);
			return (this + offset)->getValue();
		}
		else
			return this;
	}

	/*  # If this Variable is bound, then just call YP.unify to unify this with arg.
		# (Note that if arg is an unbound Variable, then YP.unify will bind it to
		# this Variable's value.)
		# Otherwise, bind this Variable to YP.getValue(arg) and yield once.  After the
		# yield, return this Variable to the unbound state.
		# For more details, see http://yieldprolog.sourceforge.net/tutorial1.html
	*/
	function<bool()> unboundunifycoro(Thing *arg)
	{
		TRACE(dout << "!Bound" << endl;)
		Thing *argv = arg->getValue();
		TRACE(dout << "value=" << argv << "/" << argv->str() << endl;)

		if (argv == this) {
			TRACE(dout << "argv == this" << endl;)
			//# We are unifying this unbound variable with itself, so leave it unbound.^M
#ifdef DEBUG
			return unbound_succeed(this, argv);
#else
			return gen_succeed();
#endif
		}
		else {
			TRACE(dout << "argv != this" << endl;)

			EEE
			return [this, entry, argv]() mutable {
				setproc(L"unify lambda 2");
				TRACE(dout << "im in ur var unify lambda, entry = " << entry << ", argv=" << argv << "/" <<
					  argv->str() << endl;)
				switch (entry) {
					case 0:
						TRACE(dout << "binding " << this << "/" << this->str() << " to " << argv << "/" <<
							  argv->str() << endl;)
						ASSERT(type == UNBOUND);
						type = BOUND;
						thing = argv;
						entry = LAST;
						return true;
					case_LAST:
						TRACE(dout << "unbinding " << this << "/" << this->str() << endl;)
						ASSERT(type == BOUND);
						type = UNBOUND;
						END
				}
			};
		}
	}
	bool would_unify(/*const*/ Thing *x)
	/*<HMC_a> koo7: ep check is supposed to determine equality by seeing if the values would unify (but not actually doing the unifying assignment)*/
	{
		ASSERT(type != OFFSET);
		ASSERT(type != BOUND);
		ASSERT(x->type != OFFSET);
		ASSERT(x->type != BOUND);
		FUN;
		if(type == NODE && x->type == NODE)
		{
			TRACE(dout << op->format(term) << " =?= " << op->format(x->term) << endl;)//??
			ASSERT(op->_terms.equals(term, x->term) == (term == x->term));
			return term == x->term;
		}
		else if (type == UNBOUND && x->type == UNBOUND)
			return true;
		else if (type == LIST && x->type == LIST)
		{
			if(size != x->size) return false;
			for (size_t i = 1; i <= size; i++) 
				if (!(this+i)->would_unify(x+i))
					return false;
			return true;
		}
		return false;
	}
};


#ifdef DEBUG
coro unbound_succeed(Thing *x, Thing *y)
{
	int entry = 0;
	return [entry, x, y]() mutable {
		ASSERT(x->type == UNBOUND);
		setproc(L"unify lambda 1");
		//TRACE(dout << "im in ur argv == this var unify lambda, entry = " << entry << ", argv= " << argv << "/" <<y->str() << endl;)
		switch (entry) {
			case 0:
				entry = 1;
				return true;
			case 1:
				entry = 666;
				return false;
			default:
				ASSERT(false);
		}
	};
}
#endif



//region sprint

wstring sprintVar(wstring label, Thing *v){
	wstringstream wss;
	wss << label << ": (" << v << ")" << v->str();
	return wss.str();
}

wstring sprintPred(wstring label, old::nodeid pred){
	wstringstream wss;
	wss << label << ": (" << pred << ")" << old::dict[pred];
	return wss.str();
}

wstring sprintThing(wstring label, Thing *t){
	wstringstream wss;
	wss << label << ": [" << t << "]" << t->str();
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
	for (auto x: constss)
		delete x;
	eps.clear();
	constss.clear();
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





coro corojoin(coro a, coro b)
{
	FUN;
	TRACE(dout << "..." << endl;)
	
	int entry = 0;
	return [a,b, entry]() mutable{
		setproc(L"unifjoin lambda");
		TRACE(dout << "entry = " << entry << endl;)

		switch(entry)
		{
		case 0:
			entry = LAST;
			while(a()){
				while(b()){
					return true;
		CASE_LAST ;
						
				}
			}
		END;
		}
	};
}

coro listunifycoro(Thing *a, Thing *b)
{
	setproc(L"listunifycoro");
	TRACE(dout << "..." << endl;)
	ASSERT(a->type == LIST);
	ASSERT(b->type == LIST);

	//gotta join up unifcoros of vars in the lists
	if(a->size != b->size)
		return GEN_FAIL ;
	if(!a->size)
		return gen_succeed();

	function<bool()> r;
	bool first = true;

	for(int i = b->size;i > 0; i--)
	{
		coro uc = unify(a+i, b+i);//////////
		
		if(first){
			r = uc;
			first = false;
		}
		else
		{
			r = corojoin(uc, r);
		}
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
coro unify(Thing *a, Thing *b){
	setproc(L"unify");
	TRACE(dout << "..." << endl;)

	if (a == b) {
		TRACE(dout << "a == b" << endl;)
		return gen_succeed();
	}

	//# First argument is a variable
	TRACE(dout << "a: " << a << ", " << a->getValue() << endl;)	
	a = a->getValue();
	if (a->type == BOUND) {
		ASSERT(0);// result of getvalue a bound var?
		return unify(a, b);
	}
	else if (a->type == UNBOUND)
		return a->unboundunifycoro(b);

	//# Second argument is a variable
	b = b->getValue();
	if (b->type == BOUND) {
		ASSERT(0);// result of getvalue a bound var?
		return unify(b, a);
	}
	else if (b->type == UNBOUND)
		return b->unboundunifycoro(a);

	//# Both arguments are nodes.
	if(a->type == NODE && b->type == NODE)
	{
		TRACE(dout << "Both args are nodes." << endl;)
		if(a->term == b->term) ////////eq needed?
			return gen_succeed();
		else
			return GEN_FAIL;
	}
	
	//# Both arguments are lists
	if(a->type == LIST && b->type == LIST)
	{
		TRACE(dout << "Both args are lists." << endl;)
		return listunifycoro(a, b);
	}



	//# Other combinations cannot unify. Fail.
	TRACE(dout << "Some non-unifying combination. Fail.(" << a->type << "," << b->type << ")" << endl;)
	return GEN_FAIL;
}

rule_t seq(rule_t a, rule_t b){
	setproc(L"seq");
	TRACE(dout << ".." << endl;)
	int entry = 0;
	int round = 0;
	return [a, b, entry, round](Thing *Ds, Thing *Do) mutable{
		setproc(L"seq lambda");	
		round++;
		TRACE(dout << "round: " << round << endl;)
		
		switch(entry){
		case 0:
			while(a(Ds, Do)){
				TRACE(dout << "MATCH A." << endl;)
				entry = 1;
				return true;
		case 1: ;
			}
			while(b(Ds, Do)){
				entry = 2;
				TRACE(dout << "MATCH B." << endl;)
				return true;
		case 2:	;
			}

			TRACE(dout << "SWITCH DONE." << endl;)

			entry = 666;
			return false;

		default:
			ASSERT(false);
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

//return reference to thing by termid
Thing &get_thing(termid x, Locals &locals, Locals &consts, locals_map &lm, locals_map &cm)
{
	size_t i;
	auto pp = thing_key(x, i, lm, cm, 0);
	if (pp == LOCAL)
		return locals[i];
	else if (pp == CONST)
		return consts[i];
	else
		ASSERT(false);
}


void print_locals(Locals &locals, Locals &consts, locals_map &lm, locals_map &cm, termid head)
{
	(void)head;
	TRACE(dout << "locals:" << endl);
	for (auto x: lm)
		dout << op->format(x.first) << " : : " << x.second << "  --> " << locals.at(x.second).str() << endl;
	TRACE(dout << "consts:" << endl);
	for (auto x: cm)
		dout << op->format(x.first) << " : : " << x.second << "  --> " << consts.at(x.second).str() << endl;
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
	FUN;/*
	s = s->getValue();
	o = o->getValue();
	for (auto i: *ep) 
	{
		TRACE(dout << s->str() << " vs " << i.first->str() << "  ,  ")
		TRACE(dout << o->str() << " vs " << i.second->str() << endl;)
		auto os = i.first->getValue();
		auto oo = i.second->getValue();
		if (s->eq(os) && o->eq(oo))
		{
			TRACE(dout << "EP." << endl;)
			return true;
		}
	}
*/
	ep->push_back(thingthingpair(s, o));
	TRACE(dout << "paths:" << endl;
	for (auto ttp: *ep)
		  dout << ttp.first << " " << ttp.second << endl;)
	return false;
}


rule_t compile_rule(termid head, prover::termset body)
{
	FUN;

	locals_map lm, cm;
	Locals locals;
	Locals *consts_ = new Locals();
	constss.push_back(consts_);
	Locals &consts = *consts_;

	make_locals(locals, consts, lm, cm, head, body);
	join_gen jg = compile_body(consts, lm, cm, head, body);

	size_t hs, ho;
	thing_key(head->s, hs, lm, cm, head);
	thing_key(head->o, ho, lm, cm, head);

	join_t j;
	coro suc, ouc;
	int round = 0, entry = 0;
	ep_t *ep = new ep_t();
	eps.push_back(ep);

	return [ep, hs, ho, locals,&consts, jg, suc, ouc, j, entry, round](Thing *s, Thing *o) mutable {
		setproc(L"rule coro");
		round++;
		TRACE(dout << "round=" << round << endl;)
		switch (entry) {
			case 0: 

				if (find_ep(ep, s, o)) {
					entry = 666;
					return false;
				}
			
				//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
				suc = unify(s, ITEM(&locals,hs));
				while (suc()) {
					TRACE(dout << "After suc() -- " << endl;)
					//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)

					ouc = unify(o, ITEM(&locals,ho));
					while (ouc()) {
						TRACE(dout << "After ouc() -- " << endl;)
						//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)

						j = jg();
						while (j(s, o, locals)) {
							TRACE(dout << "After c0() -- " << endl;)
							//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)

							entry = 1;
							return true;
							TRACE(dout << "MATCH." << endl;)
							case 1:;
							TRACE(dout << "RE-ENTRY" << endl;)
						}
					}
				}
				entry = 666;
				TRACE(dout << "DONE." << endl;)
				ASSERT(ep->size());
				ep->pop_back();
				return false;
			default:
				ASSERT(false);
		}
	};
}



//endregion











//region interface




void thatsAllFolks(int nresults){
	dout << "That's all, folks, ";
	if(nresults == 0){
		dout << KRED;
	}else{
		dout << KCYN;
	}
	dout << nresults << KNRM << " results." << endl;
}



pnode thing2node(Thing *t, qdb &r) {
	t = t->getValue();

	if (t->type == LIST)
	{
		const wstring head = listid();
		for (size_t i = 1; i <= t->size; i++) {
			auto x = (t + i);
			r.second[head].emplace_back(thing2node(x, r));
		}
		return mkbnode(pstr(head));
	}

	if (t->type == NODE)
		return std::make_shared<old::node>(old::dict[t->term->p]);

	dout << "thing2node: Wtf did you send me?, " << t->type << " " << t->offset << endl;
	
	ASSERT(false);
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
	dout << "constructing old prover" << endl;
	op = new old::prover(qkb, false);
	make_perms();
	compile_kb();
	if(check_consistency) dout << "consistency: mushy" << endl;
}


yprover::~yprover()
{
	free_eps();
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

	while (coro((Thing*)666,(Thing*)666,locals)) {
		nresults++;
		/*if (nresults >= 10) {
			dout << "STOPPING at " << KRED << nresults << KNRM << " results."<< endl;
			goto out;
		}*/

		dout << KCYN << L"RESULT " << KNRM << nresults << ":";
		qdb r;
		r.first[L"@default"] = old::mk_qlist();

		for(auto i: g)
		{
			Thing &s = get_thing(i->s, locals, consts, lm, cm);
			Thing &o = get_thing(i->o, locals, consts, lm, cm);

			dout << sprintThing(L"Subject", &s) << " Pred: " << old::dict[i->p] << " "  << sprintThing(L"Object", &o) << endl;
			add_result(r, &s, &o, i->p);

		}
		results.emplace_back(r);
	}
	thatsAllFolks(nresults);
	out:;
}

//endregion

#ifdef notes


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

#endif
