//#include <iostream>
//#include <string>
#include <functional>
#include <map>
#include "prover.h"

using namespace std;

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

class Thing{
public:
	virtual Thing *getValue(){return this;};
	virtual wstring str(){return L"wut";}
};

typedef function<bool(Thing*,Thing*)> comp;

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
}//well it could run a slightly optimized version..

/*
function<bool()> nodeComp(Node *n){
	
}
*/
//we really cant use those static vars
//naturalog talked about this problem that some of the combinators need to keep some state, i dunno what the resolution was....
comp fact(Node *s, Node *o){
	int entry = 0;
	function<bool()> c1;
	function<bool()> c2;
	return [s, o, entry, c1, c2](Thing *Ds, Thing *Do) mutable{


		//then this fact, which is supposed to be JIT-compiled already,
		//is JIT-compiling 2 generalunifycoro's each time we run it

		//like, 'fact' should be a completed function, it shouldn't
		//be making anythinghmm
		//your generalunifycoro() makes a coro which we can then run
		//but we just want to run the coro here, and
		//either make one before-hand and patch it in, or just
		//run the algorithm directly here
		//actually we can't make one before-hand because it's
		//operating over arguments we don't have yet :)
//but if your only other option is to duplicate whatever's inside Var.unify and
/*generalunifycoro...it sounds like a bad move for now ?

what you would like to pre-compile here is a code that has to check if Ds and Do 
are nodes and then to the eq logic and maybe check if theyre lists and do lists unification
and check if theyre maybe vars and do vars unification

yea basically
so we can set that up later with macros if its really a good idea
alright that sounds good, i'm not 100% sure what you have in mind but
macros might take care of some of it:.) 

yea it should be like the HMC example just err univar-ified
*/

			//we just want to run the unify algorithm
			//so we want it like this, but with a
			//version of generalunifycoro that just runs
			//the coro rather than makes a coro

			//bt i can see what you mean...but..hmm....
			//these Ds and Do are later arguments we get
			//and the fact is what's supposed to be the JIT
			//compiled function which takes them, we don't
			//JIT within the JIT
			//well since we dont have the D's beforehand,
			//theres nothing we can do
			//well, we *could* run it like this, if we
			//were planning to like, reuse the query
			//at some later point.. but that's not this phase :)
			//there's nothing wrong per se with jitting within
			//the jitted functions, it's just an extra lyaer
			//of stuff to what we're doing right now and
			//seems to be the only reason for the static problem
//just work out how to store them without the statics? did you read the SO?
//yea, i'm still not sure why we would be generating any functions inside of
//fact though; by the time fact is run, all function generating should be done,
//and 'each' fact is just one of those generated functions
//this would be like using the jitted knowledgebase to jit-compile the query
//that actually sounds good:) haha it does but i don't think we're there yet XD
//i'm still on a thing that's called baby steps lol
//well i need to ponder it a bit, maybe while napping...:)



			//this isn't correct code
			//basically this is what it should 'look like'
			//right now you have generalunifycoro as a function
			//that returns functions
			//but we should have a generalunifycoro that just runs
			//not necessarily to replace the function-making one
			//we already have but, another function that runs
			//what the generalunifycoro would be making
//i dunno, it doesnt make sense to me
// :/ that's how it has to happen though
//i mean, it doesn't *have* to happen like that, but all we're doing
//by making these other coros



		switch(entry){
		case 0:
		 c1  = generalunifycoro(Ds,s);
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

//so.. ok..
//im not sure why i even laid it out yet, we'll need to do some
//work to figure out handling the different permutations
/*ok well its still quite mysterious to me
comp join(comp a, comp b, Var *v){
	int entry = 0;
	return[a, b, v, entry](Thing *Ds, Thing *Do){
		switch(entry){
		case 0:*/
			//Different permutations for the
			//unifying var..
			//all it does is make sure that
			//both a and b are satisfied at the same time
			//as opposed to 'seq' which does a, and then does b
//ok and why do we need multiple permutations?
// ?x a ?y. ?y b ?z. vs ?y a ?x. ?z a ?y.
// this would like joinavvb and joinvabv
// i might just be overthinking it, then again i can just lay out
// each different permutation if you want
// but then i'm starting to get a bit confused about how the different
// permutations are handled when we get into to complex joins, which 
// further leads me to think i'm overthinking something here.
/* it seems to be some kind of optimization but its eluding me so far,
im reading hmcs paste..*/
// could just be an optimization
// the unifying vars seem to take care of it, i just haven't.. 'shown my work'
// 100% for that yet to make sure
// lets just go with that for now..


// different permutations of which variable joins them together
// well.. maybe haha
// yeah im starting to see the meaning of those permutations i think
// like if we had a query ?x uncle ?y, and
// {?x brother ?z. ?z parent ?y} vs // is that is_parent or has_parent?
// {?x parent ?z. ?z brother ?y} vs // exactly
// {?x brother ?z. ?y parent ?z}   // it depends on the semantics
//what does? the meaning of the statements depend on how they
//are related to by the rest of the system
//like, if i have koo7 parent koo7sMom
//but then i have koo7sUnc
// i dont see what this has to do with ...joins
// the function uncle is literally defined by a join of a particular type
// the type of join defines the semantics
// like lets say we have a parent relationship, we could either have
// Parent parent Child, or Child parent Parent
// at first you'll just have labels like Chelsea parent Hillary
// or Hillary parent Chelsea, and Hugh brother Hillary
// and depending on which ordering you choose will determine which kind of
// join you use
// like we have uncle = joinavvb(), and we pick joinavvb() because we
// know that a uncle b <= a brother v. v parent b.

// right, so, basically its just a matter of looking at the
// variables and joining them correctly

// i think theres no "joining variables", 
// clauses...right....

// basically if we have a rule like
// x uncle y <= x brother z. z parent y.
//comp uncle = join(
// then our jit should have
// uncle = joinavvb(brother, parent);
/*
yeah and the reason for these permutations is that the result is a 
function that we want to call,
in this case with two vars...kinda..maybe....
i think preds always make a function of two vars
every pred should ultimately be a function of two Things yea
 lets see how we want to call a join

the semantics of the preds has nothing to do with the reasoner we use,
much less with its jit
it is "defined" by our rules and facts
*/
//but, if we had a system (like the rest of our normal system) that was
//keeping track of the variables...
/*
			while(a(...)){
				while(b(...)){
					entry = 1;
					return true;
		case 1: ;
				}
			}
			entry = 2;
		default:
			return false;
		}
	}
}
*/

/*
comp join(comp a, comp b){
	Var v;//<-
	return join(comp a, comp b, v);
	//this one is for join(fact,fact)
}*/



//A previous join and another fact.
//{?a x ?b. ?b y ?c. ?c z ?d}
//comp complexJoin(comp a, comp b){
//what is this?
//this one is for join(join(fact,fact)),fact)h
//it might not even be necessary if we can conflate the two
//definitions, i'm not sure


//can we write an universal join first? i dont see why not

//}


/*
comp join(comp a, comp b){
now, isnt join exactly the same thing? as univars? as fact
no
join is for conjunction of facts
this fact & that fact, at the same time
{?x a man. ?x a parent} => {?x a dad}
^so, while c1(){while c2(){}}
yea
actually a fact here is i guess a special case of a join
its conjunction of unifying the subject & unifying the object
whereas the 'join' is two facts
so, same logic of while{while{}}
whereas 'seq' is while{} while{}
	[a, b, entry]
	while(a()){
		while(b()){

		}
	}

join gets a bit weird because there's different permutations of
where the join variable can appear

like {?x p ?y. ?y p ?z} vs. {?y p ?x. ?z p ?y}
see HMC's joinavvb and joinvabv


yea this probably makes the most sense
in hindsight it's what i started trying to do when i made my
reeeaaally contrived examples at the very beginning;




yeah understood...also...it seems that here, this is the level from which we 
might be able to optimize the fact thing.. sure :) plenty of possibilities here
here we know one is a var and another a node..so..and we make the var...etc..and then can create an optimized fact?




possibly, i'm not entirely sure what you have in mind
me neither i'm not too worried about optimization yet, it'll already be
very fast just with the basic setup
ok so just make the nonoptimized nonstatic fact :)
we'll see how it meshes out
}
*/

/*
i do it this way because Var.unifcoro result is an iterator, that at first call binds
and at second unbinds, so i have to keep that iterator around, between calls to person
*//*
int test1(int argc, char** argv){
	Node *_Chelsea = new Node("Chelsea");
	Node *_Hillary = new Node("Hillary");
	Node *_Bill = new Node("Bill");
	Node *_Tony = new Node("Tony");
	Node *_Hugh = new Node("Hugh");
	Node *_Roger = new Node("Roger");
	
	int entry = 0;
	function<bool(Var *)> person = 
	[_Chelsea, _Hillary, _Bill, entry](Var *person) mutable{
		//cout << "im in ur person" << entry << " person: " << person << " " << endl;
		static function<bool()> c1 = person->unifcoro(_Chelsea);
		static function<bool()> c2 = person->unifcoro(_Hillary);
		static function<bool()> c3 = person->unifcoro(_Bill);

		switch(entry){
		case 0:
			while(c1())
			{
				//cout << "unified person with chelsea" << endl;
				entry = 1;
				return true;
		case 1: ;
			}
			while(c2()){
    				//cout << "and hillary" << endl;
				entry = 2;
				return true;
		case 2: ;
			}
			while(c3()){
    				//cout << "and bill" << endl;
				entry = 3;
				return true;
		case 3: ;
			}
			entry = 4;
		default:
			entry = 0;
			//cout << "person is done" << endl;
			return false;
		}
	}; 

	entry = 0;
	comp brother = 
	[_Hillary, _Tony, _Hugh, _Bill, _Roger, entry](Thing *p, Thing *brother) mutable{
		static function<bool()> c1 = generalunifycoro(p, _Hillary);
		static function<bool()> c2 = generalunifycoro(brother, _Tony);
		static function<bool()> d1 = generalunifycoro(p, _Bill);
		static function<bool()> d2 = generalunifycoro(brother, _Roger);
		static function<bool()> d3 = generalunifycoro(brother, _Hugh);


		switch(entry){
		case 0:
			while(c1())
			{
				while(c2())
				{
					entry = 1;
					return true;
		case 1: ;
				}
			}
			while(d1()){
				while(d2()){
					entry = 2;
					return true;
		case 2: ;
				}
				while(d3()){
					entry = 3;
					return true;
		case 3: ;
				}
			}

			entry = 4;
		default:
			entry = 0;		
			return false;
		}
	};

	entry = 0;
	comp parent =  //thats expected....i think the static vars arent reinitialized...you call the same lambda again...so...yeah shouldn't it do the same thing each time? well they are kinda contrived examples arent they? ah true ok i think...not sure...but it seems to me the fault is in them not in the design..heh...dunno, well, you started on combinatorsla ggingyet? yea
	[_Chelsea, _Hillary, _Bill, entry](Thing *p, Thing *parent) mutable{
		static function<bool()> c1 = generalunifycoro(p,_Chelsea);
		static function<bool()> c2 = generalunifycoro(parent,_Hillary);
		static function<bool()> c3 = generalunifycoro(parent,_Bill);
		
		switch(entry){
		case 0:
			while(c1()){
				while(c2()){
					entry = 1;
					return true;
		case 1: ;
				}
				while(c3()){
					entry = 2;
					return true;
		case 2: ;
				}
			}
			entry = 3;
		default:
			return false;
		}
	};

	comp brotherSeq =
	seq(fact(_Hillary,_Tony),
	 seq(fact(_Bill,_Roger),fact(_Bill,_Hugh)));
}
*/
/*so it seems we have 3 variants to start with:
 1: parameterless joins, with extra rule lambda and unify's,
 2: all joins have two parameters, there are permutations,
 3: joins have only as many parameters as they need, there are even more permutations
so i guess we start with 1?
so you have a fourth way?
*/

/*

comp joinwxyz(comp a, comp b, Thing w, Thing x, Thing y, Thing z){
    int entry = 0;
			//This should be taking Ds & Do args
    return [a,b,w,x,y,z,entry](){
	switch(entry){
	    while(a(w,x)){
		while(b(y,z)){
	    }
    }
}
*/

//so, going for the no permutations option now,

comp compile(old::termid x)
{}


typedef std::map<old::nodeid, Var> varmap;
comp compile_rule(old::termid head, old::prover::termset body)
{
/*
//why not, it just overwrites the "old" one
map is a list of <key,value> aha yes now i see :)
if they were both ?a, then they would be the same key, and therefore
map to the same value, and therefore when we call body[4].o its going
to the same place?:)yep ok cool :)
*/

varmap vars;
vars[head->s->p] = Var();
vars[head->o->p] = Var();
size_t i;
for (i = 0; i < body.size(); i++)
{
    vars[body[i]->s->p] = Var();
    vars[body[i]->o->p] = Var();
}

//simplest case, there is 1 body item

comp c0 = compile(body[0]);

/*here we are in a function that compiles rules
you cant unify arguments to those in-future maybe-multiple-times called rules with anything here*/

a = vars[body[0].s]
b = vars[body[0].o]
s = vars[head.s]
o = vars[head.o]

//right just like this, but after we're done generating this function
//we don't have way to access s & o to instantiate them? when we want
//instantiate what?
//we'll use this to make a function like
//'bool uncle(Thing *Ds, Thing *Do)'

//and down in main we call uncle(something, something)
//but we have to be able to say what something and something are
//but now if we go to parameterless then uncle really looks like
//'bool uncle()'
//and we'd call it like:
//noA  no no 
//*the joins will be parameterless, not the rule lambda*
//ok so then i think im maybe confused where you're going here
//ok ok...
//all vars that appear in head or body of rule go into the map


//to call our function down in main
//first
return [ c0, a,b , s,o]   (Thing *Ds , Thing *Do)
{
/*this is the per-rule lambda, this is the price for doing it without any permutations
we are called with somethings, and we unify them onto our headthings
and maybe the headthings are also the a's and b's
a-ha.. ok i think i see now:)*/

    s.unify(Ds);
    o.unify(Do);
    while c0(a,b)...
}


/*
ok im taking a break ok ill read over it and see if i can
fill in anything else
*/


b0s = body[0].s
    
if isvar(b0s) vars[b0s] = Variable()...shrug
comp c1 = compile(body[1])
c0 = joinwxyz(c0, c1, b0s



//or lets try recursively?


struct jo{
comp coro;
varmap vars;
}

jo jointerms(term a, term b)
{
varmap v;
v[a.s] = Variable();
v[a.o] = Variable();
v[b.s] = Variable();
v[b.o] = Variable();
comp coro = joinwxyz(fact(a.p), fact(b.p), v[a.s], v[a.o], v[b.s], v[b.o]);
return new jo(coro, v);
}

jo jointermwithjoin(term a, jo b)
{

}

rulecoro
{
return [vars  ](Thing *headx, Thing *heady){
}
}







	/*  comp uncle = joinavvb(parent, bro); this, exactly?
		while(uncle(x, y)())
	if we had an universal join it would be
		comp uncle = join(parent, bro)
		while(uncle(x, z, z, y)())
	what about
		while(uncle(x, z, y, z)())yeah:)
		while(uncle(z, x, y, z)())
		while(uncle(z, x, z, y)())*/
//we could just as well use this universal join
//i'm thinking more like
 while(a(w,x)){
    while(b(y,z)){
	}
 }
/*
where w,x,y,z can each reference any Thing i guess
yeas, and the you call the whole thing with 4 parameters as above
oh i see 
yea
so..its not like im arguing for doing it this way but i think it would be good to start off with this universal join and see where it breaks
well i think the join should be universal but the uncle pred should still
take two args?
the pred has to be 2 args basically why? because that's what it does
the args are subject and object
well..that does sound like a good argument, where would we call uncle from?
it would end up getting run when we do queries, like if i do:
query: ?x uncle ?y, that runs our uncle function we're constructing here
i don't yet see the full picture for more complex queries and more
complex RHS's of =>'s but as far as 1 spo queries and LHS's of =>'s i get the picture, and basically preds become functions which run when we do the query


hmhmhm yeah
so we'll have something like
{?a pred ?b} <= { ?banana fly ?orange. ?potato xxx ?yyy . ?b c ?d}.

nosw,o this will be two joins, but only one of the arguments to
pred comes up, and there's no joining variables, and 5 of the variables
on the right don't appear on the left

if i have { LHS } => { RHS }
can i use variables on the RHS that don't appear on the LHS? sure
do they mean anything? can they mean anything?hm
maybe i should ask hmc
we should always ask hmc :)im gonna take a break
ok, yea we'll need to think on this one a little bit

i don't think variables that appear on the RHS but don't appear on the LHS
really mean anything
if we have a rule with one pred on the RHS, then
the LHS just ends up as a big lambda of joins
i think we would do this for each rule where that pred appears on the
RHS, even if there are other preds
so if we have { LHS } => {pred1. pred2.}
or, maybe we have to do a join over the RHS too

:) :) 



	really it just needs to instantiate a variable for each ? variable
	and keep using it in the right place
//well, at least i now see what these permutations are doing, although
i still dont see the whole picture
like...is it just so its nicer to call or 
joins is what makes it so that the variables must be satisfied simultaneously
like rather than x brother z being satisfied by some x and z, and then
z and y being satisfied by some completely different pair
z reappears in (x,z) and (z,y), so it needs to be the same in both

well thats obvious that's all the joins is
while(we just need to make this part independent of permutations){
 while{
 }
}
*/

/*
i had it working with joins too on my local version but they were
just HMC's joins with my patch to make up for not having unifyingvars
we
ll...keeep going i guess, well, need to fit it into your framework here
let me go read the so again but...i think that we declare comp c1 comp c2,
capture them, then initialize them on step 0 of the fact coro?
and*/

	/*
	Var _Person;

	while(person(&_Person)){
		cout <<  _Person.str() << " is a person" << endl;
	}
	*/
	/*
	cout << endl;

	Var _Person3;
	while(person(&_Person3)){
		cout << _Person3.str() << " is a person again." << endl;
	}

	*/
	//Var _Brother;

/*	in c++ terms, we dont call this thing with the arguments again and again, just once
and it returns an iterator-coro
makes sense?*/ //ah sure
	/*
	while(brother(_Hillary, &_Brother))
	{
		cout << "Hillary has brother: " << _Brother.str() << "." << endl;
	}
	*/
	/*
	Var _Brother2;

	while(brother(_Bill, &_Brother2))
	{
		cout << "Bill has brother: " << _Brother2.str() << "." << endl;
	}
	*/
	/*
	Var _Person2;
	Var _Brother3;
.
	while(brother(&_Person2, &_Brother3)){
		cout << "Person: " << _Person2.str() << ", Brother: " << _Brother3.str() << endl;
	}
	*/
	Var _Test1;
	Var _Test2;	

	cout << "Brother, sequenced: " << endl;
	while(brotherSeq(&_Test1, &_Test2)){
		cout << "Person: " << _Test1.str() << ", Brother: " << _Test2.str() << endl;
	}

	//down here instead of
	while(brotherSeq(&_Test1, &_Test2)){

	//we'd do like
/*
why?
ah you mean we avoid the rule proxy-lambda? proxy?yes the per-rule lambda whose only purpose
is this unification
ah no we would still use the per-rule lambda, but this is how we'd
instantiate the arguments to the function we generate from it
ok why? because i see how to get around the permutations this way but
i dont see how to get around the permutations otherwise
but

	BrotherSeqArg1.unify(<w/e>)
	BrotherSeqArg2.unify(<w/e>)
	while(brotherSeq){
*/
	//at least that's what i had in mind so far just for
	//basic manual construction

}




/*	
	int i,j=5;
	for (i =0; i < 3; i++)
		[j]() mutable {
			static int x = 333;
			cout << x++ << " " << i << endl;
	}();
*/

//so this is apparently how statics behave, they are shared across
//no the only point was to see if the x is shared across the 3 lambdas or not ah
//and likewise our c1 c2 etc will be shared across each fact we make?right
//hrmm well hold up

prover::prover ( old::qdb qkb, bool check_consistency)  {
    p = old::prover(qkb, false);
}
void prover::query(const qdb& goal, subs * s);

};
