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
	virtual wstring str(){return L"wut";}
};

class Var;

typedef function<bool(Thing*,Thing*)> comp;
typedef function<comp()> generator;

std::map<old::nodeid, comp> preds;


typedef function<bool()> join;
typedef std::unordered_map<old::nodeid, Var*> varmap;


typedef vector<size_t> rulesindex;
//std::unordered_map<old::nodeid, function<bool(Thing*,Thing*)> predskb;
std::unordered_map<old::nodeid, rulesindex> predskb;



function<bool()> generalunifycoro(Thing*, Thing*);

std::unordered_map<old::nodeid, generator> predGens;






class Node:public Thing
{
public:
	old::nodeid value;//hmm
	bool eq(Node *x)
	{
		return x->value == value;
	}
	Node(old::nodeid s)
	{
		value = s;
	}
	wstring str(){return *old::dict[value].value;}
};


class Var:public Thing{
public:
	bool isBound = false;

	Thing *value;

	Thing *getValue()
	{
		if (isBound)
			return value;
		else
			return this;
	}

	wstring str(){return L"var("+(isBound?value->str():L"")+L")";}

	function<bool()> unifcoro(Thing *arg){
		setproc(L"Var.unifcoro");		
		TRACE(dout << this << " unifcoro arg=" << arg <<  endl);
		if(isBound)
		{
			TRACE(dout << "isBound" << endl);
			return generalunifycoro(this, arg);
		}
		else
		{
			TRACE(dout << "not isBound" << endl);

			Thing * argv = arg->getValue();
			
			TRACE(dout << "value=" << argv << endl);

			if (argv == this)
			{
				TRACE(dout << "argv == this" << endl);
				//# We are unifying this unbound variable with itself, so leave it unbound.^M
				int entry = 0;
				return [this, entry, argv]() mutable{
					setproc(L"unify lambda 1");
					TRACE(dout << "im in ur argv == this var unify lambda, entry = " << entry << endl);
					switch(entry){
						case 0:
							//value = argv;//??? // like, this.value?
							entry = 1;
							return true;
						default:
							return false;
					}
				};
			}
			else
			{
				TRACE(dout << "argv != this" << endl);
				
				int entry = 0;
				return [this, entry, argv]() mutable{
					setproc(L"unify lambda 2"); 
					TRACE(dout << "im in ur var unify lambda, entry = " << entry << endl);

					switch(entry)
					{


						case 0:
							isBound = true;
							value = argv;
							TRACE(dout << "binding " << this << " to " << value << endl);
							entry = 1;
							return true;
						default:
							TRACE(dout << "unbinding " << this << endl);
							isBound = false;
							return false;
					}
				};
			}
		}
	}
};








bool fail()
{
	return false;
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
			default:
				return false;
		}
	};
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
	//cout << "u " << a << " " << b << endl;
	a = a->getValue();
	b = b->getValue();
	//cout << "v " << a << " " << b << endl;
	//Ohad was complaining about this, any thoughts about that?that its fine
	Var *a_var = dynamic_cast<Var*>(a);
	//cout << "a_var" << a_var << endl;
	if (a_var)
		return a_var->unifcoro(b);
	Var *b_var = dynamic_cast<Var*>(b);
		if (b_var)
				return b_var->unifcoro(a);
	//# Arguments are "normal" types.
	Node *n1 = dynamic_cast<Node*>(a);
	Node *n2 = dynamic_cast<Node*>(b);
	if(n1->eq(n2))
		return succeed();
	else
		return fail;
}

/*
function<bool()> nodeComp(Node *n){
	
}
*/


comp fact(Thing *s, Thing *o){
	int entry = 0;
	function<bool()> c1;
	function<bool()> c2;
	return [s, o, entry, c1, c2](Thing *Ds, Thing *Do) mutable{
		setproc(L"fact lambda");
		TRACE(dout << "im in ur fact,  entry: " << entry << endl);

		switch(entry){
		case 0:
			c1 = generalunifycoro(Ds,s);
			c2 = generalunifycoro(Do,o);
			while(c1()){
				while(c2()){
					entry = 1;
					return true;
		case 1: ;
				}
			}
			entry++;
		default:
			return false;
		}
	};
}

comp seq(comp a, comp b){
	int entry = 0;
	return [a, b, entry](Thing *Ds, Thing *Do) mutable{
		setproc(L"seq lambda");	
		TRACE(dout << "im in ur seq, entry: " << entry << endl);

		switch(entry){
		case 0:
			while(a(Ds, Do)){
				entry = 1;
				return true;
		case 1: ;
			}
			while(b(Ds, Do)){
				entry = 2;
				return true;
		case 2:	;
			}
			entry = 3;
		default:
			return false;
		}
	};
}


//This was called something else before? node, clashed with old namespace gotcha
Var* atom(old::nodeid n){
	Var* r = new Var();
	if (n>0)
	{
	r->isBound = true;
	r->value = new Node(n);
	}
	return r;
}


//this looks up and proxies a pred
//we capture in the lambda the key into preds[]
comp pred(old::nodeid x)
{
	setproc(L"comp pred");
	TRACE(dout << "constructing pred proxy for nodeid " << x << endl);
	int entry = 0;
	comp z;
	return [entry, z, x](Thing *Ds, Thing *Do)mutable{
		setproc(L"pred lambda");
		TRACE(dout << "im in ur pred proxy for nodeid " << x << ", entry: " << entry << endl;)
		
		switch(entry)
		{
		case 0:
			entry++;
			z = predGens[x]();
			//z = preds[x];
			bool r;
			while(true)
			{
				TRACE((dout << "calling hopefully x:" << old::dict[x] << endl))
				r = z(Ds, Do);
				if (! r) goto out;
				TRACE(dout << "pred coro for " <<  old::dict[x] << " success" << endl;)
				return true;
		case 1: ;

			}
		out: ;
			entry++;
		}
		return false;
	};
}

join joinOne(comp a, Thing* w, Thing *x){
	int entry = 0;
	return [a, w, x, entry]() mutable{
		switch(entry){
		case 0:
			while(a(w,x)){
				entry = 1;
				return true;
		case 1: ;
			}
		}
		return false;
	};
}


join joinwxyz(comp a, comp b, Thing *w, Thing *x, Thing *y, Thing *z){
	setproc(L"joinwxyz");
	TRACE(dout << "making a join" << endl);
	int entry = 0;
	return [a,b,w,x,y,z,entry]()mutable{
		setproc(L"join lambda");
		TRACE(dout << "im in ur join, entry: " << entry << endl);
		switch(entry){
		case 0:
			while(a(w,x)) {
				while (b(y, z)) {
					entry = 1;
					return true;
		case 1: ;
				}
			}
		}
		return false;
	};
}
//actually maybe only the join combinators need to do a lookup

//should be called ruleproxy after all? minus the name clash sure? i didn't see ruleproxy had become ruleproxytwo :)ah:)
comp joinproxy(join c0, Var* s, Var* o){
	int entry=0;
	// proxy coro that unifies s and o with s and o vars
	function<bool()> suc, ouc;
	return [ suc, ouc, entry, c0, s,o]   (Thing *Ds , Thing *Do) mutable
	{
		setproc(L"joinproxy lambda");
		TRACE(dout << "im in ur joinproxy, entry=" << entry << endl);
		switch(entry)
		{
		case 0:
			entry++;
			suc = s->unifcoro(Ds);
			ouc = o->unifcoro(Do);
			while(suc()) {//tbh i never went thoroughly thru the join stuff you did
//and i made this loop over the arg unification like a ruleproxy should
//well, tbh, i was looking at this func earlier and realized i hadn't gone through it thoroughly :)lol
				while (ouc()) {
					while (c0()) {
						entry = 1;
						return true;
		case 1: ;
					}
				}
			}
		}
		return false;
	};
}

join halfjoin(comp a, Var* w, Var* x, join b){
	int entry = 0;
	return [a, w, x, b, entry]() mutable{
		switch(entry){
		case 0:
			while(a(w,x)){
			   while(b()){
				entry = 1;
				return true;
		case 1: ;
			  }
			}
		}
		return false;
	};
}

comp ruleproxyTwo(varmap vars, old::termid head, old::prover::termset body)
{/*case for two body items*/
		setproc(L"comp ruleproxyTwo");
		TRACE(dout << "compiling ruleproxyTwo" << endl);

		Var *s = vars[head->s->p];
		Var *o = vars[head->o->p];

		Var *a = vars[body[0]->s->p];
		Var *b = vars[body[0]->o->p];
		Var *c = vars[body[1]->s->p];
		Var *d = vars[body[1]->o->p];

		join c0 = joinwxyz(pred(body[0]->p), pred(body[1]->p), a,b,c,d);

		// proxy coro that unifies s and o with s and o vars
		return joinproxy(c0, s, o);
}


comp ruleproxyOne(varmap vars, old::termid head, old::prover::termset body){

		setproc(L"comp ruleproxyTwo");
		TRACE(dout << "compiling ruleproxyTwo" << endl);

		Var *s = vars[head->s->p];
		Var *o = vars[head->o->p];

		Var *a = vars[body[0]->s->p];
		Var *b = vars[body[0]->o->p];

		join c0 = joinOne(pred(body[0]->p),a,b);
		return joinproxy(c0,s,o);
}

comp ruleproxyMore(varmap vars, old::termid head, old::prover::termset body){
	//we'll wanna do a regular join first

		setproc(L"comp ruleproxyMore");
		TRACE(dout << "compiling ruleproxyMore" << endl);

		Var *s = vars[head->s->p];
		Var *o = vars[head->o->p];

		int k = body.size()-2;
		Var *a = vars[body[k]->s->p];
		Var *b = vars[body[k]->o->p];
		Var *c = vars[body[k+1]->s->p];
		Var *d = vars[body[k+1]->o->p];
		
		join c0 = joinwxyz(pred(body[0]->p), pred(body[1]->p), a,b,c,d);
		for(int i = k-1; i >= 0; i--){
		Var *vs = vars[body[i]->s->p];
		Var *vo = vars[body[i]->o->p];
		comp p = pred(body[i]->p);
		c0 = halfjoin(p,vs,vo, c0);
		}
		return joinproxy(c0,s,o);
}

comp rule(old::termid head, old::prover::termset body)
{
	setproc(L"comp rule");
	TRACE(dout << "compiling rule " << op->format(head) << " " << body.size() << endl;) 
	varmap vars;

	//ok now not everything is necessarily vars
	//sometimes we'll have literals too

	//these two are just proxies for whatever input we get
	vars[head->s->p] = atom(head->s->p);
	vars[head->o->p] = atom(head->o->p);
	size_t i;
	for (i = 0; i < body.size(); i++)
	{
		old::termid t;
		t = body[i]->s;
		vars[t->p] = atom(t->p);
		t = body[i]->o;
		vars[t->p] = atom(t->p);
	}


	if(body.size() == 0){
		//this case would represent an actual kb 'fact'
		return fact(vars[head->s->p], vars[head->o->p]);
	}
	if(body.size() == 1)
	{
		return ruleproxyOne(vars, head, body);
	}
	else if(body.size() == 2)
	{
		return ruleproxyTwo(vars, head, body);
	}
	else if(body.size() > 2)
	{
		return ruleproxyMore(vars, head, body);
	}
	else
	{
		assert(false);
	}
}


/*writes into preds*/
//ok so by the time we get here, we'll already have
//predskb, a map from preds to a vector of indexes of rules with that pred
//rules with that pred in the rule-head or anywhere in the rule?head
void compile_kb(old::prover *p)
{
	setproc(L"compile_kb");
	for (auto x: predskb)
	{
	old::nodeid k = x.first;
	rulesindex rs = x.second;
	for (size_t i: rs) // for each rule with the pred
	{
		comp r = rule(p->heads[i], p->bodies[i]);
		if(preds.find(k) != preds.end()){
		dout << "seq, nodeid: " << k << "(" << old::dict[k] << ")" << endl;
		preds[k] = seq(r, preds[k]);
		}
		else{
		dout << "first, nodeid: " << k << "(" << old::dict[k] << ")" << endl;
		preds[k] = r;
		}
	}
	}
}

//something along these lines
void preds2gens(){
	for(auto x: preds){
		old::nodeid k = x.first;
		comp f = x.second;
		predGens[k] = [f](){
			setproc(L"preds2gens lambda");
			TRACE(dout << "..." << endl;)
			return f;
		};
	}
}

/*writes into predskb*/
//i see
void gen_pred2rules_map(old::prover *p)
{
	setproc(L"gen_pred2rules_map");
	TRACE(dout << "gen predskb" << endl);

	int i;
	//start at the end of the kb
	for (i = p->heads.size() - 1; i >= 0; i--)
	{
		old::nodeid pr = p->heads[i]->p;
		auto it = predskb.find(pr);
		if (it == predskb.end()){
			predskb[pr] = rulesindex();
		}
		predskb[pr].push_back(i);
	}


}


//namespace issue here? well, not an issue here, i put the whole old codebase into "old"..
///so why the 'y'? oh..yeah..i then added using namespace old so yeah
yprover::yprover ( qdb qkb, bool check_consistency)  {
	dout << "constructing old prover" << endl;
	op=p = new old::prover(qkb, false);
	gen_pred2rules_map(p);
	compile_kb(p);
	//so we'll do exactly like we did and then call this function to turn
	//our pred-coros into pred-coro-generators
	preds2gens();
	if(check_consistency) dout << "consistency: mushy" << endl;
}

void yprover::query(const old::qdb& goal){
	dout << "query" << endl;
	const old::prover::termset g = p->qdb2termset(goal);
	int nresults = 0;
	old::nodeid pr = g[0]->p;
	if (preds.find(pr) != preds.end()) {
		auto as = atom(g[0]->s->p);
		auto ao = atom(g[0]->o->p);
		dout << "query 1: " << pr << endl;
		auto coro = pred(pr);
		dout << "query 2" << endl;
		while (coro(as, ao)) {//this is weird, passing the args over and over
			nresults++;
			if (nresults > 5) {dout << "STOPPING at " << nresults << " results." << endl;break;}
			dout << L"RESULT " << nresults << ":";
			dout << old::dict[g[0]->s->p] << L": " << as->str() << ",   ";
			dout << old::dict[g[0]->o->p] << L": " << ao->str() << endl;
		}
	}
	dout << "thats all, folks, " << nresults << " results." << endl;

}

		//ok so it only supports one-pred queries at the moment    y
		//so for multi-pred queries i think we'll build it up into a join yeah
		//we don't need to worry about that yet though we'll just get this one
		//to stop inflooping:)
/*
how c++ lambdas work:

        int i,j=5;                                                                                                          
        for (i =0; i < 3; i++)                                                                                              
                [j]() mutable {                                                                                             
                        static int x = 333;                                                                                 
                        cout << x++ << " " << i << endl;                                                                    
        }();   


        vector<function<void()>> zzz;
        int xxx;
        auto a = [xxx, zzz]()mutable{xxx++; dout << xxx << endl; zzz[0]();};
        zzz.push_back(a);
        a();


	int xxx=0;
	auto a = [xxx]()mutable{xxx++; dout << xxx << endl;};
	auto b = a;
	auto c = a;
	a();b();c();



	int state=0;
	function<void()> xxx = [state]()mutable{dout << state++ << endl;};
	std::unordered_map<old::nodeid, function<function<void()>()>> pgs;
	pgs[0] = [xxx](){return xxx;};
	pgs[1] = [xxx](){return xxx;};
	pgs[0]()();
	pgs[1]()();



	int state=0,state2=0;
	//this is like a rule stuff
	function<void()> fff = [state2]()mutable{dout << "fff " << state2++ << endl;}; 
	//this is like a pred lambda
	function<void()> xxx = [state, fff]()mutable{dout << state++ << endl; fff();};
	std::unordered_map<old::nodeid, function<function<void()>()>> pgs;
	//this is the pred copy lambda
	pgs[0] = [xxx](){return xxx;};
	pgs[1] = [xxx](){return xxx;};
	pgs[0]()();
	pgs[1]()();


*/