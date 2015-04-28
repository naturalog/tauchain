

/*
  The main inference loop.  Includes standard back-chaining reasoning (Robertson) plus a "euler path detection"
  anti-looping check. (Runtime productivity check, ensures termination.)
  Inputs:
    goal - a rule where the body is the query to be matched
    maxNumberOfSteps - limits count of transitivities calculated, allows for an early bail-out.  -1 to disable
  Returns:
    undefined ("false") if the reasoner ran to completion
    empty string ("true") if the reasoner terminated early by reaching maxNumberOfSteps
  Outputs:
    Leaves a trace of the resolution path in the "evidence" array
*/
function prove(goal, maxNumberOfSteps) {
    /*The goal queue, a list of goals yet to be dispatched.
      Structured using "evidence frames" which relate a goal rule to be matched with the already-matched rule
      that prompted it as a goal.  Also stores the active environment and ground state at the time that
      the frame was created, for evidentiary tracking, to establish the proof trace in evidence. */
    //rule: rule to be matched (body)
    //src: either an integer reference to a case for the head predicate, or a reference to the rule, from which this frame was created.  src=0 indicates the root goal (for euler path)
    //ind: index of the statement within the rule body that is "next" to be matched (for matching and euler path)
    //parent: a reference to the parent frame that this frame was derived from (for euler path)
    //env: the active environment bindings for this frame
    //ground: the active ground state for this frame
    /* You can think of this queue as the "call stack" for the interpreter.  When a new rule is decended into, a new frame is added.  When a rule is fully matched and "returns" to
       the parent rule that induced it a frame is popped (and moved to evidence if it was matched) */
    var queue = [ {rule:goal, src:0, ind:0, parent:null, env:{}, ground:[]}];
    //Initialize a local proof trace if there is none defined globally
    if (typeof(evidence) == 'undefined') evidence = {};
    //Initialize a local step counter if there is none defined globally
    if (typeof(step) == 'undefined') step = 0;

    /*Iterate as long as we continue to have a goal which needs to be matched
      This is the main inference loop.  It will take the top goal from the queue and attempt to match it.
      If the goal is already entirely matched (or is empty) then the goal is met, and the frame is moved to evidence.
      If the goal is matched against a rule head, the statements of the rule's body will be added to queue.
    */
    while (queue.length > 0) {
        //Take the top of queue
        var c = queue.pop();
        if (typeof(trace) != 'undefined') document.writeln('POP QUEUE\n' + JSON.stringify(c.rule) + '\n');
        //duplicate its ground state
        var g = aCopy(c.ground);
        //increase step counter for bailout
        step++;
        //check bailout, return true if we're over step count max
        if (maxNumberOfSteps != -1 && step >= maxNumberOfSteps) return '';
        //check if this frame still has any statements to be matched.
        if (c.ind >= c.rule.body.length) {
            //if it does not, check if the parent is null indicating root goal
            if (c.parent == null) {
                //this is a root goal, iterate over the final environment and copy our evidence frame information into evidence list
                for (var i = 0; i < c.rule.body.length; i++) {
                    //evaluate the statement in the enviornment
                    var t = evaluate(c.rule.body[i], c.env);
                    //initialize this predicate's evidence list, if necessary
                    if (typeof(evidence[t.pred]) == 'undefined') evidence[t.pred] = [];
                    //add the evidence for this statement
                    evidence[t.pred].push({head:t, body:[{pred:'GND', args:c.ground}]});
                }
                //this rule is done, leave it popped from queue and continue on with the next queue entry
                continue;
            }
            //parent is not null, so we continue at this depth, adding this matched rule to ground state
            if (c.rule.body.length != 0) g.push({src:c.rule, env:c.env});
            //construct a new frame at the next index of the parent rule, with a copy of the evidence state
            var r = {rule:
                     {head:c.parent.rule.head, body:c.parent.rule.body}, src:
                     c.parent.src, ind:
                     c.parent.ind,
                     parent:
                     c.parent.parent != null ? new copy(c.parent.parent) : null, env:new copy(c.parent.env), ground:g
                    };
            //unify this new frame with our current environment
            unify(c.rule.head, c.env, r.rule.body[r.ind], r.env, true);
            //advance to the next statement in the parent frame
            r.ind++;
            //add this new "next statement of parent" frame back into queue
            queue.push(r);
            if (typeof(trace) != 'undefined') document.writeln('PUSH QUEUE\n' + JSON.stringify(r.rule) + '\n');
            //and iterate, which will cause it to be popped right back off at the top of the loop!
            continue;
        }
        //This current frame still has statements to be matched, so try to match them, pushing new subgoals if necessary
        //get the statement 't' for "this statement"
        var t = c.rule.body[c.ind];
        //check to see if this statement can be matched as a builtin predicate
        var b = builtin(t, c);
        if (b == 1) {
            //it can and did!
            //Add it as a matched statement to our ground state
            g.push({src:{head:evaluate(t, c.env), body:[]}, env:{}});
            //push a new evidenc frame for the parent rule
            var r = {rule:{head:c.rule.head, body:c.rule.body}, src:c.src, ind:c.ind, parent:c.parent, env:c.env, ground:g};
            //advance the frame to the next statement in the parent rule
            r.ind++;
            //push the parent rule back on to queue to continue with it
            queue.push(r);
            if (typeof(trace) != 'undefined') document.writeln('PUSH QUEUE\n' + JSON.stringify(r.rule) + '\n');
            //and iterate, again causing this new frame to be popped right back off at the top of the loop!
            continue;
        }
        else if (b == 0) continue; //this indicates that it was a builtin but failed to match, so we just continue on

        //check if we have any cases to match against.  If not, just continue on.
        if (cases[t.pred] == null) continue;

        //We do have some cases to attempt to match this frame against!
        var src = 0; //initialize our index counter
        //loop over each potential case and add subgoals as necessary
        for (var k = 0; k < cases[t.pred].length; k++) {
            //fetch the case 'rl' for "rule"
            var rl = cases[t.pred][k];
            //increment index into the cases for this predicate
            src++;
            //copy ground state for new frame
            var g = aCopy(c.ground);
            //if this rule is a *fact* then we add it to ground state
            if (rl.body.length == 0) g.push({src:rl, env:{}});
            //Set up this rule as a potential new frame
            var r = {rule:rl, src:src, ind:0, parent:c, env:{}, ground:g};
            //check if this rule matches the current subgoal
            if (unify(t, c.env, rl.head, r.env, true)) {
                //it does!  Check it for a euler path...
                var ep = c;  // euler path
                //Walk up the evidence tree and see if we find a frame already in the tree that is the same as this "potential next frame"
                //If we do find that this "next" frame was already seen this indicates that we are about to enter a proof state that we have already been in
                //This would create a non-productive loop, so if we do encounter such a case we simply fail out of the match and continue on
                while (ep = ep.parent) if (ep.src == c.src && unify(ep.rule.head, ep.env, c.rule.head, c.env, false)) break;
                //Check if we iterated up to the root goal.  If so, then we know this next frame is a *new* proof state that we have not encountered before
                //and that we should continue resolution trying to match this frame as a new subgoal!
                if (ep == null) {
                    //Add our new frame to queue
                    queue.unshift(r);
                    if (typeof(trace) != 'undefined') document.writeln('EULER PATH UNSHIFT QUEUE\n' + JSON.stringify(r.rule) + '\n');
                }
            } //end specific subgoal
        } //end iteration of subgoals
    } //end main loop
    //if we reach here, then we have completed iteration over the entire query.  Anything we matched is not moved over to evidence, with its proof trace.
}


/* The unifier.  Attempts to match two statements in corresponding environments.  Updates one environment with information from the other if they do match.
   Inputs:
    s - source statement
    senv - source environment
    d - destination statement
    denv - destination environment.  Updated with variable bindings if s does unify with d.
   Returns: true if unification happened, false otherwise
   Outputs: updates denv
*/
function unify(s, senv, d, denv, f) {
    if (typeof(trace) != 'undefined') document.writeln('UNIFY\n' + JSON.stringify(s) + '\nWITH\n' + JSON.stringify(d) + '\n');
    //If source predicate is a variable
    if (isVar(s.pred)) {
        //evaluate it in the environment
        var sval = evaluate(s, senv);
        //if it did evaluate, unify with the value
        if (sval != null) return unify(sval, senv, d, denv, f);
        //otherwise assume match
        else return true;
    }
    //If the desination predicate is a variable
    else if (isVar(d.pred)) {
        //evalaute it in the environment
        var dval = evaluate(d, denv);
        //if it did evaluate, unify with the value
        if (dval != null) return unify(s, senv, dval, denv, f);
        //otherwise
        else {
            //if we have the override flag, update the destination predicate with its value from source
            //note: currently unused
            if (f != null) denv[d.pred] = evaluate(s, senv);
            return true;
        }
    }
    //otherwise, check for matching predicates
    else if (s.pred == d.pred && s.args.length == d.args.length) {
        //they match, check each argument and make sure they also unify, fail out if not
        for (var i = 0; i < s.args.length; i++) if (!unify(s.args[i], senv, d.args[i], denv, f)) return false;
        //everything matched!  These statements unify and denv is updated.
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
     env - environment of variable bindings
   Returns: The term with variables replaced, if they were found in the environment.  Null otherwise.
   Outputs: none
*/
function evaluate(t, env) {
    //If the predicate is a variable
    if (isVar(t.pred)) {
        //look up the variable in environment
        var a = env[t.pred];
        //if the variable has a value, re-evaluate with the predicate variable replaced
        if (a != null) return evaluate(a, env);
        //otherwise fail
        else return null;
    }
    //The predicate is not a variable and there are no parameters to it, so we are done, return the value
    else if (t.args.length == 0) return t;
    //the predicate is not a variable but there are some parameter terms.  Evalaute each.
    else {
        //Placeholder for evaluated arguments
        var n = [];
        //loop over arguments
        for (var i = 0; i < t.args.length; i++) {
            //evaluate this argument
            var a = evaluate(t.args[i], env);
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
    for (var i = 0; i < t.length; i++) a[i] = t[i];
    return a;
}
//Helper: duplicate an object
function copy(t) {
    for (var i in t) this[i] = t[i];
}
