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
std::map<old::nodeid, generator> preds;


typedef function<bool()> join;
typedef function<join()> join_gen;

typedef std::unordered_map<old::termid, Var*> varmap;

typedef vector<size_t> rulesindex;
std::unordered_map<old::nodeid, rulesindex> pred_index;


function<bool()> generalunifycoro(Thing*, Thing*);
void atom(old::termid n, varmap &vars);


//std::unordered_map<old::termid, generator> preds;

std::unordered_map<old::nodeid, Var*> globals;



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
		value = s;
	}
	wstring str(){return op->format(value);}
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
							TRACE(dout << "binding " << this << "/" << this->str() << " to " << value << "/" << value->str() << endl);
							entry = 1;
							return true;
						case 1:
							TRACE(dout << "unbinding " << this << "/" << this->str() << endl);
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



wstring sprintVar(wstring label, Var *v){
	wstringstream s;
	s << label << ": (" << v << ")" << v->str();
	return s.str();
}

wstring sprintPred(wstring label, old::nodeid pred){
	wstringstream s;
	s << label << ": (" << pred << ")" << old::dict[pred];
	return s.str();
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
	TRACE(dout << "..." << endl);
	
	int entry = 0;
	return [a,b, entry]() mutable{
		setproc(L"unifjoin lambda");
		TRACE(dout << "entry = " << entry << endl);

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

//these? Var.unifcoro()
//yea its one of these
function<bool()> listunifycoro(List *a, List *b)
{
	setproc(L"listunifycoro");
	TRACE(dout << "im in ur listunifycoro" << endl);
	//gotta join up unifcoros of vars in the lists
	if(a->nodes.size() != b->nodes.size())
		return fail;
	//must be int or cant go negative to exit loop
	int i;
	//we'll start at the end of the list
	
	function<bool()> r;
	bool first = true;

	for(i = b->nodes.size()-1;i >= 0; i--) // we go from end  already..
	{
		//we need a join that operates on parameterless coros
		//your turn:D :) ok lemme see what i can do
		//this is the only place we would use it we might as well just make it specially in here

		//well i guess we can just join up generalunifcoros for a start

		//but it's here! D:
		//but uc is closer to the beginning
		join uc = generalunifycoro(a->nodes[i], b->nodes[i]);	
		//interesting:)
		
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
	a = a->getValue();
	b = b->getValue();
	Var *a_var = dynamic_cast<Var*>(a);
	if (a_var)
		return a_var->unifcoro(b);
	Var *b_var = dynamic_cast<Var*>(b);
	if (b_var)
		return b_var->unifcoro(a);

	//# Arguments are "normal" types.
	Node *n1 = dynamic_cast<Node*>(a);
	Node *n2 = dynamic_cast<Node*>(b);

	if(n1&&n2)
	{
		if(n1->eq(n2))
			return succeed();
		else
			return fail;
	}
	
	List *l1 = dynamic_cast<List*>(a);
	List *l2 = dynamic_cast<List*>(b);

	if (l1&&l2)
		return listunifycoro(l1, l2);	

	return fail;
	
}


comp fact(Thing *s, Thing *o){
	setproc(L"fact");
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
				TRACE(dout << "MATCH c1() " << endl);
				while(c2()){
					TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << "Do: " << Do << "/" << Do->str() << endl);
					entry = 1;
					TRACE(dout << "MATCH" << endl);
					return true;
		case 1: ;
		TRACE(dout << "RE-ENTRY" << endl);
				}
			}
			entry = 666;
			TRACE(dout << "DONE." << endl);
			return false;
		default:
			assert(false);
		}
	};
}

comp seq(comp a, comp b){
	setproc(L"seq");
	TRACE(dout << ".." << endl);
	int entry = 0;
	int round = 0;
	comp ac, bc;
	return [a, b, entry, round, ac, bc](Thing *Ds, Thing *Do) mutable{
		setproc(L"seq lambda");	
		round++;
		TRACE(dout << "round: " << round << endl);
		
		switch(entry){
		case 0:
			while(a(Ds, Do)){
				TRACE(dout << "MATCH A." << endl);
				entry = 1;
				return true;
		case 1: ;
			}

			while(b(Ds, Do)){
				entry = 2;
				TRACE(dout << "MATCH B." << endl);
				return true;
		case 2:	;
			}

			TRACE(dout << "SWITCH DONE." << endl);

			entry = 666;
			return false;

		default:
			assert(false);
		}
		TRACE(dout << "Why are we here?" << endl);
		assert(false);
	};
}


//This was called something else before? node, clashed with old namespace gotcha
void atom(old::termid n, varmap &vars){
	Var* r = vars[n] = new Var();
	if (n->p>0)
	{
		r->isBound = true;
		//so rather than have the value be a new Node you'd have it be a new List?yes cool
		if(*old::dict[n->p].value == L".")
		{
			std::vector<Thing*> nodes;
			for(auto y:op->get_dotstyle_list(n)) {
				atom(y, vars);
				nodes.push_back(vars[y]);
			}
			r->value = new List(nodes);
		}
		else
			r->value = new Node(n);
	}
}


comp rule(old::termid head, old::prover::termset body);

comp compile_pred(old::termid x) {
	setproc(L"compile_pred");
	rulesindex rs = pred_index[x->p];
	comp r;
	bool first = true;

	//compile each rule with the pred in the head, seq them up
	for (int i = rs.size()-1;i>=0; i--) {

		comp y = rule(op->heads[i], op->bodies[i]);

		if (first) {
			first = false;
			TRACE(dout << "first, nodeid: " << x->p << "(" << old::dict[x->p] << ")" << endl;)
			r = y;
		}
		else {
			TRACE(dout << "seq, nodeid: " << x->p << "(" << old::dict[x->p] << ")" << endl;)
			r = seq(y, r);
		}
	}
	if (first) // cant leave it empty
		return (fail_with_args);
	return r;
}


typedef function<comp()> pred_t;

pred_t pred(old::termid x)
{
	setproc(L"pred");
	TRACE(dout << "constructing pred proxy for nodeid " << x->p << endl);
	return [x]()mutable{
		setproc(L"pred lambda");
		TRACE(dout << "nodeid " << x->p << endl);

		return compile_pred(x);
	};
}

join_gen joinOne(pred_t a, Thing* w, Thing *x){
	setproc(L"joinOne");
	TRACE(dout << "..." << endl);
	int entry = 0;
	int round = 0;
	comp ac;
	return [ac, a, w, x, entry, round]() mutable{
		setproc(L"joinOne gen");
		return [ac, a, w, x, entry, round]() mutable{
			setproc(L"joinOne λ λ");
			round++;
			TRACE(dout << "round=" << round << endl);
			switch(entry){
			case 0:
				TRACE(dout << "OUTER -- w: " << "/" << w->str() << ", x: " << x << "/" << x->str() << endl);
				ac = a();
				while(ac(w,x)){
					TRACE(dout << "INNER -- w: " << w << "/" << w->str() << ", x: " << x << "/" << x->str() << endl);
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


join_gen joinwxyz(pred_t a, pred_t b, Thing *w, Thing *x, Thing *y, Thing *z){
	setproc(L"joinwxyz");
	TRACE(dout << "making a join" << endl);
	int entry = 0;
	int round = 0;
	comp ac, bc;
	return [a,b,w,x,y,z,entry, round, ac,bc]()mutable{
		setproc(L"join lambda");
		return [a,b,w,x,y,z,entry, round, ac,bc]()mutable{
			setproc(L"join lambda lambda");
			round++;
			TRACE(dout << "round: " << round << endl);
			switch(entry){
			case 0:
				ac = a();
				while(ac(w,x)) {
					bc = b();
					TRACE(dout << "MATCH A." << endl);
					while (bc(y, z)) {
						TRACE(dout << "MATCH." << endl);
						entry = 1;
						return true;
			case 1: ;
					TRACE(dout << "RE-ENTRY" << endl);
					}
				}
				entry = 666;
				TRACE(dout << "DONE." << endl);
				return false;
			default:
				assert(false);
			}
		};
	};
}

comp ruleproxy(join_gen c0_gen, Var *s, Var *o){
	setproc(L"ruleproxy");
	TRACE(dout << "ruleproxy" << endl);
	join c0;
	int entry=0;
	int round=0;
	// proxy coro that unifies s and o with s and o vars
	function<bool()> suc, ouc;
	return [ suc, ouc, entry, c0_gen, c0, s,o,round]   (Thing *Ds , Thing *Do) mutable
	{
		setproc(L"ruleproxy lambda");
		round++;
		TRACE(dout << "round=" << round << endl);
		switch(entry)
		{
		case 0:
			entry++;
			
			suc = generalunifycoro(Ds, s);

			
			TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << endl); 
			TRACE(dout << "Do: " << Do << "/" << Do->str() << ", o: " << o << "/" << o->str() << endl);
			while(suc()) {//tbh i never went thoroughly thru the join stuff you did
				TRACE(dout << "After suc() -- " << endl);
				TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << endl)
				TRACE(dout << "Do: " << Do << "/" << Do->str() << ", o: " << o << "/" << o->str() << endl)
				ouc = generalunifycoro(Do, o);
				while (ouc()) {
					TRACE(dout << "After ouc() -- " << endl);
					TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << endl); 
					TRACE(dout << "Do: " << Do << "/" << Do->str() << ", o: " << o << "/" << o->str() << endl);
					c0 = c0_gen();
					while (c0()) {
						TRACE(dout << "After c0() -- " << endl);
						TRACE(dout << "Ds: " << Ds << "/" << Ds->str() << ", s: " << s << "/" << s->str() << endl); 
						TRACE(dout << "Do: " << Do << "/" << Do->str() << ", o: " << o << "/" << o->str() << endl);
						entry = 1;
						return true;
						TRACE(dout << "MATCH." << endl);
		case 1: ;
			TRACE(dout << "RE-ENTRY" << endl);
					}
				}
			}
			entry = 666;
			TRACE(dout << "DONE." << endl);
			return false;
		default:
			assert(false);
		}
	};
}

join_gen halfjoin(pred_t a, Var* w, Var* x, join_gen b){
	setproc(L"halfjoin");
	TRACE(dout << "..." << endl);
	int entry = 0;
	int round = 0;
	comp ac;
	join bc;
	return [a, w, x, b, entry, round,ac,bc]() mutable{
		setproc(L"halfjoin gen");
		return [a, w, x, b, entry, round,ac,bc]() mutable{
			setproc(L"halfjoin lambda");
			round++;
			TRACE(dout << "round=" << round << endl);
			switch(entry){
			case 0:
				ac = a();
				while(ac(w,x)){
					TRACE(dout << "MATCH a(w,x)" << endl);
					bc = b();
					while(bc()){
						entry = 1;
						TRACE( dout << "MATCH." << endl);
						return true;
			case 1: ;
				TRACE(dout << "RE-ENTRY" << endl);
				  }
				}
				entry = 666;
				TRACE(dout << "DONE." << endl);
				return false;
			default:
				assert(false);
			}
		};
	};
}

comp ruleproxyTwo(varmap vars, old::termid head, old::prover::termset body)
{/*case for two body items*/
		setproc(L"comp ruleproxyTwo");
		TRACE(dout << "compiling ruleproxyTwo" << endl);

		Var *s = vars[head->s];
		Var *o = vars[head->o];

		Var *a = vars[body[0]->s];
		Var *b = vars[body[0]->o];
		Var *c = vars[body[1]->s];
		Var *d = vars[body[1]->o];

		auto c0 = joinwxyz(pred(body[0]), pred(body[1]), a,b,c,d);

		// proxy coro that unifies s and o with s and o vars
		return ruleproxy(c0, s, o);
}


comp ruleproxyOne(varmap vars, old::termid head, old::prover::termset body){

		setproc(L"comp ruleproxyOne");
		TRACE(dout << "compiling ruleproxyOne" << endl);

		Var *s = vars[head->s];
		Var *o = vars[head->o];

		Var *a = vars[body[0]->s];
		Var *b = vars[body[0]->o];

		join_gen c0 = joinOne(pred(body[0]),a,b);
		return ruleproxy(c0, s, o);
}

comp ruleproxyMore(varmap vars, old::termid head, old::prover::termset body){
	//we'll wanna do a regular join first

		setproc(L"comp ruleproxyMore");
		TRACE(dout << "compiling ruleproxyMore" << endl);

		Var *s = vars[head->s];
		Var *o = vars[head->o];

		int k = body.size()-2;
		Var *a = vars[body[k]->s];
		Var *b = vars[body[k]->o];
		Var *c = vars[body[k+1]->s];
		Var *d = vars[body[k+1]->o];
		
		join_gen c0 = joinwxyz(pred(body[0]), pred(body[1]), a,b,c,d);
		for(int i = k-1; i >= 0; i--){
			Var *vs = vars[body[i]->s];
			Var *vo = vars[body[i]->o];
			pred_t p = pred(body[i]);
			c0 = halfjoin(p,vs,vo, c0);
		}
		return ruleproxy(c0, s, o);
}

comp rule(old::termid head, old::prover::termset body)
{
	setproc(L"comp rule");
	TRACE(dout << "compiling rule " << op->format(head) << " " << body.size() << endl;)
	//we make a per-rule varmap
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


	if(body.size() == 0){
		return fact(vars[head->s], vars[head->o]);
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

void generate_pred_index(old::prover *p)
{
	setproc(L"generate_pred_index");
	TRACE(dout << "..." << endl);

	for (size_t i = 0; i < p->heads.size(); i++)
	{
		old::nodeid pr = p->heads[i]->p;
		if(pred_index.find(pr) != pred_index.end()){
			pred_index[pr] = rulesindex();
		}
		pred_index[pr].push_back(i);
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

yprover::yprover ( qdb qkb, bool check_consistency)  {
	dout << "constructing old prover" << endl;
	op=p = new old::prover(qkb, false);
	generate_pred_index(p);
	if(check_consistency) dout << "consistency: mushy" << endl;
}

void yprover::query(const old::qdb& goal){
	dout << KGRN << "Query." << KNRM << endl;

	results.clear();

	const old::prover::termset g = p->qdb2termset(goal);
	int nresults = 0;
	old::nodeid pr = g[0]->p;

	if (pred_index.find(pr) != pred_index.end()) {


		TRACE(dout << sprintPred(L"Making pred",pr) << "..." << endl);
		varmap vars;
		atom(g[0]->s, vars);
		atom(g[0]->o, vars);
		Var *s = vars[g[0]->s];
		Var *o = vars[g[0]->o];

		auto coro = pred(g[0])();

		TRACE(dout << sprintPred(L"Run pred: ",pr) << " with  " << sprintVar(L"Subject",s) << ", " << sprintVar(L"Object",o) << endl);
		// this is weird, passing the args over and over
		while (coro(s,o)) {
			nresults++;

			dout << KCYN << L"RESULT " << KNRM << nresults << ":";
			dout << old::dict[g[0]->s->p] << L": " << s << ", " << s->str() << ",   ";
			dout << old::dict[g[0]->o->p] << L": " << o << ", " << o->str() << endl;
			
			if (nresults >= 123) {dout << "STOPPING at " << nresults << KRED << " results." << KNRM << endl;break;}
 
		}

		thatsAllFolks(nresults);
	}else{
		
		dout << "Predicate '" << old::dict[pr] << "' not found, " << KRED << "0" << KNRM << " results." << endl;
	}
}

/*
void yprover::clear_results()
{

	results.first[L"@default"] = old::mk_qlist();


pnode thing2node(Thing *t, qdb &r) {
	auto v = t->getValue();
	List *l = dynamic_cast<List *>(v);
	if (l) {
		const wstring head = id();
		for (auto x: l->nodes)
			r.second[head].push_back();
		return mkbnode(pstr(head));
	}
	Node *n = dynamic_cast<Node *>(v);
	if (n)
		return std::make_shared<old::node>(old::dict[n1->value]);
}


void add_result(qdb &r, Thing *s, Thing o, old::nodeid p)
{
	old::pquad q = make_shared<old::quad>(old::quad(
				thing2node(
				std::make_shared<old::node>(old::dict[pr]),
				std::make_shared<old::node>(old::dict[n2->value])));
				results.first[L"@default"]->push_back(q);
			}
	r.first[L"@default"]->push_back(q)
*/


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



how stuff works:


	auto T = atom(op->make(mkiri(pstr(L"T")), 0,0));
	auto F = atom(op->make(mkiri(pstr(L"F")), 0,0));

	auto tnf = fact(T, F);
	auto fnt = fact(F, T);
	auto tnf2 = fact(T, F);
	auto fnt2 = fact(F, T);

	auto s1 = seq(tnf,fnt);

	Var * x = new Var();
	Var * xx = new Var();
	Var * y = new Var();
	Var * yy = new Var();

	while(s1(x, xx)) {
		auto s2 = seq(tnf2, fnt2);
		while (s2(y, yy)) {
			dout << x->str() << ", " << y->str() << endl;
		}
	}
	dout << "======================================================================================" << endl;





*/
/*

i'm thinking something like

struct fact{

}

struct seq{

}

struct join{

}

struct halfjoin{
	fact current; 
	join rest;
}
1) ok, but as a solution to what?
so, we have some issues with the current setup
for one the way we're trying to keep things around and having to recompile and all that is kinda messy
not to mention that we don't really need to recompile each time, hmc said that jit was really more wrt block-changes
//mkay, but it should work i think there's some issues with how we set it up, i think maybe some things aren't
//getting generated like they're supposed to, but we could solve this vars issue this way also
//im all for solving the vars issue, just not to run away from a problem i dont see/understand, well, that's why i've
//been trying to determine where exactly in the code we're going wrong, because we could continue with our current method
//its just not ideal, basically our current method works but we have a control flow bug seems to be something not getting
//restarted as its supposed to be, but we could just put things into this other structure and perhaps just avoid these
//bugs altogether
i would say go ahead and work your idea out ...but i will focus on this here for now
gonna go fix up some food now ok sounds good

2) wouldnt hmc say that now you have an interpreter not a compiler?
no these would just take the place of our coro generators and would hold coros/run them
*/



/*
//skip rules that wouldnt match
bool wouldmatch(old::termid t, size_t i) {
	assert(p->heads[i]->s->p);
	assert(x->s->p);
	if (!(
			atom(p->heads[i]->s)->unifcoro(atom(x->s))()
			&&
			atom(p->heads[i]->o)->unifcoro(atom(x->o))()
	)) {
		TRACE(dout << "wouldnt match" << endl;)
		return false;
	}
	else
		return true;
}
*/

/*
generator factGen(Thing *s, Thing *o){
	
}*/

/*
function<bool()> nodeComp(Node *n){
	
}
*/
		///*if g[0]->o == g[0]->s: s = o;*/


/*writes into preds*/
//ok so by the time we get here, we'll already have
//pred_index, a map from preds to a vector of indexes of rules with that pred in the head
/*void compile_kb()
{
	setproc(L"compile_kb");
	for (auto x: pred_index)
	{
		old::nodeid k = x.first;
		preds[k] = pred(k);//just queue it up, get the lookuping lambda
	}
}*/

//actually maybe only the join combinators need to do a lookup
