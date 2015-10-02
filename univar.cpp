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

std::vector<comp> predGens;

int nterms = 0;
std::queue<old::termid> predQueue;






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
		TRACE(dout << this << "/" << this->str() << " unifcoro arg=" << arg << "/" << arg->str() <<  endl);
		if(isBound)
		{
			TRACE(dout << "isBound" << endl);
			TRACE(dout << "arg: " << arg << "/" << arg->str() << endl);
			return generalunifycoro(this, arg);
		}
		else
		{
			TRACE(dout << "not isBound" << endl);

			Thing * argv = arg->getValue();
			
			TRACE(dout << "value=" << argv << "/" << argv->str() << endl);

			if (argv == this)
			{
				TRACE(dout << "argv == this" << endl);
				//# We are unifying this unbound variable with itself, so leave it unbound.^M
				int entry = 0;
				return [this, entry, argv]() mutable{
					setproc(L"unify lambda 1");
					TRACE(dout << "im in ur argv == this var unify lambda, entry = " << entry << ", argv= " << argv << "/" << argv->str() << endl);
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
					TRACE(dout << "im in ur var unify lambda, entry = " << entry << ", argv=" << argv << "/" << argv->str() << endl);

					switch(entry)
					{


						case 0:
							isBound = true;
							value = argv;
							TRACE(dout << "binding " << this << "/" << this->str() << " to " << value << endl);
							entry = 1;
							return true;
						default:
							TRACE(dout << "unbinding " << this << "/" << this->str() << endl);
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
bool fail_with_args(Thing *_s, Thing *_o)
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
			TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << "Do: " << Do << "/" << Do->str() << endl);
			while(c1()){
				while(c2()){
					TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << "Do: " << Do << "/" << Do->str() << endl);
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


comp pred(old::termid x)
{
	setproc(L"pred");
	size_t index = nterms++;//predQueue.size();
	predQueue.push(x);
	old::nodeid dbgx = x->p;
	TRACE(dout << "constructing pred proxy for nodeid " << x->p << endl);
	int entry = 0;
	comp z;
	return [entry, z, dbgx, index](Thing *Ds, Thing *Do)mutable{
		setproc(L"pred lambda");
		TRACE(dout << "im in ur pred proxy for nodeid " << dbgx << ", index: " << index << " entry: " << entry << endl;)
		
		switch(entry)
		{
		case 0:
			entry++;
			z = predGens[index];
			bool r;
			while(true)
			{
				TRACE((dout << "calling hopefully x:" << old::dict[dbgx] << endl));
				TRACE(dout << "Ds: " << Ds << "/ " << Ds->str() << ", Do: " << Do << "/" << Do->str() << endl);
				r = z(Ds, Do);
				if (! r) goto out;
				TRACE(dout << "pred coro for " <<  old::dict[dbgx] << " success" << endl;)
				TRACE(dout << "Ds: " << Ds << "/ " << Ds->str() << ", Do: " << Do << "/" << Do->str() << endl);
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
		setproc(L"joinOne lambda");
		switch(entry){
		case 0:
			TRACE(dout << "OUTER -- w: " << "/" << w->str() << ", x: " << x << "/" << x->str() << endl);
			while(a(w,x)){
				TRACE(dout << "INNER -- w: " << w << "/" << w->str() << ", x: " << x << "/" << x->str() << endl);
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
			
			suc = generalunifycoro(Ds, s);
			ouc = generalunifycoro(Do, o);
			
			TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << endl); 
			TRACE(dout << "Do: " << Do << "/" << Do->str() << ", o: " << o << "/" << o->str() << endl);
			while(suc()) {//tbh i never went thoroughly thru the join stuff you did
//and i made this loop over the arg unification like a ruleproxy should
//well, tbh, i was looking at this func earlier and realized i hadn't gone through it thoroughly :)lol
				TRACE(dout << "After suc() -- " << endl);
				TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << endl)
				TRACE(dout << "Do: " << Do << "/" << Do->str() << ", o: " << o << "/" << o->str() << endl)
				while (ouc()) {
					TRACE(dout << "After ouc() -- " << endl);
					TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << endl); 
					TRACE(dout << "Do: " << Do << "/" << Do->str() << ", o: " << o << "/" << o->str() << endl);
					while (c0()) {
						TRACE(dout << "After c0() -- " << endl);
						TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << endl); 
						TRACE(dout << "Do: " << Do << "/" << Do->str() << ", o: " << o << "/" << o->str() << endl);
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

		join c0 = joinwxyz(pred(body[0]), pred(body[1]), a,b,c,d);

		// proxy coro that unifies s and o with s and o vars
		return joinproxy(c0, s, o);
}


comp ruleproxyOne(varmap vars, old::termid head, old::prover::termset body){

		setproc(L"comp ruleproxyOne");
		TRACE(dout << "compiling ruleproxyOne" << endl);

		Var *s = vars[head->s->p];
		Var *o = vars[head->o->p];

		Var *a = vars[body[0]->s->p];
		Var *b = vars[body[0]->o->p];

		join c0 = joinOne(pred(body[0]),a,b);
		return joinproxy(c0,s,o);
}

comp ruleproxyMore(varmap vars, old::termid head, old::prover::termset body){
	//we'll wanna do a regular join first

		setproc(L"comp ruleproxyMore");
		TRACE(dout << "compiling ruleproxyMore" << endl);

		Var *s = vars[head->s->p];
		Var *o = vars[head->o->p];

		if(head->s->p == head->o->p){
			assert(s == o);
		}

		int k = body.size()-2;
		Var *a = vars[body[k]->s->p];
		Var *b = vars[body[k]->o->p];
		Var *c = vars[body[k+1]->s->p];
		Var *d = vars[body[k+1]->o->p];
		
		join c0 = joinwxyz(pred(body[0]), pred(body[1]), a,b,c,d);
		for(int i = k-1; i >= 0; i--){
		Var *vs = vars[body[i]->s->p];
		Var *vo = vars[body[i]->o->p];
		comp p = pred(body[i]);
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
	
	//yea yea we might not need the 's' one i just want to make sure
	//vars[head->s->p] = (vars.find(head->s->p) != vars.end()) ? atom(head->s->p) : vars[head->s->p];
	//vars[head->o->p] = (vars.find(head->o->p) != vars.end()) ? atom(head->o->p) : vars[head->o->p];
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
//predskb, a map from preds to a vector of indexes of rules with that pred in the head
/*void compile_kb()
{
	setproc(L"compile_kb");
	for (auto x: predskb)
	{
		old::nodeid k = x.first;
		preds[k] = pred(k);//just queue it up, get the lookuping lambda
	}
}*/

/*imo this should infiloop, without ep check, on any recursive example
step 2 should be to not do it per pred but per term(with unification, checking if a rule is relevant)*/
void compile_preds(old::prover *p){
	setproc(L"compile_preds");

	while(!predQueue.empty()){
		old::termid x = predQueue.front(); predQueue.pop();
		rulesindex rs = predskb[x->p];
		comp r;
		bool first = true;

		//compile each rule with the pred in the head, seq them up
		for (size_t i: rs)
		{


			assert(p->heads[i]->s->p);
			assert(x->s->p);
			if (!(
						atom(p->heads[i]->s->p)->unifcoro(atom(x->s->p))()
					&&
						atom(p->heads[i]->o->p)->unifcoro(atom(x->o->p))()
				)) {
				TRACE(dout << "wouldnt match" << endl;)
				continue;
			}



			comp y = rule(p->heads[i], p->bodies[i]);

			if(first){
				first = false;
				TRACE(dout << "first, nodeid: " << x->p << "(" << old::dict[x->p] << ")" << endl;)
				r = y;
			}
			else{
				TRACE(dout << "seq, nodeid: " <<  x->p << "(" << old::dict[x->p] << ")" << endl;)
				r = seq(r,y);
			}
		}
		if (first) // cant leave it empty
			predGens.push_back(fail_with_args);
		predGens.push_back(r);
	}
}

/*writes into predskb*/
//i see
void gen_pred2rules_map(old::prover *p)
{
	setproc(L"gen_pred2rules_map");
	TRACE(dout << "gen predskb" << endl);

	int i;
	//start at the end of the kb//irrelevant?
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
	//compile_kb();
	//compile_preds(p);
	if(check_consistency) dout << "consistency: mushy" << endl;
}

//ok so it only supports one-pred queries at the moment    y
//so for multi-pred queries i think we'll build it up into a join yeah
void yprover::query(const old::qdb& goal){
	dout << "query" << endl;
	const old::prover::termset g = p->qdb2termset(goal);
	results.first.clear();results.second.clear();
	int nresults = 0;
	old::nodeid pr = g[0]->p;

	if (predskb.find(pr) != predskb.end()) {
		//varmap vars;
		//putting them to vars should be irrelevant at this point, all the pointers have been captured
		//
		Var *s = atom(g[0]->s->p);
		Var *o = atom(g[0]->o->p);
		/*if g[0]->o->p == g[0]->s->p: s = o;*/
		dout << "query 1: (" << pr << ") " << old::dict[g[0]->p] << endl;
		auto coro = pred(g[0]);
		compile_preds(p);
		dout << "query --  arg1: " << s << "/" << s->str() << ", arg2: " << o << "/" << o->str() << endl;
		//this is weird, passing the args over and over
		while (coro(s,o)) {
			nresults++;
			if (nresults > 123) {dout << "STOPPING at " << nresults << " results." << endl;break;}
			dout << L"RESULT " << nresults << ":";
			dout << old::dict[g[0]->s->p] << L": " << s << ", " << s->str() << ",   ";
			dout << old::dict[g[0]->o->p] << L": " << o << ", " << o->str() << endl;

			auto sv = s->getValue();
			auto ov = o->getValue();
			Node *n1 = dynamic_cast<Node*>(sv);
			Node *n2 = dynamic_cast<Node*>(ov);
			if (n1 && n2)
			{
				old::pquad q = make_shared<old::quad>(old::quad(
				std::make_shared<old::node>(old::dict[n1->value]),
				std::make_shared<old::node>(old::dict[pr]),
				std::make_shared<old::node>(old::dict[n2->value])));
				results.first[L"@default"]->push_back(q);
			}
		}
	}
	dout << "thats all, folks, " << nresults << " results." << endl;

}


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
