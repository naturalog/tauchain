#include <iostream>
#include <vector>
#include <functional>
#include <unordered_map>


using namespace std;

#define LLL cout << endl << __FILE__ << ":" << __LINE__ << ":" << endl;

//how c++ lambdas work:

int main()
{
LLL
	{
		int i;
		for (i = 0; i < 3; i++)
			[]() mutable {
				static int x = 333;
				cout << x++ << endl;
			}();
	}
LLL
	{/*//crash
		vector<function<void()>> zzz;
		int xxx;
		auto a = [xxx, zzz]()mutable {
			xxx++;
			cout << xxx << endl;
			zzz[0]();
		};
		zzz.push_back(a);
		a();
	*/}
LLL
	{
		auto xxx = 0;
		auto a = [xxx]()mutable {
			xxx++;
			cout << xxx << endl;
		};
		auto b = a;
		auto c = a;
		a();
		b();
		c();
	}
LLL
	{
		auto xxx = 0;
		auto a = [xxx]()mutable {
			xxx++;
			cout << xxx << endl;
		};
		vector<function<void()>> ooo;
		ooo.push_back(a);
		auto c = ooo[0];
		a();
		c();
	}
LLL
	{

		vector<int> xxx;
		xxx.push_back(3);

		auto a = [xxx]()mutable {
			xxx[0]++;
			cout << xxx[0] << endl;
		};
		auto b = a;
		auto c = a;
		a();
		b();
		c();
	}
LLL
	{
		int state = 0;
		function<void()> xxx = [state]()mutable { cout << state++ << endl; };
		std::unordered_map<int, function<function<void()>()>> pgs;
		pgs[0] = [xxx]() { return xxx; };
		pgs[1] = [xxx]() { return xxx; };
		pgs[0]()();
		pgs[1]()();
	}
LLL
	{
		int state = 0, state2 = 0;
		//this is like a rule stuff
		function<void()> fff = [state2]()mutable { cout << "fff " << state2++ << endl; };
		//this is like a pred lambda
		function<void()> xxx = [state, fff]()mutable {
			cout << state++ << endl;
			fff();
		};
		std::unordered_map<int, function<function<void()>()>> pgs;
		//this is the pred copy lambda
		pgs[0] = [xxx]() { return xxx; };
		pgs[1] = [xxx]() { return xxx; };
		pgs[0]()();
		pgs[1]()();
	}
}



#ifdef fffffffffold

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
19:10 < koo7> hmc, care to sketch out the procedure of compiling a rule?
19:13 < HMC_Alpha> a single rule?
19:13 < koo7> yes
19:15 < HMC_Alpha> if the body length is zero, compile as a fact.  If the body length is 1, compile as a single unification to the body.  If the body length is 2, compile as a single full join.  If the body length is >2, compile the first two statments as a full join, and then compose the result iteratively in a half join with each additional statement.
19:17 < koo7> so much for the obvious high level stuff
19:17 < HMC_Alpha> ?
19:18 < HMC_Alpha> not sure what you mean
19:49 < koo7> i take a rule, and create a table of variables that are in its head and body, and then go over the body items and compose lambdas that capture those variables...
19:52 < HMC_Alpha> eh?
19:52 < HMC_Alpha> the variables in the head become parameters to the coro
19:52 < HMC_Alpha> the variables in the body become "local" join variables
19:54 < HMC_Alpha> so you aren't creating any variables "before" the compilation, but during

21:01 < HMC_Alpha> well it optimizes into a single lambda, it seems
21:01 < HMC_Alpha> pastebin.com/WVa0iAgk

21:18 < HMC_Alpha> fixed: pastebin.com/qDAvP3xE
21:19 < HMC_Alpha> hrmm, ofc this is not how ours would look, either, we'd always have 4 params on each
21:19  * HMC_Alpha ponders
21:20 < HMC_Alpha> so we'd actually be a little closer to the LJW one after all, heh
21:20 < HMC_Alpha> hrmm
21:20 < HMC_Alpha> yah I'll have to think about this some more, now I see what you are struggling w/


19:54 < koo7> i dont see how the variables propagate across the joins to where they are needed
19:55 < HMC_Alpha> "propogate to where they are needed"?
19:55 < HMC_Alpha> I don't understand what you mean
19:56 < koo7> {?a b ?c} <= {?d e ?f. ?g h ?i. ?a aa ?aaa. ?i h ?g}
19:56 < koo7> j1 = join(e(?d,?f),h(?g,?i); j2=join(j1, aa(?a, ?aaa)); j3=join(j2, h(?i, ?g));
19:56 < koo7> now i call j3 with a and c..
19:57 < koo7> and ?a makes its way across j2 to aa how?
19:58 < HMC_Alpha> ah, ?a is not a join variable in this example!
19:58 < koo7> so what?
19:58 < koo7> its in the head and in one of the body items
19:59 < HMC_Alpha> so it is passed into the creation of that join step directly, it is not "local" to any join
19:59 < HMC_Alpha> your join variables there are i and g
20:00 < HMC_Alpha> brb
20:02 < koo7> maybe ?a is also needed in another body item. this suggests a need for a table of variables to me?
20:07 < HMC_Alpha> probably it would help to look at how yieldprolog would actually compile these examples?
20:08 < HMC_Alpha> meaning feed it to the yp.js as prolog source and see how the resulting coro constructions look
20:08 < HMC_Alpha> if you give me a sec I can run your example through it :-)

23:01 < HMC_Alpha> the compiled code *creates* a chain of references at each result, and then yields to the caller
23:01 < HMC_Alpha> the caller follows the indirection to see the binding
23:01 < HMC_Alpha> the compiled code is then resumed and creates a new chain of references for the next result (well, really modifies the chain it has)
23:02 < HMC_Alpha> and then yields again
23:02 < HMC_Alpha> and then the *caller* again follows the indirections to see the bound result
23:02 < HMC_Alpha> and then resumes the compiled code
23:02 < HMC_Alpha> all the compiled code every does is bind and yield and bind and yield and bind and yield and....
23:02 < HMC_Alpha> until it runs out of results to bind to
23:12 < HMC_Alpha> wut?
23:12 < HMC_Alpha> we don't even want or need aot, first of all
23:13 < HMC_Alpha> second of all, the "jitted thing" (the predicates) is not what *needs* the dereferenced data
23:13 < HMC_Alpha> you could certainly have the jitted code do some dereferencing, maybe copy results into some temporary placeholder list to return to the caller at once
23:13 < HMC_Alpha> but why?
23:13 < HMC_Alpha> it's overhead you don't need on the dynamic side
23:14 < HMC_Alpha> it's more instructions that you don't *need* to jit, so why jit them?  Why not have them static, on disk, already compiled, ready to go?
23:14 < HMC_Alpha> :-)
23:14 < HMC_Alpha> why add time to the compile step for something that is entirely static?
23:14 < HMC_Alpha> (the dereferencing)
23:14 < HMC_Alpha> the jit compile step, i mean


23:19 < HMC_Alpha> we specifically do not want to be an interpreter, rememeber?
23:20 < naturalog> so what you suggest sounds similar to this latter
23:20 < HMC_Alpha> ok so this paper is at least on the correct topic
23:20 < HMC_Alpha> but
23:20 < HMC_Alpha> are we a partial evaluator?
23:20 < HMC_Alpha> NO
23:21 < HMC_Alpha> so.... still not relevant to us? :-)
23:21 < naturalog> in what sense prolog is pe and tau isnt?
23:21 < HMC_Alpha> whether or not prolog has a specializer is implementation dependent
23:22 < HMC_Alpha> but tau has no need for one
23:22 < HMC_Alpha> we do not intend to mix interpretive evalaution and compilation
23:22 < HMC_Alpha> we intend to only compile.
23:22 < HMC_Alpha> we intend *no* interpretation
23:22 < HMC_Alpha> so we have no specializer, we just compile.
23:23 < naturalog> well you suggest compiling small pieces with interpreted logic (the indirection follower) between them dont you
23:23 < HMC_Alpha> no?
23:23 < HMC_Alpha> when did I say anything of the sort?
23:23 < HMC_Alpha> we will have no interpreter.
23:23 < HMC_Alpha> we will have the aot compiled core itself, and we will have the jit compiled on-chain logic itself.  *NO* interpeter.
23:24 < HMC_Alpha> ah well maybe you write your tau core client in python or something... but that would be... "misguided"?  (I'll use nice words for it.)
23:24 < HMC_Alpha> but that is not what we're talking here.... :-)
23:24 < HMC_Alpha> we're talking about our aot *compiled* c++ core interacting with our jit *compiled* chain logic
23:24 < HMC_Alpha> not an interpeter in sight.

00:43 < HMC_Alpha> ok, so I must be tired.  With only a few minutes away from it, I realized what I was missing wrt koo, and where the confusion came in ;-)
00:43 < HMC_Alpha> we did discuss at least part of this once before, so we should dig up the logs on it as well
00:44 < HMC_Alpha> but the short answer is that the join variables do have to be allocated externally to the outer closure


05:02 < koo7> omgwtfbbq
05:07 < koo7> stoopkid, yes it was i only then realized that anything in the varmap could be just a literal, and i thought about it for a while and think we would have to permutate over the combinations of literals and vars, to avoid that for now we can either merge the functionality of node and var into thing, or, as i started doing, make everything var and bind those that are literals
05:11 < koo7> maybe, at this point, we should start looking into generating the permutations, instead of dancing around them, i think the general idea is getting clearer

11:08 < stoopkid> joins and lookup should be straight-forward but i haven't thought through all the details of order-of-operations in the kb-building process
11:11 < stoopkid> HMC_Alph1: any comments wrt the ordering here? should we be starting our building of the function at the bottom of the kb and work our way towards the top, in the same fashion that we're construct the joins from right to left?
11:12 < stoopkid> basically in general, we have to build up the functions in the opposite direction that we plan to execute them, yes?
11:13 < stoopkid> well, i guess i can answer my own question here with 'duh'
11:14 < stoopkid> but, proper ordering of building the function gets a bit tricky when we look at rules that express a recursion relation on the pred in the rule-head
11:18 < stoopkid> were there no possibility of recursive relations on preds, then if we expressed our KB as a graph of how the preds connect together, then we'd have an acyclic graph
11:19 < stoopkid> and dependencies could be tracked, and everything built up starting with independent preds and then building up the preds that depend on those
11:21 < stoopkid> but, when we hit a recursion relation like {?x a Nat. ?y a Nat. ?x eq ?y} => {?y eq ?x}
11:23 < stoopkid> HMC_Alph1: for this case, i could use some input on how exactly that should be handled wrt the order in which we build up our 'eq' function (in this case)


12:57 < HMC_Alph1> stoopkid: direct self-recursive is the easy case, the coroutine just calls a new instance of itself
13:01 < HMC_Alph1> corecursion is the hard case, because the functions need to call instances of eachother (so yes, in this case you will either need a handle functor (indirection!) or will need to use a "dirty trick" to make call site fixups)


13:09 < HMC_Alph1> gtr again for a bit, but i'll put together some recursive example when I'm back
13:11 < HMC_Alph1> the short explanation there is that it needs multiply-instantiated generators as in py/js yield semantics, which our simplified c++ semicoros "don't do by default"... but extending them as such is pretty straightforward
13:12  * HMC_Alph1 bbiab
13:14 < koo5> :)
13:15 < koo5> stoopkid, any idea what he means
13:16 < koo5> HMC_Alph1, recursion or corecursion, same problem
13:18 < koo5> we set up indirection by an additional coro, although then i thought maybe the lookup will be possible on the callsite directly
13:20 < koo5> but no idea what you mean by multiply instantiated generators
13:20 < koo5> we can eventually throw more permutations at it for optimization

18:41 < HMC_Alph> .g ncatlab function extensionality
18:41 < tau-bot> HMC_Alph: http://ncatlab.org/nlab/show/function+extensionality


03:17 < HMC_Alph> stoopkid: so maybe you guys should try doing the jit with boost asym coro and then hand-optimize them later
03:18 < HMC_Alph> instead of thinking about the "many generators of the same pred" problem head on.
03:19 < stoopkid> this package has some features we'd be looking for here?
03:21 < HMC_Alph> sure, it's coros have everything we need and then some (read: slower)
03:23 < stoopkid> HMC_Alph: looks good, why the asym over the sym? the sym looks more like what we're doing now idk what these pull_type push_type do yet
03:23 < HMC_Alph> ofc we'll want to replace it with hand specialized control flow later, but for now it would be a lot of implementation detail to not have to think about
03:27 < stoopkid> cool, we'll see what we can work up with it
03:28 < HMC_Alph> well asym just might be easier to think about in this case?
03:30 < stoopkid> i see, "pull_type transfers data from another execution context", didn't even realize we could do that


13:40 < rszeno> body is just a pot for soul, :)
13:44 < koo7> dont say pot, stoopkid will wake up and want to smoke it

17:06 < HMC_Alph> koo7: did you see my suggestion to stoopkid wrt the jit?
17:07 < HMC_Alph> to try making it using boost coros first?
17:07 < koo7> yes, i dont like it:)


17:08 < HMC_Alph> is not good for a long term, but will remove a lot of things that have to be thought about along the way in the meantime
17:09 < HMC_Alph> also did you see the article series i linked where the guy builds up an inference using cps monads?


17:13 < HMC_Alph> we could structure a small coro monad library similarly to how he builds his cps monad


17:14 < HMC_Alph> hehe, i've been looking at c++14 lambda features a bit as well for it...
polymorphic lambdas simplify a couple of aspects, but I'm not sure how I feel about relying on c++14 features XD


17:28 < HMC_Alph> anyway, I should also say that I'm still fine with the idea of "punting" and building an abstract machine and
native emit backend, and returning to finish the arch-neutral compiler later... XD
17:29 < koo7> abstract machine?
17:30 < HMC_Alph> koo7: yes, defining a lower IR than the rdf itself and compiling to this first, then emitting native code
blocks corresponding to the ir instructions

17:35 < HMC_Alph> btw is anyone here familiar with the history/origin of coroutines and the stories of "pipe day"? XD
17:35 < HMC_Alph> http://www.cs.dartmouth.edu/~doug/sieve/

04:09 < HMC_Alph> https://hackage.haskell.org/package/monadiccp-0.6.1
04:09 < HMC_Alph> (OT)
04:09 < HMC_Alph> (JIT related stuff)


05:44 < naturalog> env are copied if thats what you want to point
05:45 < HMC_Alph> well, yes, but not just that... :-)
05:45 < HMC_Alph> ok let's go a step further
05:45 < HMC_Alph> Nat
05:46 < HMC_Alph> walk me through sucessor?
05:46 < naturalog> ok so now you touch a point i didnt touch, which is builtins
05:46 < HMC_Alph> up to like S(S(Z)) or so :-)
05:46 < naturalog> i.e. for succ its for creating new terms
05:46 < HMC_Alph> no builtin, i just mean
05:47 < HMC_Alph> Z a Nat. {?x a ?Nat. ?x :s ?y.} => {?y a ?Nat}. Z :s :one. :one :s :two. :two s :three.
05:47 < HMC_Alph> ?WHO a ?WHAT?
05:48 < naturalog> you point to hitting ground?
05:49 < HMC_Alph> well hrm, that doesn't actually touch on my concern...
05:49 < naturalog> so the compiled func will also know where it hits ground (easy to precalc), and on that case, it pushes the env ptr to a ground list
05:49 < naturalog> so is it about two bodies?
05:50 < naturalog> in this case, what is euler doing? pushes a frame per head, with the first body, and when it finishes it goes up and increments the body#
05:50 < HMC_Alph> no, I guess to actually hit the concern I'd need to use a recursive example

05:51 < HMC_Alph> can you get this to a workable state so I can try banging on it a bit? :-)
05:51 < naturalog> thats what im trying to do for 1m
05:51 < HMC_Alph> XD
05:51 < naturalog> but it also took me 1m to figure out its the optimal way
05:51 < naturalog> i mentioned that env conds things here many times
05:51 < naturalog> i even wrote it in some working version
05:52 < HMC_Alph> I just don't see how it can be. :-)
05:52 < naturalog> euler is doing nothing else
05:52 < HMC_Alph> so?
05:52 < naturalog> essentially. remove all comparisons you can do beforehand, and you get that design
05:52 < naturalog> yp omits all comparisons per term
05:52 < naturalog> or per pred
05:53 < naturalog> not per pair of body-head
05:53 < naturalog> but that pair is the ultimate one as seen from euler
05:53 < HMC_Alph> per pred is *less* than per pair of body-head
05:53 < HMC_Alph> one pred is many body-head!
05:53 < naturalog> recall the abcdxy example
05:54 < naturalog> will yp omit abcxy?
05:54 < naturalog> or only x?
05:54 < HMC_Alph> i'm not sure what you mean, abcy will be bound, x will not
05:55 < naturalog> abcy will appear in the code somehow


05:55 < naturalog> ^ in yp
05:55 < HMC_Alph> as parameters that are never touched
05:56 < naturalog> not compared between the two terms?
05:56 < HMC_Alph> this is a trivial price to pay for zero indirection, and I don't see how it doesn't quickly win out ;-)
05:56 < naturalog> it compares at least the references
05:56 < naturalog> or at least it check what's a var
05:57 < naturalog> its pred-centric, not pair-centric
05:57 < HMC_Alph> it doesn't!  It knows this already...
05:57 < HMC_Alph> it structures the code by what is/isn't bound, it doesn't check it at runtime
05:57 < naturalog> it doesnt for one term, but not for the other
05:57 < HMC_Alph> ?
05:58 < naturalog> static pred gets a dynamic term and matches it
05:58 < naturalog> its not statid pred gets a static ter
05:58 < naturalog> c m
05:58 < HMC_Alph> i don't know what you mean by "static term" "dynamic term"
05:58 < HMC_Alph> we're ignoring all of YPs dynamics
05:58 < HMC_Alph> and talking only about the static compiler
05:58 < naturalog> static=compile time, dynamic=runtime
05:59 < HMC_Alph> ah, it is given a static term for all
06:00 < HMC_Alph> every variable is in place before runtime. :-|


13:09 < xtalmath> dangerous games... https://en.wikipedia.org/wiki/Albanian_Rebellion_of_1997


21:19 < HMC_Alph> when the 'a' predicate calls itself with an object argument bound to "man" it will do nothing... (with just this one rule)
21:19 < koo7> ah
21:22 < HMC_Alph> so "{?x a :man} => {?x a mortal}. :socrates a :man." will compile into roughly:
21:24 < HMC_Alph> a(s,o) = Var X; for(unify(o, mortal)) { for(a(X, man)) { yield }}; for(unify(s, socrates)) { for(unify(o, man)) { yield }};
21:25 < HMC_Alph> in the "inner" generator, o will be bound to man, so the first loop makes 0 iterations...
21:25 < HMC_Alph> without the "fact" included it will do nothing.  With the fact included it will do nothing in the first loop, and then yield up the fact to the "outer" (calling) generator


21:29 < HMC_Alph> there shouldn't be anything that will "blow up" except an unproductive recursion (explicit infinite loop)
21:30 < koo7> all lambdas should set entry back to 0 on their "end"?
21:31 < HMC_Alph> I can give you such an example trivially... "{?x :foo ?y} => {?a :foo ?b}" will "blow up" ofc...
21:31 < HMC_Alph> koo7: eh?
21:32 < stoopkid> koo7: all lambdas should ultimately get entry set back to 0; idk if this happens automatically after the coroutine has finished execution or if we need to do it explicitly when we're at the end
21:33 < koo7> i dont see what would do it "automatically" for us
21:33 < HMC_Alph> why set back to 0 or not?  Shouldn't even matter... we don't re-use any generator do we?
21:33 < stoopkid> me neither and you'll see in my previous yp stuff that i explicitly set it back to 0 at the end
21:34 < stoopkid> HMC_Alph: i'm thinking next time we want to run a query
21:34 < stoopkid> i dont think its the source of the blow up here
21:34 < koo7> HMC_Alph, how about a next rule in a sequence?
21:34 < stoopkid> if anything them not being set back to 0 should be blowing up 'even less'
21:35 < HMC_Alph> i think you are confused about something...
21:35 < HMC_Alph> koo7: the next rule in a sequence will use *another* instance of the coroutine for any preds that overlap
21:36 < stoopkid> aha


21:39 < HMC_Alph> http://pastebin.com/BURcRwsR
21:40 < HMC_Alph> 6 total instances of the foo coroutine here... ;-)
21:40 < HMC_Alph> 6 distinct generators, with distinct state. :-|
21:41 < HMC_Alph> does this explain what you are confused about, or just confuse more?
21:41 < koo6> ok cool
21:41 < koo6> its clear now
21:41 < stoopkid> i think i'm definitely de-confused a bit
21:41 < koo6> at least until i face the code which i dont feel like doing now
21:42 < HMC_Alph> this is what I was trying to explain some days ago about multiple instantiations of the coroutines... :-)



21:42 < HMC_Alph> if I had thought to do this tiny example demonstration back then I think it might've saved some time and headache, but I didn't quite realize at that point the particular detail you were overlooking ;-)
21:43 < koo6> stoopkid, i think this is only a problem of pred, since it goes into the table?
21:44 < koo7> we always create a new Var.unifcoro, always a new succeed
21:46 < koo7> everything
22:01 < koo7> hrrrm
22:01 < koo7> how to compile this..


22:05 < HMC_Alph> .g let over lambda over let over lambda
22:05 < tau-bot> HMC_Alph: http://letoverlambda.com/textmode.cl/guest/toc

22:03 < HMC_Alph> monads? XD


03:19 < rszeno> no
03:19 < rszeno> not understanding basic things and coding is a mistake
03:19 < rszeno> is blind development
03:21 < rszeno> unification is just finding a special substitution, the most general substitution
03:21 < rszeno> both forward and backward chaining use it
03:23 < rszeno> all the complications in describing this are because is a problem to find a efficient algo not because is such a complicate thing
03:23 < rszeno> is just a search for matching


03:59 < HMC_Alph> understanding yarrow/twelf/agda/idris stuffs should basically be "the goalpost" here ;-)


04:01 < HMC_Alph> rszeno: did you see that LogicT.pdf?
04:01 < HMC_Alph> http://okmij.org/ftp/papers/LogicT.pdf
04:02 < HMC_Alph> section 5.2 is particularly interesting wrt our coroutines. :-)


04:39 < HMC_Alph> stoopkid: i'm thinking about giving in and just using c++14 to implement the coro compositions (it doesn't look like it would take that much to implement this LogicT.pdf directly, heh)


05:08 < HMC_Alph> instead of calling a function that generates a coro lambda...
05:09 < HMC_Alph> make that function generate a lambda which returns the coro lambda
05:09 < HMC_Alph> so then when you need two of them, you're just calling that function twice to get the function that you call in the loops
05:09 < HMC_Alph> follow?
05:10 < stoopkid> partially
05:10 < HMC_Alph> that is all that the python foo example is doing under the hood
05:11 < HMC_Alph> foo() actually only gets called 6 times, one to get each generator
05:11 < HMC_Alph> the outer and the 5 inners
05:11 < HMC_Alph> that generator is the actual coroutine, which is the function the loop is calling
05:11 < HMC_Alph> python is just hiding that under syntax :-)
05:12 < HMC_Alph> *functions the loops are calling
05:12 < HMC_Alph> heh


05:24  * HMC_Alph nods
05:24 < HMC_Alph> foo() itself is not the coroutine
05:24 < HMC_Alph> does this make sense?
05:25 < HMC_Alph> and do you see why this makes {?x a :man} => {?x a :mortal} not an infloop? :-D
05:26 < stoopkid> indeed, i guess i was confused from python's hidden syntax; we've been going as if those were the coros themselves
05:27 < HMC_Alph> probably my fault for leaving it out of my clintons ;-)
05:27 < HMC_Alph> you can see how for that toy example it "doesn't apply" maybe

05:50 < stoopkid> so we aot-compile the kb into basically the structure we have now, but with coro-generators (rather than the coros themselves), and when the coro-generators are called, this performs what we're referring to as 'jit'?
05:55 < HMC_Alph> ah,well I wouldn't worry too much about the recompilation for now
05:55 < HMC_Alph> but yah, you are doing a full static compile to start
05:56 < HMC_Alph> later it will be an incremental compilation, with recompilations when blocks change definitions...
05:56 < HMC_Alph> but I wouldn't worry about any of that for now and just focus on "compilation in general" :-)


05:57 < HMC_Alph> when we actually do jit compiles is more to do with builtins and typesystem than the compiler transform itself


05:59 < HMC_Alph> for this "entirely arch unspecified" version of the backend we probably won't even go beyond just recompiling around predicates changed in blocks... we'll probably leave anything like compile-on-deman or hotpath optimizations to native code emitters ("outside of spec") :-)
05:59 < HMC_Alph> well
05:59 < HMC_Alph> local hotpath
06:00 < HMC_Alph> but that ties into that "other half of the jit" thing some :-)
06:01 < stoopkid> hehe well i certainly don't mind leaving it as simple as possible
06:03  * HMC_Alph nods


18:04 < HMC_Alph> stoopkid: it does have to be all of the coros, not just the "top level" ones
18:05 < HMC_Alph> well, maybe I misunderstand what you ask, I'd have to look at the code :-)
18:06 < HMC_Alph> but everything that yields needs a state for each distinct call site


19:57 < HMC_Alph> adam789654123: http://safecurves.cr.yp.to/  at least some are trying. ;-)



22:40 < HMC_Alph> We used to think that if we knew one, we knew two, because one and one are two. We are finding that we must learn a great deal more about 'and.'
22:40 < HMC_Alph> â<80><94>Sir Arthur Eddington

22:40 < HMC_Alph> 22:34  * adam789654123 wonders the distinction between "Martain Lof's Type Theory" and "Dependently Typed Lambda Calculus"
22:41 < HMC_Alph> cumulativity of universe hierarchy

22:55 < koo7> i wonder why the cli runs query from 3 places:))
22:57 < stoopkid> probably a combo of stoopkid's bad planning + stoopkid's bad C++

04:47 < HMC_Alph> tau will have distinct prover and verifier... this is mostly related to auth, though...
rszeno is correct in that without our auth model we could get away with only a prover (verify is then just another
proof over the proof)
04:49 < HMC_Alph> but we will have three such modes, actually.. as in lambda-auth there is the "peover' and the "verifier" and
the "identity"

05:20 < stoopkid> ah koo7 said there might be something buggy with the lists still

05:22 < HMC_Alph> sure, so maybe some simpler list tests to try to narrow that down?
05:22 < HMC_Alph> i'm going to look over the recursion for ep check

05:26 < stoopkid> yea a simpler 2 var cnf also doesn't work
05:26 -!- sifar [~sifar@unaffiliated/sifar] has quit [Ping timeout: 272 seconds]
05:41 < stoopkid> maybe since we're on example-making we should go over the DRTs stuff as well

05:56 < HMC_Alph> well the recursion problem isn't quite ep check...
05:57 < HMC_Alph> the recursion is in compile and ep check is at runtime
05:59 < HMC_Alph> ok, so let's start with simple recursion... like your causes example
06:01 < HMC_Alph> the function that gives you the coroutine should be called by those coroutine instances...

06:02 < stoopkid> hm true
06:02 < stoopkid> i thought we fixed that, guess not

06:02 < HMC_Alph> it is not quite a recursive lambda in the c++...
06:04 < HMC_Alph> it is an outer lambda with a self capture that returns an inner lambda which captures the
 captured outer lambda... and which calls it when called...
06:05 < HMC_Alph> the outer lambda is the python foo function itself, the unner lambda is the actual coro function that
python hides fron you
06:06 < HMC_Alph> those inner coro functions call *foo* (the outer) if they are recursive... to get a new coroutine instance...
they don't call their own coro re-entry point. ;-)
06:07 < HMC_Alph> follow?

06:07 < stoopkid> yea i follow that i'm just trying to figure out where in the C++ we're going wrong
06:08 < HMC_Alph> lol
06:09 < HMC_Alph> .g lambda over lambda
06:09 < tau-bot> HMC_Alph: https://chriskohlhepp.wordpress.com/lambda-over-lambda-in-cplusplus14/
06:10 < HMC_Alph> .g c++ recursive lambda capture
06:10 < tau-bot> HMC_Alph: http://stackoverflow.com/questions/7861506/recursive-call-in-lambda-c11
06:10 < HMC_Alph> meh, no
06:11 < HMC_Alph> .g c++ lambda recursion
06:11 < tau-bot> HMC_Alph: http://stackoverflow.com/questions/14531993/can-lambda-functions-be-recursive
06:11 < HMC_Alph> sure, that
06:11 < stoopkid> i mean where in our big tangled document of C++, not C++ in general :)
06:12 < HMC_Alph> heh


07:02 < stoopkid> there's a strong correlation between bugs getting solved and koo7 coming online

07:27 < koo7> stop breaking it, guys
07:27 < stoopkid> did i break it? :O
07:27 < koo7> we dont reuse compiled preds....i think we went over why
07:28 < HMC_Alph> ?
07:28 < stoopkid> pred() kept adding it to the predQueue during compile_preds that's why it was inflooping
07:29 < koo7> now, why unification...because when we brute force each pred compilation and compile all rules, we go into a loop on much more examples
07:29 < HMC_Alph> i don't follow
07:30 < koo7> we dont reuse compiled coros, and we dont copy them because of the Vars thing
07:30 < koo7> we compile a fresh "pred" for every use of it
07:32 < HMC_Alph> sure, but to compile recursion you *do* need to re-use a closure
07:32 < koo7> what closure?
07:32 < stoopkid> i think its rather, the first time the pred is used, it gets compiled and then this compiled pred-function is wrapped in a generator function, so that on each use of the pred, this generator returns a fresh instance of our compiled function
07:33 < koo7> stoopkid, no, that was your version, but it didnt work because of Vars
07:33 < HMC_Alph> you need to be able to "call foo" at runtime to get the coroutine functions at runtime
07:34 < HMC_Alph> right now you just have "a lambda that is one coroutine" at runtime
07:34 < koo7> its all in function pred and compile_preds
07:35 < koo7> at runtime you call a lambda returned by pred
07:35 < stoopkid> this would be a lot easier if we were all on azure :)
07:35 < koo7> yes
07:35 < HMC_Alph> you need a lambda that returns this lambda at runtime...
07:35 < koo7> and what will i do with it?
07:35 < HMC_Alph> let me put together an example of what I mean...
07:37 < HMC_Alph> i'll make that foo example in c++


07:47 < stoopkid> ill try not to break your code too much more
07:47 < koo7> :))
07:48 < HMC_Alph> yes, order matters in the kb
07:49 < koo7> of rules?
07:49  * HMC_Alph nods
07:49 < koo7> so even order of matches is defined?
07:49 < HMC_Alph> basically, yes.
07:49 < stoopkid> maybe we should go back to aot compile?
07:50 < koo7> so you were right after all stoopkid
07:50 < stoopkid> about what
07:50 < koo7> that order of rules in kb matters
07:50 < HMC_Alph> very much so, heh
07:51 < koo7> stoopkid, go ahead, then you will see why i changed it to "jit"
07:51 < stoopkid> the order only matters for our particular 'standardization' though, right?
07:52 < stoopkid> koo7: i cant think of why we'd need to
07:53 < koo7> well, there wasnt the compiletime unification check back then, so it just compiled everything twice
07:54 < koo7> i guess that was when we still reused pred coros
07:55 < stoopkid> mm
07:56 < stoopkid> the issue is we need to create new instances of the vars each time to capture into our lambdas?
07:56 < koo7> sounds about right
07:57 < koo7> this is why we cant simply copy the compiled coros like you did
07:57 < stoopkid> lets make a var into a var generator like we make our preds into pred generators then
07:57 < koo7> yeah? and make the different coros use the same generated var how?
07:58 < stoopkid> keep track of vars across a query?
08:00 < stoopkid> maybe not? not sure, HMC_Alph any thoughts?
08:00 < koo7> maybe something maybe with some indexes could be conjured up
08:01 < HMC_Alph> http://pastebin.com/NaS8bKLr
08:01 < koo7> but i dont believe in the motivation for this, its just to avoid the compile time ep check because hmc doesnt
understand why it must be there
08:02 < stoopkid> i'm not sure why it must be there
08:02 < HMC_Alph> ep check cant be done at compile time...
08:04 < stoopkid> capturing a lambda into itself on definition is interesting i don't even think that's the issue here though
08:04 < stoopkid> lookup accomplishes the same with indirection
08:06 < stoopkid> koo7: compiling shouldn't infloop because it's basically just replacing the preds in our finite kb with
functions
08:06 < HMC_Alph> no, not quite the same
08:06 < stoopkid> or, function calls, whichever
08:06 < stoopkid> or
08:06 < stoopkid> maybe not
08:06 < koo7> its not, that would mean lookups everywhere
08:08 < koo7> HMC_Alph, this doesnt solve the variables thing
08:09 < koo7> or....i dunno
08:10 < stoopkid> lemme think about it, i think i'm gonna try to work it up in a fresh document
08:11 < koo7> stoopkid, what was your original problem, recursive examples infiloop?
08:12 < koo7> well, the unification is a hack that doesnt catch all cases, just some. it also seems like an optimization
08:13 < koo7> i hoped it would let us progress and a full solution would be the ep check
08:16 < HMC_Alph> ep check is for runtime loops...
08:16 < koo7> HMC_Alph, *something like* ep check that would catch the compile time recursion?
08:17 < stoopkid> i still don't see where we should be getting compile time recursion
08:18 < koo7> because each invocation of a pred in a rule body requires that a fresh coro for that pred be compiled
08:19 < koo7> we cant copy because of vars
08:19 < HMC_Alph> no coros should actually exist until runtime...
08:19 < koo7> well, that was the current state of code
08:21 < koo7> maybe you should go over that in a bit more detail
08:21 < stoopkid> the pred itself should be packaged into a generator, and each invocation of a pred in a rule body should be a
call to the generator to get a fresh instance of the coro at runtime

08:57 < koo7> ok i see if we compile strictly on demand, that ep check will avoid for us the infiloop

09:19 < HMC_Alph> it isn't so much compiling on demand as creating tge xoroutine instances on demand
09:20 < HMC_Alph> you can still compile everything up front
09:20 < HMC_Alph> *the coroutine

10:08 < koo7> HMC_Alph, we cant compile everything up front, we dont know how many instances of each pred coro we need
10:09 < koo7> and i think youre wrong that theres just one lambda coro, theres a lambda that looks up a coro
16:09 < HMC_Alph> this is a slightly simplified explanation, but hopefully gets the basic point across? :-)

16:21 < HMC_Alph> http://people.cs.kuleuven.be/~jon.sneyers/papers/toplas-complexity.pdf

16:28 < koo7> yeah that sounds simple, at least a non-optimized way of doing that
16:29 < koo7> we have more problems, for one our atom() function must participate in the per-rule varmap
16:29 < koo7> but more importantly we apparently have to wrap preds in yet another lambda
16:33 < HMC_Alph> not sure what you mean wrt atom()
16:34 < HMC_Alph> but yah, you do still need to do that lambda-over-lambda thing to get multiple instances of the same coro
working correctly... ;-)

16:04 < HMC_Alph> anyway the ep check is not very hard... what you basically want to do is to keep a "memo set" per predicate... when each predicate is first invoked, it should check it's set to see if its argument pair is already there...
16:04 < HMC_Alph> if so, it should "fail" immediately and not do any loop
16:05 < HMC_Alph> if not, it should add it's argument value to the set and then continue as normal.
16:05 < HMC_Alph> and that's it, that is all it takes.
16:06 < HMC_Alph> ofc when the predicate is finished with it's looping and is about to hit it's "normal fail" (done w/ it's looping) it should remove the values it was started with from the set again
16:07 < HMC_Alph> so basically each predicate call will first check to see if this is an unproductive "duplicate" call (if it is returned to a point in the search that it is "already currently searching"... meaning it has found a loop in the search)
16:08 < HMC_Alph> and will bail out without "re-matching" what it is already searching, instead of going around the loop again


19:19 < HMC_Alph> koo7: do you understand how the proof tracing would work?
19:20 < koo7> i understood what you said, but im focusing on lambdas now
19:20  * HMC_Alph nods
19:21 < HMC_Alph> all three concerns are rather closely related :-)
19:21 < HMC_Alph> you can think of the ep check as looking at the state of the next step of the proof and saying "is this state already in our proof anyway?" and bailing out on the infinitely long proof subtree. ;-)
19:23 < HMC_Alph> and the state of the proof is basically just a trace log of "what coro lambdas went where (in the search space) and why, during inference so far"


19:27 < HMC_Alph> naturalog: i mean that you keep building the state machine... instead of letting the cpu be the state machine... ;-)


19:32 < HMC_Alph> the only actual pointer is at the join variables, and these are just bindings that are *written* to and branched from (directly) at runtime, not walked iteratively except perhaps in the *caller* to the query...
19:32 < HMC_Alph> 19:35 < naturalog> so it boils down to "calculus" of appending vars/nonvars to lines of the form t=?x=?z.... <-- yes this
19:32 < HMC_Alph> but an algebra, not a calculus
19:32 < naturalog> right, not even monoid
19:32 < HMC_Alph> ah, is a monoid
19:32 < HMC_Alph> (!)
19:33 < naturalog> no id elem
19:33 < HMC_Alph> is a monoid, monad, even an arrow
19:33 < HMC_Alph> monadplus, even, yes
19:33 < HMC_Alph> it has a zero, called "fail"
19:33 < HMC_Alph> where coros stop
19:33 < naturalog> (there is no identity element so its not a monoid)
19:33 < HMC_Alph> it *has* an id
19:34 < naturalog> element that equal to everyone? which?
19:34 < HMC_Alph> (function extensionality proves it, ofc)
19:34 < naturalog> well the structure of equalities in that vm isnt monoid, ofc the whole processcan be modeled as monoids
19:35 < HMC_Alph> backtracking inference is a monadplus (coroutines or cps or your tabling, take your pick)
19:36 < HMC_Alph> this is not someting I quite fully realized going into tau
19:36 < HMC_Alph> 19:39 < naturalog> yes it must be
19:36 < HMC_Alph> yes
19:36 < HMC_Alph> ultimately even stoopkid "saw it" before me, I now realize ;-)
19:37 < HMC_Alph> in retrospect it is almost "obvious" enough to be 'astonishing'... kleisli arrows and all

20:22 < HMC_Alph> i'm trying to convince you that the system does demand a particular performance *profile* for some concerns
of general operation, and that I don't think your solution will scale into a specific performance curve that will suit (m)any
users of it! XD

16:49 < HMC_Alph> "generator" is just a name for the particular coroutines we use, specifically that being an asymmetric semicoroutine
16:50 < HMC_Alph> but yes, what you are actually compiling each predicate to is a function which returns ("instantiates") this coroutine function
16:50 < HMC_Alph> so if you call the same compiled predicate N times, you now have N identical live generators in hand
16:51 < HMC_Alph> so the "compiled predicate" is like an initial entrypoint that gives a fresh continuation each time

16:52 < HMC_Alph> join/seq are combinators
16:53 < HMC_Alph> they take some compiled predicates and give you a new compiled predicate
16:54 < HMC_Alph> when you call this new compiled predicate, it returns a coroutine... as you call this coroutine it calls the compiles predicates you gave it to get the "inner" coroutines that are composed over, and iterates them


21:14 < koo7> social scope: a bunch of unemployed misfits led by a crazed fractal junkie
21:16  * HMC_Alph snorts some crazed fractals

< koo7> variable unification should be a lambda over lambda too?
< HMC_Alph> you mean like the coro from UnifyingVariable.unify()?
< HMC_Alph> koo7: yes, all of the coro should work like python's afaict :-)
03:19 < koo7> HMC_Alph, no
03:20 < koo7> one more, like pred
03:37 < HMC_Alph> koo7: all of the coroutines including the one that yields up the unifyingvariable binding,
need to have this "hidden python function" outer lambda, yes

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
^cps?

When using ld, include the following command line option:
-Bsymbolic
If using gcc to build a library, add this option to the command-line:
-Wl,-Bsymbolic
https://docs.oracle.com/cd/E19957-01/806-0641/chapter4-16/index.html
https://docs.oracle.com/cd/E19957-01/806-0641/6j9vuquim/index.html#chapter2-14824


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

	/*lets task a second thread with copying the template?
	 * LocalsQueue &locals_queue = *new LocalsQueue();
	locals_queues.push_back(&locals_template);
	 http://www.drdobbs.com/parallel/writing-lock-free-code-a-corrected-queue/210604448
	 http://www.boost.org/doc/libs/1_39_0/libs/pool/doc/index.html

	 http://man7.org/linux/man-pages/man3/alloca.3.html
	 nope, we cant have locals on stack
	 	 
	http://en.cppreference.com/w/cpp/thread/future
	https://gist.github.com/rmartinho/5231565
	 */



gcc  -O3 -Q --help=optimizers













Counting bits set, Brian Kernighan's way

unsigned int v; // count the number of bits set in v
unsigned int c; // c accumulates the total bits set in v
for (c = 0; v; c++)
{
  v &= v - 1; // clear the least significant bit set
}



 Counting bits set in 14, 24, or 32-bit words using 64-bit instructions

unsigned int v; // count the number of bits set in v
unsigned int c; // c accumulates the total bits set in v

// option 1, for at most 14-bit values in v:
c = (v * 0x200040008001ULL & 0x111111111111111ULL) % 0xf;



 A generalization of the best bit counting method to integers of bit-widths upto 128 (parameterized by type T) is this:

v = v - ((v >> 1) & (T)~(T)0/3);                           // temp
v = (v & (T)~(T)0/15*3) + ((v >> 2) & (T)~(T)0/15*3);      // temp
v = (v + (v >> 4)) & (T)~(T)0/255*15;                      // temp
c = (T)(v * ((T)~(T)0/255)) >> (sizeof(T) - 1) * CHAR_BIT; // count



 Swapping values with XOR

#define SWAP(a, b) (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))

This is an old trick to exchange the values of the variables a and b without using extra space for a temporary variable.

On January 20, 2005, Iain A. Fleming pointed out that the macro above doesn't work when you swap with the same memory location, such as SWAP(a[i], a[j]) with i == j. So if that may occur, consider defining the macro as (((a) == (b)) || (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))). On July 14, 2009, Hallvard Furuseth suggested that on some machines, (((a) ^ (b)) && ((b) ^= (a) ^= (b), (a) ^= (b))) might be faster, since the (a) ^ (b) expression is reused.




 Reverse the bits in a byte with 4 operations (64-bit multiply, no division):

unsigned char b; // reverse this byte

b = ((b * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;




#endif