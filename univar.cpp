#include <functional>
#include <unordered_map>
#include "prover.h" // namespace "old"

using namespace std;



/*so it seems we have 3 variants to start with:
 1: parameterless joins, with extra rule lambda and unify's,
 2: all joins have two parameters, there are permutations,
 3: joins have only as many parameters as they need, there are even more permutations
so i guess we start with 1?
*/





class Thing{
public:
	virtual Thing *getValue(){return this;};
	virtual wstring str(){return L"wut";}
};

typedef vector<size_t> rulesindex;
typedef function<bool(Thing*,Thing*)> comp;
std::unordered_map<old::nodeid, rulesindex> predskb;
typedef std::unordered_map<old::nodeid, Var> varmap;
std::map<old::nodeid, comp> preds;








class Node:public Thing
{
public:
	old::termid value;
	bool eq(Node *x)
	{
		return x->value == value;
	}
	Node(old::termid s)
	{
		value = s;
	}
	wstring str(){return *old::dict[value->p].value;}
};

function<bool()> generalunifycoro(Thing*, Thing*);

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
		if(isBound)
		{
			return generalunifycoro(this, arg);
		}
		else
		{
			Thing * argv = arg->getValue();
			
			if (argv == this)
			{
				//# We are unifying this unbound variable with itself, so leave it unbound.^M
				int entry = 0;
				return [this, entry, argv]() mutable{
					switch(entry){
						case 0:
							value = argv;//???
					                entry = 1;
							return true;
						default:
							entry = 0;
							return false;
	
					}
				};
			}
			else
			{
				int entry = 0;
				return [this, entry, argv]() mutable{ 
					switch(entry)
					{

						case 0:
							isBound = true;
							value = argv;
							//cout << "binding " << this << " to " << value << endl;
							entry = 1;
							return true;
						case 1:
							
						default:
							//cout << "unbinding " << this << endl;
							isBound = false;
							entry = 0;
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

function<bool()> succeed()
{//yield once
    int entry = 0;
    return [entry]() mutable{
    switch(entry){
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
		default:
			return false;
		}
	};
}

comp seq(comp a, comp b){
	int entry = 0;
	return [a, b, entry](Thing *Ds, Thing *Do) mutable{	
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

comp joinwxyz(comp a, comp b, Thing w, Thing x, Thing y, Thing z){
    int entry = 0;
    return [a,b,w,x,y,z,entry](Thing *Ds, Thing *Do){
	switch(entry){
	    while(a(w,x)){
		while(b(y,z)){
		    return true;
	    }
	}
	return false;
    };
}
//actually maybe only the join combinators need to do a lookup




Var node(old::termid t){
    Var r;
    if (t->p>0)
    {
	r.isBound = true;
	r.value = new Node(t);//leak
    }
    return r;
}

comp pred(old::nodeid x)
{
	int entry = 0;

	//this looks up and runs a rule
	return [entry, x](Thing *Ds, Thing *Do)mutable{
		switch(entry){
			...
	    auto z = rules[x];
	    while(z(Ds, Do))
		return true;
			...
	    return false;
			...
		}
	};
}






comp rule(old::termid head, old::prover::termset body)
{

	varmap vars;

	//hrmm
	//it needs to work with the table of preds
	

	//ok now not everything is necessarily vars
	//sometimes we'll have literals too

	//these two are just proxies for whatever input we get
	vars[head->s->p] = Var();
	vars[head->o->p] = Var();
	size_t i;
	for (i = 0; i < body.size(); i++)
	{
	    old::termid t = body[i]->s;
	    vars[t->p] = compile_node(t);
	    t = body[i]->o;
	    vars[t->p] = compile_node(t);
	}


	//simplest case, there is 1 body item
	comp c0;
	//this case would represent an actual kb 'fact', yes?
	if(body.size() == 0){
	    c0 = fact(vars[head->s->p], vars[head->o->p]);
	}
	if(body.size() == 1){
	    c0 = compile(term(body[0]));
	}
	//this looks right
	else if(body.size() == 2){
	    c0 = joinwxyz(body[0],body[1]);
	}
	else if(body.size() > 2){
    	    int k = body.size()-2;

	    //i guess should just have joinwxyz as compile(term,term)
	    //seems reasonable

	    comp c0 = joinwxyz(body[k+1],body[k+2]);
	    while(k >= 0){
	        c0 = halfjoin(body[k],c0);
	        k--;
	    }
	}else{
	    //???? ???:)
	    cout << "Negative body size? What is this, American TV?" << endl;
	}
	/*here we are in a function that compiles rules
	you cant unify arguments to those in-future maybe-multiple-times
 	called rules with anything here*/
	
	Var a = vars[body[0]->s->p];
	Var b = vars[body[0]->o->p];
	Var s = vars[head->s->p];
	Var o = vars[head->o->p];



	return [ c0, a,b , s,o]   (Thing *Ds , Thing *Do) mutable
	//rule proxy coro that unifies s and o with s and o vars
	{
	    s.unifcoro(Ds)();
	    o.unifcoro(Do)();
	    while( c0(a,b))
	    	    return true;
	}

}



void compile_kb(old::prover::kb kb)
{
    for (auto x: predkb)
    {
	nodeid k = x.first;
	rulesindex rs = x.second;
	for (size_t i: rs)
	{
	    comp r = rule(kb.bodies[i]);
	    if(preds.has(k))
		preds[k] = seq(preds[k], r);
	    else
		rules[k] = r;
	}
    }
}



void gen_pred2rules_map(prover p)
{
    for (i = 0; i < p.kb.heads.size(); i++)
    {
	nodeid pr = p.kb.heads[i]->p;
	auto it = predskb.find(pr);
	if it == predskb.end()
	    predskb[pr] = new rulesindex();
	predskb[pr].push_back(i);
    }
}

prover::prover ( old::qdb qkb, bool check_consistency)  {
    //prover::p
    p = old::prover(qkb, false);
    size_t i;
    compile_kb();
}

void prover::query(const qdb& goal, subs * s){
    auto g = goal["@default"];
    varmap m;
    while(join(m, (term(i) for i in g))())
	for(auto v: m)
	    cout << v.first.str() << ": " << v.second.str();
};

// we will simply try to use prover as a kb
// yea im just kinda mulling it over
// do we need to do work on our classes to integrate to that
// structure or you already took care of that?
/*
well we use stuff like term and termid and nodeid..
what other structure?
well this is preliminary, i will see when it compiles if it also links and then its done i guess
well doesn't ours operate over strings or.. you take the pointers so its the same?
well....yes it was strings and its nodeids now i think..or termids..still have to think it thru
its just a hack
well, maybe since we got the basic structure down and this is mostly
a big document of notes maybe we should just rewrite it into that
framework? theres no framework
hmm, i guess i mean to pull up master and 
afaik master has : experimental lambdas in prover, unintegrated p.cpp(?),
and i dunno if anything else (interesting?) it has the whole rest of
the system of parsing and all that stuff? it's a complete program
taht we add a part to,
 ambiguous cli is better ok that works :)
its just a hack...if it doesnt work out we scrap it
not sure what you mean just a hack
we have the system in ambiguous_cli with the whoel
command line, parser, and qdb generator, and that
qdb works off integer pointers so we should write it
into the prover.cpp there
write what what's in this univar.cpp here and what we're going
to make with the joins
well we make a new prover class here...whats the matter about what file it
is in, and also, we will keep the old prover for marpa
*/

/*ok we can try to continue where we left off yesterday i guessssss
lets leave it for a bit, i want to consult with naturalog about
his plans for 'real jit'ok, unless you're gung-ho to get started :)
well, not immediately but i guess i will want to 
btw wrt naturalog, he seems to be just reading up on lots of stuff,
ive read a bit about jitting prolog and im certain hmc has too
im 100% positive our version is important, and the only other
meaningful version would be this but native emit; i haven't read
hardly anything about it, but yea this version is important and i agree
about native emit
i dunno if naturalog is collecting resources to help with the native emit
version or hes just totally lost:) what do you think i think? :) i just
want to know if he's got anything so far
hehe i see, from my convo with him yesterday or whenever that was it 
didnt sound like he had much, if anything; seems to spend most of his
time reading about stats and watching physics lectures anyway, not bad
hobbies but they don't make a jit hehe
ok so then maybe i should consult with HMC on that instead, but...
i imagine a response along the lines of "where's the current jit?"
or maybe i should just scrap this whole plan to consult in the 
first-place lol
it would be nice if we could actually get naturalog to be working with
us though.. ever since we started the jit it's felt like a competition
his fault to begin with, but, i let it get the better of me and
now me and him are in somewhat of an adversarial relationship it seems,
or, rival at least.. and that's my fault
idk, maybe i just feel bad for snapping at him the other day and now
i'm trying to re-involve him
*/





/*
b0s = body[0].s
if isvar(b0s) vars[b0s] = Variable()...shrug
comp c1 = compile(body[1])
c0 = joinwxyz(c0, c1, b0s
struct jo{
	comp coro;
	varmap vars;
}
jo jointerms(term a, term b)
{//this was an attempt at generating joins recursively
	varmap v;
	v[a.s] = Variable();
	v[a.o] = Variable();
	v[b.s] = Variable();
	v[b.o] = Variable();
	
	comp coro = joinwxyz(fact(a.p), fact(b.p), v[a.s], v[a.o], v[b.s], v[b.o]);
	return new jo(coro, v);
	//does jo need a constructor?i dont know
}//it doesnt seem to do any recursion tho
jo jointermwithjoin(term a, jo b){
}
rulecoro{
	return [vars  ](Thing *headx, Thing *heady){
	}
}
*/




/*
comp term(old::termid x)
{
    for(auto y:p.heads)
	if (
	    y->p == x->p 
	    && node(y->s).unifcoro(node(x->s))())
//	    && node(y->s).unifcoro(node(x->s))()	    
		//add it to the seq
}
*/
