#include <functional>
#include <unordered_map>
#include "univar.h"

using namespace std;
using namespace old;


/*so it seems we have 3 variants to start with:
 1: parameterless joins, with extra rule lambda and unify's,
 2: all joins have two parameters, there are permutations,
 3: joins have only as many parameters as they need, there are even more permutations
so i guess we start with 1?
*/

old::prover *op;



class Thing{
public:
	virtual Thing *getValue(){return this;};
	virtual wstring str(){assert(false);}
	old::termid ep_value;
};

class Var;

typedef function<bool(Thing*,Thing*)> comp;


typedef function<bool()> join;
typedef function<join()> join_gen;

typedef std::unordered_map<old::termid, Thing *> varmap;

typedef vector<size_t> rulesindex;
std::unordered_map<old::nodeid, rulesindex> pred_index;


function<bool()> generalunifycoro(Thing*, Thing*);
void atom(old::termid n, varmap &vars);


typedef function<comp()> pred_t;
std::unordered_map<old::nodeid, pred_t> preds;

//std::unordered_map<old::nodeid, Var*> globals;

typedef std::pair<Thing*,Thing*> thingthingpair;
typedef std::unordered_map<old::nodeid, std::vector<thingthingpair>> ep_t;

ep_t ep;


class Node:public Thing
{
public:
	old::termid value;
	bool eq(Node *x)
	{
		setproc(L"eq");
		TRACE(dout << op->format(value) << " =?= " << op->format(x->value) << endl;)
		return op->_terms.equals(value, x->value);
	}
	Node(old::termid s)
	{
		ep_value = s;
		value = s;
	}
	wstring str(){return op->format(value);}
};


class Var:public Thing{
public:
	bool isBound = false;

	Thing *value=(Thing*)666;

	Thing *getValue()
/*btw!!
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
		if (isBound)
			return value;
		else
			return this;
	}

	Var(){}

	Var(old::termid n){
		ep_value = n;	
	}


	wstring str() {
		if (isBound) {
			assert(value != (Thing*)666);
			return L"var(" + value->str() + L")";
		}
		return L"var()";
	}



/*  # If this Variable is bound, then just call YP.unify to unify this with arg.
    # (Note that if arg is an unbound Variable, then YP.unify will bind it to
    # this Variable's value.)
    # Otherwise, bind this Variable to YP.getValue(arg) and yield once.  After the
    # yield, return this Variable to the unbound state.
    # For more details, see http://yieldprolog.sourceforge.net/tutorial1.html
*/

	//lets break this up into a couple functions for readability
	function<bool()> unifcoro(Thing *arg){
		setproc(L"Var.unifcoro");
		TRACE(dout << this << "/" << this->str() << " unifcoro arg=" << arg << "/" << arg->str() <<  endl;)
		if(isBound)
		{
			TRACE(dout << "isBound: " << this << ", " << this->getValue() << endl;)
			TRACE(dout << "arg: " << arg << "/" << arg->str() << endl;)
			return generalunifycoro(this, arg);
		}
		else
		{
			TRACE(dout << "!Bound" << endl;)

			Thing * argv = arg->getValue();

			TRACE(dout << "value=" << argv << "/" << argv->str() << endl;)

			if (argv == this)
			{
				TRACE(dout << "argv == this" << endl;)
				//# We are unifying this unbound variable with itself, so leave it unbound.^M
				int entry = 0;
				return [this, entry, argv]() mutable{
					assert(!isBound);
					setproc(L"unify lambda 1");
					TRACE(dout << "im in ur argv == this var unify lambda, entry = " << entry << ", argv= " << argv << "/" << argv->str() << endl;)
					switch(entry){
						case 0:
							//value = argv;//??? // like, this.value?
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
			else
			{
				TRACE(dout << "argv != this" << endl;)

				int entry = 0;
				return [this, entry, argv]() mutable{

					setproc(L"unify lambda 2");
					TRACE(dout << "im in ur var unify lambda, entry = " << entry << ", argv=" << argv << "/" << argv->str() << endl;)

					switch(entry)
					{
							case 0:
							TRACE(dout << "binding " << this << "/" << this->str() << " to " << argv << "/" << argv->str() << endl;)
							assert(!isBound);
							isBound = true;
							value = argv;
							entry = 1;
							return true;
						case 1:
							TRACE(dout << "unbinding " << this << "/" << this->str() << endl;)
							assert(isBound);
							isBound = false;
							entry = 666;
							return false;
						default:
							assert(false);
					}
				};
			}
		}
	}
};



Thing  * vvv(Thing *v)
{	
	return v;
}

class List: public Thing{
public:
	std::vector<Thing*> nodes;
	List(std::vector<Thing *> n)
	{
		nodes = n;
	}

	wstring str(){
		wstringstream r;
		r << L"(";
		if(this->nodes.size() > 0){
			for(size_t i=0;i<this->nodes.size();i++){
				if(i != 0) r << " ";
				r << nodes[i]->str();
			}
		}else{
			r << " ";
		}
		r << ")";
		return r.str();
	}


};



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

join dbg_fail()
{
	int entry = 0;
	return [entry]() mutable{
		setproc(L"dbg_fail lambda");
		TRACE(dout << "..." << endl;)

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

comp dbg_fail_with_args()
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

//yield once
function<bool()> succeed()
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


join unifjoin(join a, join b)
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

function<bool()> listunifycoro(List *a, List *b)
{
	setproc(L"listunifycoro");
	TRACE(dout << "..." << endl;)

	//gotta join up unifcoros of vars in the lists
	if(a->nodes.size() != b->nodes.size())
		return dbg_fail();
	
	function<bool()> r;
	bool first = true;

	for(int i = b->nodes.size()-1;i >= 0; i--) 
	{
		join uc = generalunifycoro(a->nodes[i], b->nodes[i]);
		
		if(first){
			r = uc;
			first = false;
		}
		else
		{
			r = unifjoin(uc, r);
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
function<bool()> generalunifycoro(Thing *a, Thing *b){
	setproc(L"generalunifycoro");
	TRACE(dout << "..." << endl;)

	if (a == b)
		return succeed();
	

	//# First argument is a variable
	TRACE(dout << "a: " << a << ", " << a->getValue() << endl;)	
	a = a->getValue();
	Var *a_var = dynamic_cast<Var*>(a);
	if (a_var){
		TRACE(dout << "arg1 is a variable." << endl;) 
		return a_var->unifcoro(b);
	}	

	//# Second argument is a variable
	b = b->getValue();
	Var *b_var = dynamic_cast<Var*>(b);
	if (b_var){
		TRACE(dout << "arg2 is a variable." << endl;)
		return b_var->unifcoro(a);
	}

	//# Both arguments are nodes.
	Node *n1 = dynamic_cast<Node*>(a);
	Node *n2 = dynamic_cast<Node*>(b);
	if(n1&&n2)
	{
		TRACE(dout << "Both args are nodes." << endl;)
		if(n1->eq(n2))
			return succeed();
		else
			return dbg_fail();
	}
	
	//# Both arguments are lists
	List *l1 = dynamic_cast<List*>(a);
	List *l2 = dynamic_cast<List*>(b);
	if (l1&&l2){
		TRACE(dout << "Both args are lists." << endl;)
		return listunifycoro(l1, l2);	
	}

	//assert(false);
	//# Other combinations cannot unify. Fail.
	TRACE(dout << "Some non-unifying combination. Fail." << endl;)
	return dbg_fail();
}

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


comp fact(Thing *s, Thing *o){
	setproc(L"fact");
	int entry = 0;
	function<bool()> c1;
	function<bool()> c2;
	return [s, o, entry, c1, c2](Thing *Ds, Thing *Do) mutable{
		setproc(L"fact lambda");
		TRACE(dout << "im in ur fact,  entry: " << entry << endl;)

		switch(entry){
		case 0:
			c1 = generalunifycoro(Ds,s);
			TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << "Do: " << Do << "/" << Do->str() << endl;)
			while(c1()){
				TRACE(dout << "MATCH c1() " << endl;)

				c2 = generalunifycoro(Do,o);
				while(c2()){
					TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << "Do: " << Do << "/" << Do->str() << endl;)
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

comp seq(comp a, comp b){
	setproc(L"seq");
	TRACE(dout << ".." << endl;)
	int entry = 0;
	int round = 0;
	comp ac, bc;
	return [a, b, entry, round, ac, bc](Thing *Ds, Thing *Do) mutable{
		setproc(L"seq lambda");	
		round++;
		TRACE(dout << "round: " << round << endl;)
		
		switch(entry){
		case 0:
			//ac = a;
			while(a(Ds, Do)){
				TRACE(dout << "MATCH A." << endl;)
				entry = 1;
				return true;
		case 1: ;
			}

			//bc = a;
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


void atom(old::termid n, varmap &vars){
	setproc(L"atom");
	TRACE(dout << "termid:" << n << " p:" << old::dict[n->p] << endl;)
	if (vars.find(n) != vars.end())
		return;
	if (n->p>0)
	{

		if(*old::dict[n->p].value == L".")
		{
			TRACE(dout << "list" << endl;)
			std::vector<Thing*> nodes;
			for(auto y: op->get_dotstyle_list(n)) {
				TRACE(dout << "item..." << endl;)
				atom(y, vars);
				nodes.push_back(vars.at(y));
			}
			auto r = vars[n] = new List(nodes);
			TRACE(dout << "new List: " << r << endl;)
		}
		else {
			auto r = vars[n] = new Node(n);
			TRACE(dout << "new Node: " << r << endl;)
		}

	}
	else {
		auto r = vars[n] = new Var(n);
		TRACE(dout << "new Var: " << r << endl;)
	}
}


comp rule(old::termid head, old::prover::termset body);


comp compile_pred(old::nodeid pr) {
	setproc(L"compile_pred");
	rulesindex rs = pred_index[pr];
	TRACE(dout << "# of rules: " << rs.size() << endl;)
	
	comp r;
	bool first = true;
	
	//compile each rule with the pred in the head, seq them up
	for (int i = rs.size()-1; i>=0; i--) {
		
		comp y = rule(op->heads[rs[i]], op->bodies[rs[i]]);

		if (first) {
			first = false;
			TRACE(dout << "first, nodeid: " << pr << "(" << old::dict[pr] << ")" << endl;)
			r = y;
		}
		else {
			TRACE(dout << "seq, nodeid: " << pr << "(" << old::dict[pr] << ")" << endl;)
			r = seq(y, r);
		}
	}

	if (first) // cant leave it empty
		return dbg_fail_with_args();
	return r;
}

pred_t pred(old::nodeid pr)
{
	setproc(L"pred");
	TRACE(dout << "constructing pred proxy for (" << pr << ")" << old::dict[pr] << endl;)
	comp y;// = compile_pred(pr);
	//you think theres any advantage to compiling it aot like this?
	//well, in our case yes, we could compile on-demand, but we were compiling on-demand every invocation
	//so the time-complexities here aren't even comparable
	//one is finite & one-time, the other is basically that over and over and over again
	//mkay but you forgot about variables? ? this is variation of your predGens lookup table mechanism
	//yes
	//i'm not sure where the difference arises variables, nodes, all the pointers to stuff we new
	//with just aot, one instance of pred with one instances of variables could be invoked recursively one
	//inside another, ok then we'll handle that at the var level

	/*well...
	anyway, you think you agree with me in how i think this works? i'm not 100% on how its working me neither
	i mean..im sure we capture these pointers and then we try to reuse them from two different instances of the pred,
	but, yes i agree
	but im totally not sure if thats a bad thing
	ok im pretty sure it is, but, i think i have an idea how to fix it
	hmm, ok i think i see generally what the problem is, i'll need to think about it
	i havent given much thought to it yet...
	we capture pointers to variables which must be being kept around somewhere
	*/

	return [pr, y]() mutable{
		setproc(L"pred lambda");
		TRACE(dout << "nodeid: " << pr << endl;)

		y = compile_pred(pr);

		int entry=0;
		int round=0;

		size_t index=666666;

		return [index, pr, y, entry, round](Thing *Ds, Thing *Do) mutable{
		        setproc(L"pred ep lambda");
			TRACE(dout << "entry=" << entry << endl);

			bool hit = false;

			switch(entry)
			{
			case 0:
				
				dout << "check ep table" << endl;
				for (auto x: ep[pr])
				{
					dout << x.first->ep_value << "=?=" << Ds->ep_value << endl;
					dout << x.second->ep_value << "=?=" << Do->ep_value << endl;
					/*auto ucs = generalunifycoro(x.first, Ds);
					while(ucs())
					{
						auto uco = generalunifycoro(x.second, Do);
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
					dout << "ep! ep!" << endl;
					entry = 666;
					return false;
				}

				//add:
				index = ep[pr].size();
				dout << "store " << Ds->ep_value << " " << Do->ep_value << endl;
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

join_gen joinOne(old::nodeid a, Thing* w, Thing *x){
	setproc(L"joinOne");
	TRACE(dout << "..." << endl;)

	int entry = 0;
	int round = 0;

	comp ac;

	return [a, w, x, entry, round, ac]() mutable{
		setproc(L"joinOne gen");
		return [ac, a, w, x, entry, round]() mutable{
			setproc(L"joinOne λ λ");
			round++;
			TRACE(dout << "round=" << round << endl;)
			switch(entry){
			case 0:
				ac = preds[a]();
				TRACE(dout << "OUTER -- w: " << w << "/" << w->str() << ", x: " << x << "/" << x->str() << endl;)
				while(ac(w,x)){
					TRACE(dout << "INNER -- w: " << w << "/" << w->str() << ", x: " << x << "/" << x->str() << endl;)
					entry = 1;
					return true;
			case 1: ;
				}
				entry=666;
				return false;
			default:
				assert(false);
			}
		};
	};
}


join_gen joinwxyz(old::nodeid a, old::nodeid b, Thing *w, Thing *x, Thing *y, Thing *z){
	setproc(L"joinwxyz");
	TRACE(dout << "making a join" << endl;)
	int entry = 0;
	int round = 0;
	comp ac;
	comp bc;
	return [a,b,w,x,y,z,entry,round,ac,bc]()mutable{
		setproc(L"join lambda");
		return [a,b,w,x,y,z,entry, round, ac,bc]()mutable{
			setproc(L"join lambda lambda");
			round++;
			TRACE(dout << "round: " << round << endl;)
			switch(entry){
			case 0:
				TRACE( dout << sprintPred(L"a()",a) << endl;)
				TRACE( dout << sprintPred(L"b()",b) << endl;)
				ac = preds[a]();
				while(ac(w,x)) {
					TRACE(dout << "MATCH A." << endl;)

					bc = preds[b]();
					while (bc(y,z)) {
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

comp ruleproxy(join_gen c0_gen, Thing *s, Thing *o){
	setproc(L"ruleproxy");
	TRACE(dout << "ruleproxy" << endl;)

	
	int entry=0;
	int round=0;

	join c0;
	function<bool()> suc, ouc;

	return [ suc, ouc, entry, c0_gen, c0, s, o, round]   (Thing *Ds , Thing *Do) mutable
	{
		setproc(L"ruleproxy lambda");
		round++;
		TRACE(dout << "round=" << round << endl;)
		switch(entry)
		{
		case 0:
			entry++;
			TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)
	
			suc = generalunifycoro(Ds, s);
			ouc = generalunifycoro(Do, o);
			c0 = c0_gen();			
			while(suc()) {
				TRACE(dout << "After suc() -- " << endl;)
				TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;)

				//ouc = generalunifycoro(Do, o);
				while (ouc()) {
					TRACE(dout << "After ouc() -- " << endl;)
					TRACE(dout << sprintSrcDst(Ds,s,Do,o) << endl;) 

					//c0 = c0_gen();
					while (c0()) {
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

join_gen halfjoin(old::nodeid a, Thing* w, Thing* x, join_gen b){
	setproc(L"halfjoin");
	TRACE(dout << "..." << endl;)

	int entry = 0;
	int round = 0;

	comp ac;
	join bc;

	return [a, w, x, b, entry, round, ac, bc]() mutable{
		setproc(L"halfjoin gen");
		return [a, w, x, b, entry, round,ac,bc]() mutable{
			setproc(L"halfjoin lambda");
			round++;
			TRACE(dout << "round=" << round << endl;)
			switch(entry){
			case 0:
				TRACE(dout << sprintPred(L"a()",a) << endl;)
				ac = preds[a]();
				while(ac(w,x)){
					TRACE(dout << "MATCH a(w,x)" << endl;)
					bc = b();
					while(bc()){
						entry = 1;
						TRACE( dout << "MATCH." << endl;)
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

comp ruleproxyOne(varmap vars, old::termid head, old::prover::termset body)
{
	setproc(L"comp ruleproxyOne");
	TRACE(dout << "compiling ruleproxyOne" << endl;)

	Thing *s = vars.at(head->s);
	Thing *o = vars.at(head->o);

	Thing *as = vars.at(body[0]->s);
	Thing *ao = vars.at(body[0]->o);

		
	join_gen c0 = joinOne(body[0]->p,as,ao);
	return ruleproxy(c0, s, o);
}

comp ruleproxyMore(varmap vars, old::termid head, old::prover::termset body)
{
	setproc(L"comp ruleproxyMore");
	TRACE(dout << "compiling ruleproxyMore" << endl;)

	int k = body.size()-2;
	Thing *as = vars.at(body[k]->s);
	Thing *ao = vars.at(body[k]->o);
	Thing *bs = vars.at(body[k+1]->s);
	Thing *bo = vars.at(body[k+1]->o);
	join_gen c0 = joinwxyz(body[k]->p, body[k+1]->p, as, ao, bs, bo);

	for(int i = k-1; i >= 0; i--){
		Thing *vs = vars.at(body[i]->s);
		Thing *vo = vars.at(body[i]->o);

		c0 = halfjoin(body[i]->p, vs, vo, c0);
	}

	Thing *s = vars.at(head->s);
	Thing *o = vars.at(head->o);

	return ruleproxy(c0, s, o);
}

comp rule(old::termid head, old::prover::termset body)
{
	setproc(L"comp rule");
	TRACE(dout << "compiling rule " << op->format(head) << " " << body.size() << endl;)

	varmap vars;

	//these two are just proxies for whatever input we get
	atom(head->s, vars);
	atom(head->o, vars);

	size_t i;
	for (i = 0; i < body.size(); i++)
	{
		old::termid t;
		t = body[i]->s;
		atom(t, vars);
		t = body[i]->o;
		atom(t, vars);
	}

	TRACE(dout << vars.size() << " vars" << endl;)


	if(body.size() == 0){
		return fact(vars.at(head->s), vars.at(head->o));
	}
	if(body.size() == 1)
	{
		return ruleproxyOne(vars, head, body);
	}
	else if(body.size() > 1)
	{
		return ruleproxyMore(vars, head, body);
	}
	else
	{
		assert(false);
	}
}

void compile_kb(old::prover *p)
{
	setproc(L"generate_pred_index");
	TRACE(dout << "..." << endl;)
	TRACE(dout << "# of rules: " << p->heads.size() << endl;)

	//old::prover --> pred_index (preprocessing step)
	for (size_t i = 0; i < p->heads.size(); i++)
	{
		old::nodeid pr = p->heads[i]->p;
		TRACE(dout << "adding rule for pred [" << pr << "]" << old::dict[pr] << "'" << endl;)
		if(pred_index.find(pr) == pred_index.end()){
			pred_index[pr] = rulesindex();
		}

		pred_index[pr].push_back(i);
	}

	//pred_index --> preds (compilation step)
	for(auto x: pred_index){
		TRACE(dout << "Compling pred: " << old::dict[x.first] << endl;)
		pred_t tmp_pred = pred(x.first);
		if(!tmp_pred){
			assert(false);
		} 
		preds[x.first] = tmp_pred;
		if(!preds[x.first]){
			assert(false);
		}
	}

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
}

