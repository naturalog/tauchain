//Euler-path check

//Using Boost asymmetric coroutines
*Uses pull_type and push_type rather than yield



//Need to reimplement these functions to the extent
//that switching to Boost asymmetric coroutines affects them.

//Unifying variables


//General unifier


//Knowledge-base
*Is a map of preds->functions


//Construct the knowledge-base.
//Should be starting at the bottom and working towards the top,
//as we will be executing starting from the top and working towards
//the bottom. Likewise we'll construct from right-to-left in a rule
//because it will be executed from left-to-right.

//Loop over each quad in the KB.
	//Ordering of operations
	//
	//Each fact and rule (via the rule-head) is associated to
	//a predicate, as facts and rule-heads are spo.

	Variant 1. Construct all preds in the pred-map before compiling
	Variant 2. Construct preds in the pred-map as we compile.

	I.
	-----------------------------
	* Look at the last quad
	* Get its predicate
	* Compile the quad as either fact or rule, depending.
	* A rule may call other rules, so, the combinators may need
	  to use a lookup though i'm not sure what we may gain from
 	  using asymmetric coroutines.
	
		
	//1. Fact or rule
        //Fact: compile a fact.
        *Unifies the subject and object arguments with
        it's captured subject and object, respectively.
	
	//How are rules structured in the quads exactly?

        //Rule: compile a rule.
        //No terms in the body:
        * Compile as..

        //One term in the body:
        * Compile as clause

        //Two terms in the body:
        * Compile as join(clause,clause)

        //More than two terms in the body
        * Compile as halfjoin(clause,join)
	

        //Start with the last two terms in the body
        //and work towards the first


	//Construct a function P for the predicate.
        //P goes into a map of preds->functions
        //


        //Sequence preds starting from the bottom
        //

        //**Lists**


	--> into A.


	II.
	=======================================	
	
	* Look at the 2nd to last quad
	* ....

	--> into B.



	III.
	========================================
	* sequence (B, A) into PredsTable->pred.





	IV.
	========================================
	* for each remaining quad for this pred,
		Look at that quad
		...
		-->into i
		sequence(i,PredsTable->Pred) into PredsTable->Pred
	=========================================

//Query
*

