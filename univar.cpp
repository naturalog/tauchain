#include <functional>
#include <unordered_map>
#include "univar.h"

using namespace std;
using namespace old;





#define FUN setproc(__FUNCTION__);





class Thing;
//btw im using gen in the sense that its a lambda generating another lambda

typedef std::pair<Thing*,Thing*> thingthingpair;
typedef std::vector<thingthingpair> ep_t;
typedef vector<Thing> List;
typedef vector<Thing> locals;

typedef function<bool()> coro;
typedef function<bool(Thing*,Thing*)> pred_t;
typedef function<pred_t()> pred_gen;
typedef function<bool(Thing*,Thing*)> rule_t;
typedef function<rule_t()> rule_gen;
typedef function<bool(Thing*,Thing*, locals)> join_t;
typedef function<join_t()> join_gen;





std::unordered_map<old::nodeid, pred_gen> preds;
old::prover *op; // used as a kb




coro unbound_succeed(Thing *x, Thing *y);
coro unify(Thing *, Thing *);
pred_gen pred(old::nodeid pr);






























//yield once
coro gen_succeed()
{
	int entry = 0;
	return [entry]() mutable{
		switch(entry)
		{
		case 0:
			entry = 1;
			return true;
		case 1:
			entry = 666;
			return false;
		default:
			assert(false);
		}
	};
}

join_t succeed_with_args()
{
	int entry = 0;
	return [entry](Thing *Ds, Thing *Do, locals _) mutable{
		(void)Ds;
		(void)Do;
		(void)_;
		switch(entry)
		{
		case 0:
			entry = 1;
			return true;
		case 1:
			entry = 666;
			return false;
		default:
			assert(false);
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
			assert(false);
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
			assert(false);
		}
	};
}

#define GEN_FAIL (dbg_fail())
#define GEN_FAIL_WITH_ARGS (dbg_fail_with_args())

#endif





















enum ThingType {UNBOUND, BOUND, NODE, LIST, LINK};
class Thing
{
public:
	ThingType type;
	union {old::termid term; Thing * thing; List * list; int offset};


	Thing(old::termid n)
	{
		FUN;
		TRACE(dout << "termid:" << n << " p:" << old::dict[n->p] << endl;)
		if (n->p > 0) {
			if (*old::dict[n->p].value == L".") {
				type = LIST;
				TRACE(dout << "list" << endl;)
				auto l = op->get_dotstyle_list(n);
				list = new List(l.size());
				for (auto y: l) {
					TRACE(dout << "item..." << endl;)
					list->push_back(Thing(y));
				}
				TRACE(dout << "new List: " << this << endl;)
			}
			else {
				type = NODE;
				term = n;
				TRACE(dout << "new Node: " << this << endl;)
			}
		}
		else {
			type = UNBOUND;
			TRACE(dout << "new Var: " << this << endl;)
		}
	}





	/*bool eq(Node *x)
	{
		setproc(L"eq");
		TRACE(dout << op->format(value) << " =?= " << op->format(x->value) << endl;)
		return op->_terms.equals(value, x->value);
	}*/
	wstring str() {
		switch(type)
		{
			case UNBOUND:
				return L"var()";
			case BOUND:
				assert(thing);
				return L"var(" + thing->str() + L")";
			case NODE:
				assert(term);
				return op->format(term);
			case LIST:
			{
				assert(list);
				wstringstream r;
				r << L"(";
				if(list->size() > 0)
				{
					for(size_t i=0;i<list->size();i++)
					{
						if(i != 0) r << " ";
							r << list->at(i).str();
					}
				}else{
					r << " ";
				}
				r << ")";
				return r.str();
			}
		}
		assert(false);
	}





	Thing *getValue()
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
	if (type == BOUND)
		return thing->getValue();
	else if (type == LINK)
		return (this + offset)->getValue();
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

	function<bool()> boundunifycoro(Thing *arg){
		setproc(L"boundunifycoro");
		//TRACE(dout << this << "/" << this->str() << " unifcoro arg=" << arg << "/" << arg->str() <<  endl;)
		TRACE(dout << "isBound: " << this << ", " << this->getValue() << endl;)
		TRACE(dout << "arg: " << arg << "/" << arg->str() << endl;)
		return unify(this, arg);
	}

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

			int entry = 0;
			return [this, entry, argv]() mutable {

				setproc(L"unify lambda 2");
				TRACE(dout << "im in ur var unify lambda, entry = " << entry << ", argv=" << argv << "/" <<
					  argv->str() << endl;)

				switch (entry) {
					case 0:
						TRACE(dout << "binding " << this << "/" << this->str() << " to " << argv << "/" <<
							  argv->str() << endl;)
						assert(type == UNBOUND);
						type = BOUND;
						thing = argv;
						entry = 1;
						return true;
					case 1:
						TRACE(dout << "unbinding " << this << "/" << this->str() << endl;)
						assert(type == BOUND);
						type = UNBOUND;
						entry = 666;
						return false;
					default:
						assert(false);
				}
			};
		}
	}











class yterm {
public:
	nodeid p;
	Thing s, o;
	yterm(termid x):p(x->p),s(x->s),o(x->o){};
};

typedef vector<yterm> body_t;

class kb_rule {
public:
	yterm head;
	body_t body;
	kb_rule(yterm h, body_t b):head(h),body(b){};

};

typedef vector<kb_rule> kb_rules;

std::unordered_map<old::nodeid, kb_rules> rules;


body_t make_body(old::prover::termset b)
{
	body_t r;
	for (termid i: b)
		r.push_back(yterm(i));
	return r;
}

void compile_kb()
{
	FUN
	TRACE(dout << "# of rules: " << op->heads.size() << endl;)

	//old::prover --> pred_index (preprocessing step)
	for (size_t i = 0; i < op->heads.size(); i++)
	{
		old::nodeid pr = op->heads[i]->p;
		TRACE(dout << "adding rule for pred [" << pr << "] " << old::dict[pr] << "'" << endl;)
		rules[pr].push_back(kb_rule(yterm(op->heads[i]), make_body(op->bodies[i])));
		reverse(rules[pr].begin(), rules[pr].end());
	}

	//pred_index --> preds (compilation step)
	for(auto x: rules){
		TRACE(dout << "Compling pred: " << old::dict[x.first] << endl;)
		preds[x.first] = pred(x.first);
	}
}











rule_t rule(kb_rule x, size_t &nlocals);










coro corojoin(coro a, coro b)
{
	setproc(L"unifjoin");
	TRACE(dout << "..." << endl;)
	
	int entry = 0;
	return [a,b, entry]() mutable{
		setproc(L"unifjoin lambda");
		TRACE(dout << "entry = " << entry << endl;)

		switch(entry)
		{
		case 0:
			entry++;
			while(a()){
				while(b()){
					return true;
		case 1: ;
						
				}
			}
			entry = 666;
			return false;
		default:
			assert(false);
		}
	};
}

coro listunifycoro(Thing *aa, Thing *bb)
{
	setproc(L"listunifycoro");
	TRACE(dout << "..." << endl;)
	assert(aa->type == LIST);
	assert(aa->type == LIST);

	assert(aa->list);
	assert(bb->list);

	List & a = *aa->list;
	List & b = *bb->list;

	//gotta join up unifcoros of vars in the lists
	if(a.size() != b.size())
		return dbg_fail();
	
	function<bool()> r;
	bool first = true;

	for(int i = b.size()-1;i >= 0; i--)
	{
		coro uc = unify(&a[i], &b[i]);
		
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
	if (a->type == BOUND)
		return a->boundunifycoro(b);
	else if (a->type == UNBOUND)
		return a->unboundunifycoro(b);

	//# Second argument is a variable
	b = b->getValue();
	if (b->type == BOUND)
		return b->boundunifycoro(a);
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
	if(a->type  == LIST && b->type == LIST)
	{
		TRACE(dout << "Both args are lists." << endl;)
		return listunifycoro(a, b);
	}

	//# Other combinations cannot unify. Fail.
	TRACE(dout << "Some non-unifying combination. Fail." << endl;)
	return GEN_FAIL;
}


rule_t fact(yterm * head){
	FUN;
	int entry = 0;
	coro c1;
	coro c2;
	Thing s = Thing(head->s);
	Thing o = Thing(head->o);
	return [s, o, entry, c1, c2](Thing *Ds, Thing *Do) mutable{

		setproc(L"fact lambda");
		TRACE(dout << "im in ur fact,  entry: " << entry << endl;)

		switch(entry){
		case 0:
			c1 = unify(Ds, &s);
			//TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << "Do: " << Do << "/" << Do->str() << endl;)
			while(c1()){
				TRACE(dout << "MATCH c1() " << endl;)

				c2 = unify(Do, &o);
				while(c2()){
					//TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << "Do: " << Do << "/" << Do->str() << endl;)
					entry = 1;
					TRACE(dout << "MATCH" << endl;)
					return true;
		case 1: ;
		TRACE(dout << "RE-ENTRY" << endl;)
				}
			}
			entry = 666;
			TRACE(dout << "DONE." << endl;)
			return false;
		default:
			assert(false);
		}
	};
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
			assert(false);
		}
		TRACE(dout << "Why are we here?" << endl;)
		assert(false);
	};
}




pred_gen pred(old::nodeid pr)
{
	setproc(L"compile_pred");

	kb_rules &rs = rules[pr];

	TRACE(dout << "# of rules: " << rs.size() << endl;)

	rule_t y;
	bool first = true;

	//compile each rule with the pred in the head, seq them up
	for (auto kbr: rules[pr]) {
		rule_t x = rule(kbr);

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

	assert(!first);

	return [pr, y]() mutable {
		setproc(L"pred lambda");
		TRACE(dout << "nodeid: " << pr << endl;)

		return y;
	};
}






typedef vector<old::termid> locals_map;

void lm_add(locals_map &lm, old::termid t)
{
	for (auto x: lm)
	{
		if (x == t)
			return;
	}
	lm.push_back(t);
}

size_t lm_index(locals_map &lm, old::termid t)
{
	for(size_t i = 0; i < lm.size(); i++)
		if (lm[i] == t)	
			return i;
	assert(false);
}

//permutation map key
char pk(locals_map &lm, termid x, termid head)
{
	char sk;
	if (x == head->s)
		sk = 's';
	else if (x == head->o)
		sk = 'o';
	else 
	{
		sk = 'l';
		lm_add(lm, x);
	}
	return sk;
}	

rule_t compile_rule(yterm head, body_t body, size_t &nlocals)
{
	FUN;
	//TRACE(dout << "compiling ruleproxy" << endl;)
	join_gen jg = succeed_with_args_gen();
	locals_map lm;
	for(int i = body.size() - 1; i >= 0; i--){
		termid s = body[i]->s;
		termid o = body[i]->o;
		char sk = hk(s, head);
		char ok = hk(o, head);
		jg = perms[sk][ok](body[i]->p, jg, lm_index(lm, s), lm_index(lm, o));
	}

	locals * l;
	
	return [ jg, suc,ouc,j, entry,round]   (Thing *s , Thing *o) mutable
	{
		setproc(L"rule coro");

		locals l = lt;



		round++;
		TRACE(dout << "round=" << round << endl;)
		switch(entry)
		{
		case 0:
			entry++;
			//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)

			for (int i = 0;i < nlocals; i++)
				l.push_back(new Var());
	
			suc = unify(s, hs);
			while(suc()) {
				TRACE(dout << "After suc() -- " << endl;)
				//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)

				ouc = unify(o, ho);
				while (ouc()) {
					TRACE(dout << "After ouc() -- " << endl;)
					//TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;) 

					j = jg();
					while (j(l,s,o)) {
						TRACE(dout << "After c0() -- " << endl;)
						TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)					
	
						entry = 1;
						return true;
						TRACE(dout << "MATCH." << endl;)
		case 1: ;
			TRACE(dout << "RE-ENTRY" << endl;)
					}
				}
			}
			entry = 666;
			TRACE(dout << "DONE." << endl;)
			return false;
		default:
			assert(false);
		}
	};
}


rule_t rule(kb_rule x, size_t &nlocals)
{
	FUN;
	//TRACE(dout << "compiling rule " << op->format(head) << " " << body.size() << endl;)
	if(x.body.size() == 0)
		return fact(x.head);
	else
		return compile_rule(x.head, x.body, nlocals);
}



//9x:
join_gen join_sl(pred_gen a, join_gen b, size_t wi, size_t xi){
	setproc(L"joinsw");
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	pred_t ac;
	join_t bc;
	return [a,b,wi,xi,entry,round,ac,bc]()mutable{
		setproc(L"join gen");
		return [a,b,wi,xi,entry, round, ac,bc](Thing *s, Thing *o)mutable{
			setproc(L"join coro");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch(entry){
			case 0:
				TRACE( dout << sprintPred(L"a()",a) << endl;)
				ac = a();
				while(ac(s,locals[wi])) {
					TRACE(dout << "MATCH A." << endl;)

					bc = b();
					while (bc(s,o)) {
						TRACE(dout << "MATCH." << endl;)
						entry = 1;
						return true;

			case 1: ;
					TRACE(dout << "RE-ENTRY" << endl;)
					}
				}
				entry = 666;
				TRACE(dout << "DONE." << endl;)
				return false;
			default:
				assert(false);
			}
		};
	};
}




















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
	auto v = t->getValue();
	while(true)//dig to the value
	{
		auto old = v;
		v = v->getValue();
		if (v == old) break;
	}

	List *l = dynamic_cast<List *>(v);
	if (l) {
		const wstring head = listid();
		for (auto x: l->nodes)
			r.second[head].emplace_back(thing2node(x, r));
		return mkbnode(pstr(head));
	}

	Node *n = dynamic_cast<Node *>(v);
	if (n)
		return std::make_shared<old::node>(old::dict[n->value->p]);

	Var *var = dynamic_cast<Var *>(v);
	if (var)
		dout << "thing2node: Wtf did you send me?, " << var->str() << endl;
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
	dout << "constructing old prover" << endl;
	op = p = new old::prover(qkb, false);
	compile_kb(p);
	if(check_consistency) dout << "consistency: mushy" << endl;
}

void yprover::query(const old::qdb& goal){
	dout << KGRN << "Query." << KNRM << endl;

	results.clear();

	const old::prover::termset g = p->qdb2termset(goal);
	int nresults = 0;
	old::nodeid pr = g[0]->p;

	if (pred_index.find(pr) != pred_index.end()) {
		varmap vars;
		atom(g[0]->s, vars);
		atom(g[0]->o, vars);
		TRACE(dout << vars.size() << " vars" << endl;)

		Thing *s = vars.at(g[0]->s);
		Thing *o = vars.at(g[0]->o);

		TRACE(dout << sprintPred(L"Making pred",pr) << "..." << endl;)

		auto coro = preds[pr]();

		TRACE(dout << sprintPred(L"Run pred: ",pr) << " with " << sprintVar(L"Subject",s) << ", " << sprintVar(L"Object",o) << endl;)

		// this is weird, passing the args over and over
		while (coro(s,o)) {
			nresults++;

			dout << KCYN << L"RESULT " << KNRM << nresults << ":";
			
			//get actually Subject/Object names from old::dict
			dout << sprintThing(L"Subject", s) << ", " << sprintThing(L"Object", o) << endl;
			
			
			qdb r;
			r.first[L"@default"] = old::mk_qlist();
			add_result(r, s, o, pr);
			results.emplace_back(r);
			

			if (nresults >= 10) {
				dout << "STOPPING at " << KRED << nresults << KNRM << " results."<< endl;
				break;
			}

		}

		thatsAllFolks(nresults);
	}else{
		
		dout << "Predicate '" << old::dict[pr] << "' not found, " << KRED << "0" << KNRM << " results." << endl;
	}
	
dout << "this 
is
a 
multiline
string" << endl;//nope, im not writing it in c++:)

}

#ifdef notes
/*
anyway...one join function, joins "just pass" query s and o down between them,
each join calls one pred,
so we are only concerned with permuting the two params to the pred,
and these can be either: s, o, or a local var
so thats 9 variants
a join captures two indexes into the local vars table, which it may or may not use
so...not the most efficient but a simple design, what do you think?
i think it would work, if i understand it correctly
i would start with a python script that prints out the code... which code exactly?
the permutations...so thats like our function joinwxyz now 
+ some wrapping, are you better in python than C++?hmm maybe comparable

dunno if i want to start tonight tho
yea i feel like i want to spend some time just analyzing this whole thing from first principles
i just think it will be a good exp to finish this, and good to not think about things too much
and just hack when i feel like it mmm, i'd probably like to a have firm understanding before i 
start any hacking. i have understanding, but no firm :) the ideal scenario would be that i could
actually analyze this mathematically
understanding of general tau matters or this thing? our implementation of the reasoner
well, i only have some idea where to start i guess
*/
/*


/*so it seems we have 3 variants to start with:
 1: parameterless joins, with extra rule lambda and unify's,
 2: all joins have two parameters, there are permutations,
 3: joins have only as many parameters as they need, there are even more permutations
so i guess we start with 1?
*/


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

/*
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
				assert(ep[pr].size() > index);
				{
					auto xxx = ep[pr][index];
					TRACE(dout << "erase " << xxx.first << " " << xxx.second << endl);
				}

				ep[pr].erase(ep[pr].begin()+index);
				entry = 666;
				return false;
			default: ;
				assert(false);
			}
		};
	};
}
*/


#endif


coro unbound_succeed(Thing *x, Thing *y)
{
	int entry = 0;
	return [entry, x, y]() mutable {
		assert(x->type == UNBOUND);
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
				assert(false);
		}
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
			assert(false);
	}
	*/
};
/*List(std::vector<Thing *> n)
	{
		nodes = n;
	}*/


/*
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
*/

