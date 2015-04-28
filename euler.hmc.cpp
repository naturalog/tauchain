#include <forward_list>
#include <deque>
#include <climits>
#include <iostream>
#include <sstream>
#include <string>

const uint M1 = 1024 * 1024;
const uint max_predicates = M1, max_rules = M1;
const uint GND = INT_MAX;

typedef forward_list<class predicate*> predlist;
ostream& operator<<(ostream&, const predlist&);

struct predicate {
	int p = 0;
	predlist args;
	predicate& init(int _p = 0, predlist _args = predlist()) {
		p = _p;
		args = _args;
		return *this;
	}
	bool& null = (bool&)p;
	ostream& operator<<(ostream&) { return o << dict[p] << '(' << args << ')'; }
} *predicates = new predicate[max_predicates];

struct rule {
	predicate* head = 0;
	predlist body;
	rule& init(predicate* h, predlist b = predlist()) {
		head = h;
		body = b;
		return this;
	}
	ostream& operator<<(ostream&) { return o << *head << '(' << args << ')'; }
} *rules = new rule[max_rules];

uint npredicates = 0, nrules = 0;

typedef map<int, predicate*> subst;
typedef forward_list<pair<rule*, subst>> gnd;
typedef map<int, forward_list<rule*>> evd;

ostream& operator<<(ostream& o, const subst& s) { for (auto x : s) cout << x.first << ':' << x.second << endl; return o; }
ostream& operator<<(ostream& o, const gnd& s) { for (auto x : s) cout << x.first << ':' << x.second << endl; return o; }

struct frame {
	rule* r = 0;
	uint src = 0, ind = 0;
	frame* parent = 0;
	subst s;
	gnd g;
	frame(rule* _r = 0, uint _src = 0, uint _ind = 0, frame* p = 0, subst _s = subst(), gnd _g = gnd()) :
		r(_r), src(_src), ind(_ind), parent(p), s(_s), g(_g) {}
	ostream& operator<<(ostream&) { return o << "rule: " << *r << " src: " << src << " ind: " << ind << " parent: " << *parent << " subst: " << s << " gnd: " << g; }
};

ostream& operator<<(ostream& o, const predlist& l) {
	for (predlist::iterator it;;) {
		o << *it;
		if (++it == l.end()) break;
		else o << ',';
	}
	return o;
}

evd prove(rule* goal, int maxNumberOfSteps) {
    deque<frame> queue;
    queue.emplace_back(goal);
    evd e;
    uint step = 0;

    while (queue.size() > 0) {
        frame c = queue.front();
	queue.pop_front();
        gnd g(c.g);
        step++;
        if (maxNumberOfSteps != -1 && step >= maxNumberOfSteps) return evd();
        if (c.ind == c.rule.body.size()) {
            if (!c.parent) {
                for (size_t i = 0; i < c.rule.body.size(); ++i) {
                    var t = evaluate(c.rule.body[i], c.sub); //evaluate the statement in the enviornment
		    if (e.find(t.p) == e.end()) e[t.p] = {};//initialize this predicate's evidence list, if necessary
                    e[t.p].emplace_front(rules[nrules++].init(t, {predicates[npredicates++].init(GND, to_predlist(c.ground))}));//add the evidence for this statement
                }
                //this rule is done, leave it popped from queue and continue on with the next queue entry
                continue;
            }
            //parent is not null, so we continue at this depth, adding this matched rule to ground state
            if (!c.rule.body.empty()) g.emplace_front(c.rule, c.sub);
            //construct a new frame at the next index of the parent rule, with a copy of the evidence state
            frame r(
	      	     rules[nrules++].init(c.parent->rule.head, c.parent->rule.body),
                     c.parent.src, 
                     c.parent.ind,
                     c.parent.parent, 
		     subst(c.parent.sub), 
		     g
                    );
            //unify this new frame with our current environment
            unify(c.rule.head, c.sub, r.rule.body[r.ind], r.sub, true);
            //advance to the next statement in the parent frame
            r.ind++;
            //add this new "next statement of parent" frame back into queue
            queue.push_back(r);
            //and iterate, which will cause it to be popped right back off at the top of the loop!
            continue;
        }
        //This current frame still has statements to be matched, so try to match them, pushing new subgoals if necessary
        //get the statement 't' for "this statement"
        predicate* t = c.rule.body[c.ind];
        //check to see if this statement can be matched as a builtin predicate
        int b = builtin(t, c);
        if (b == 1) {
            //it can and did!
            //Add it as a matched statement to our ground state
            g.emplace_front(rules[nrules++].init(evaluate(t, c.sub)), subst());
            //push a new evidenc frame for the parent rule
            frame r = c;
	    r.g = gnd();
            //advance the frame to the next statement in the parent rule
            r.ind++;
            //push the parent rule back on to queue to continue with it
            queue.push_back(r);
//            if (typeof(trace) != 'undefined') document.writeln('PUSH QUEUE\n' + JSON.stringify(r.rule) + '\n');
            //and iterate, again causing this new frame to be popped right back off at the top of the loop!
            continue;
        }
        else if (b == 0) continue; //this indicates that it was a builtin but failed to match, so we just continue on

        //check if we have any cases to match against.  If not, just continue on.
        if (cases.find(t.p) == cases.end()) continue;

        //We do have some cases to attempt to match this frame against!
        uint src = 0; //initialize our index counter
        //loop over each potential case and add subgoals as necessary
        for (var k = 0; k < cases[t.pred].size(); k++) {
            //fetch the case 'rl' for "rule"
            var rl = cases[t.pred][k];
            //increment index into the cases for this predicate
            src++;
            //copy ground state for new frame
            var g = aCopy(c.ground);
            //if this rule is a *fact* then we add it to ground state
            if (rl.body.size() == 0) g.push({src:rl, env:{}});
            //Set up this rule as a potential new frame
            var r = {rule:rl, src:src, ind:0, parent:c, env:{}, ground:g};
            //check if this rule matches the current subgoal
            if (unify(t, c.sub, rl.head, r.sub, true)) {
                //it does!  Check it for a euler path...
                var ep = c;  // euler path
                //Walk up the evidence tree and see if we find a frame already in the tree that is the same as this "potential next frame"
                //If we do find that this "next" frame was already seen this indicates that we are about to enter a proof state that we have already been in
                //This would create a non-productive loop, so if we do encounter such a case we simply fail out of the match and continue on
                while (ep = ep.parent) if (ep.src == c.src && unify(ep.rule.head, ep.sub, c.rule.head, c.sub, false)) break;
                //Check if we iterated up to the root goal.  If so, then we know this next frame is a *new* proof state that we have not encountered before
                //and that we should continue resolution trying to match this frame as a new subgoal!
                if (ep == null) {
                    //Add our new frame to queue
                    queue.unshift(r);
//                    if (typeof(trace) != 'undefined') document.writeln('EULER PATH UNSHIFT QUEUE\n' + JSON.stringify(r.rule) + '\n');
                }
            } //end specific subgoal
        } //end iteration of subgoals
    } //end main loop
    //if we reach here, then we have completed iteration over the entire query.  Anything we matched is not moved over to evidence, with its proof trace.
}


/* The unifier.  Attempts to match two statements in corresponding environments.  Updates one environment with information from the other if they do match.
   Inputs:
    s - source statement
    ssub - source environment
    d - destination statement
    dsub - destination environment.  Updated with variable bindings if s does unify with d.
   Returns: true if unification happened, false otherwise
   Outputs: updates dsub
*/
bool unify(const predicate& s, const subst& ssub, const predicate& d, subst& dsub, bool f) {
//    if (typeof(trace) != 'undefined') document.writeln('UNIFY\n' + JSON.stringify(s) + '\nWITH\n' + JSON.stringify(d) + '\n');
    //If source predicate is a variable
    if (isVar(s.pred)) {
        //evaluate it in the environment
        var sval = evaluate(s, ssub);
        //if it did evaluate, unify with the value
        if (sval != null) return unify(sval, ssub, d, dsub, f);
        //otherwise assume match
        else return true;
    }
    //If the desination predicate is a variable
    else if (isVar(d.pred)) {
        //evalaute it in the environment
        var dval = evaluate(d, dsub);
        //if it did evaluate, unify with the value
        if (dval != null) return unify(s, ssub, dval, dsub, f);
        //otherwise
        else {
            //if we have the override flag, update the destination predicate with its value from source
            //note: currently unused
            if (f != null) dsub[d.pred] = evaluate(s, ssub);
            return true;
        }
    }
    //otherwise, check for matching predicates
    else if (s.pred == d.pred && s.args.size() == d.args.size()) {
        //they match, check each argument and make sure they also unify, fail out if not
        for (var i = 0; i < s.args.size(); i++) if (!unify(s.args[i], ssub, d.args[i], dsub, f)) return false;
        //everything matched!  These statements unify and dsub is updated.
        return true;
    }
    //otherwise we fail unification
    else {
        if (typeof(trace) != 'undefined') document.writeln('FAILED TO UNIFY\n' + JSON.stringify(s) + '\nWITH\n' + JSON.stringify(d) + '\n');
        return false;
    }
}

/* Environment evaluator.  Determines the value of an expression within an environment.  If the expression is a term we just return the term.
   Inputs:
     t - term to evaluate
     sub - environment of variable bindings
   Returns: The term with variables replaced, if they were found in the environment.  Null otherwise.
   Outputs: none
*/
predicate* evaluate(const predicate& t, const subst& sub) {
    //If the predicate is a variable
    if (isVar(t.pred)) {
        //look up the variable in environment
        var a = sub[t.pred];
        //if the variable has a value, re-evaluate with the predicate variable replaced
        if (a != null) return evaluate(a, sub);
        //otherwise fail
        else return null;
    }
    //The predicate is not a variable and there are no parameters to it, so we are done, return the value
    else if (t.args.size() == 0) return t;
    //the predicate is not a variable but there are some parameter terms.  Evalaute each.
    else {
        //Placeholder for evaluated arguments
        var n = [];
        //loop over arguments
        for (var i = 0; i < t.args.size(); i++) {
            //evaluate this argument
            var a = evaluate(t.args[i], sub);
            if (a != null) n.push(a); //we were able to evaluate, so include the value of the argument
            else n.push({pred:t.args[i].pred, args:[]}); //we were not able to evaluate, so leave it as a variable predicate
        }
        //return the term with variables all replaced
        return {pred:t.pred, args:n};
    }
    //never reach here
}

//Helper: checks if a term is a variable (if it begins with '?'.  Crappy.)
function isVar(s) {
    return s.charAt(0) == '?';
}
//Helper: duplicate an array
function aCopy(t) {
    var a = new Array();
    for (var i = 0; i < t.size(); i++) a[i] = t[i];
    return a;
}
//Helper: duplicate an object
function copy(t) {
    for (var i in t) this[i] = t[i];
}
