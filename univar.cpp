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





class Thing{
public:
	virtual Thing *getValue(){return this;};
	virtual wstring str(){return L"wut";}
};

class Var;

typedef function<bool(Thing*,Thing*)> comp;
std::map<old::nodeid, comp> preds;


typedef function<bool()> join;
typedef std::unordered_map<old::nodeid, Var*> varmap;


typedef vector<size_t> rulesindex;
std::unordered_map<old::nodeid, rulesindex> predskb;

function<bool()> generalunifycoro(Thing*, Thing*);








class Node:public Thing
{
public:
	old::nodeid value;
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



Var* atom(old::nodeid n){
    Var* r = new Var();
    if (n>0)
    {
	r->isBound = true;
	r->value = new Node(n);
    }
    return r;
}

comp pred(old::nodeid x)
{
	int entry = 0;

	//this looks up and proxies a pred
	return [entry, x](Thing *Ds, Thing *Do)mutable{
		comp z;
		switch(entry)
		{
			case 0:
			    entry++;
			    z = preds[x];
			    while(z(Ds, Do))
			    {
				return true;
			case 1:;
			    }
			    entry++;
		}
		return false;
	};
}




function<bool()> joinwxyz(comp a, comp b, Thing *w, Thing *x, Thing *y, Thing *z){
    int entry = 0;
    return [a,b,w,x,y,z,entry]()mutable{
	switch(entry){
	    case 0:
		entry++;
		while(a(w,x)){
		    while(b(y,z)){
			return true;
	    case 1:;
		    }
		}
	}
	return false;
    };
}
//actually maybe only the join combinators need to do a lookup



comp ruleproxy(varmap vars, old::termid head, old::prover::termset body)
{

	    Var *s = vars[head->s->p];
	    Var *o = vars[head->o->p];

	    Var *a = vars[body[0]->s->p];
	    Var *b = vars[body[0]->o->p];
	    Var *c = vars[body[1]->s->p];
	    Var *d = vars[body[1]->o->p];

	    join c0 = joinwxyz(pred(body[0]->p), pred(body[1]->p), a,b,c,d);

	    int entry=0;
	    // proxy coro that unifies s and o with s and o vars
	    return [ entry, c0, s,o]   (Thing *Ds , Thing *Do) mutable
	    {
		switch(entry)
		{
		    case 0:
			entry++;
	    		s->unifcoro(Ds)();
			o->unifcoro(Do)();
			while( c0())//call the join
			{
	    		    return true;
		    case 1:;
			}
		}
		return false;
	    };
}




comp rule(old::termid head, old::prover::termset body)
{
	cout << "rule " << endl;;//head << " " << body << endl; 
	varmap vars;

	//hrmm
	//it needs to work with the table of preds
	

	//ok now not everything is necessarily vars
	//sometimes we'll have literals too

	//these two are just proxies for whatever input we get
	vars[head->s->p] = new Var();
	vars[head->o->p] = new Var();
	size_t i;
	for (i = 0; i < body.size(); i++)
	{
	    old::termid t = body[i]->s;
	    vars[t->p] = atom(t->p);
	    t = body[i]->o;
	    vars[t->p] = atom(t->p);
	}


	if(body.size() == 0){
	    //this case would represent an actual kb 'fact'
	    return fact(vars[head->s->p], vars[head->o->p]);
	}
	if(body.size() == 1){
	    //need to do the var shuffle
	    
	    assert(false);

	    //return //pred(body[0]->p));
	}
	else if(body.size() == 2)
	{
	    return ruleproxy(vars, head, body);
	}
	else if(body.size() > 2){
	    /*todo
    	    int k = body.size()-2;

	    //i guess should just have joinwxyz as compile(term,term)
	    //seems reasonable//or not

	    comp c0 = joinwxyz(pred(body[k]),pred(body[k+1]));
	    while(k > 0)
	        c0 = halfjoin(pred(body[k]),c0);
	        k--;
	    */
	    assert("explosion happened");
	}else{
	    //???? ???:)
	    cout << "Negative body size? What is this, American TV?" << endl;
	}
}


/*writes into preds*/
void compile_kb(old::prover *p)
{
    cout << "compile" << endl;
    for (auto x: predskb)
    {
	old::nodeid k = x.first;
	rulesindex rs = x.second;
	for (size_t i: rs)
	{
	    comp r = rule(p->heads[i], p->bodies[i]);
	    if(preds.find(k) != preds.end())
		preds[k] = seq(preds[k], r);
	    else
		preds[k] = r;
	}
    }
}


/*writes into predskb*/
void gen_pred2rules_map(old::prover *p)
{
    cout << "gen predskb" << endl;
    size_t i;
    for (i = 0; i < p->heads.size(); i++)
    {
	old::nodeid pr = p->heads[i]->p;
	auto it = predskb.find(pr);
	if (it == predskb.end())
	    predskb[pr] = rulesindex();
	predskb[pr].push_back(i);
    }
}

yprover::yprover ( old::qdb qkb, bool check_consistency)  {
    cout << "constructing old prover" << endl;
    p = new old::prover(qkb, false);
    gen_pred2rules_map(p);
    compile_kb(p);
}

void yprover::query(const old::qdb& goal, old::subs * s){
    //auto g = *(goal.first[L"@default"]);//qlist
    //while(join(m, (term(i) for i in g))())
    cout << "query" << endl;
    const old::prover::termset g = p->qdb2termset(goal);
    varmap m;
    //just one for now
    while(pred(g[0]->p)(atom(g[0]->s->p), atom(g[0]->o->p)))
	for(auto v: m)
	    wcout << old::dict[v.first].value << L": " << v.second->str();
};
