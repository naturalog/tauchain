#include <string>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <deque>
using namespace std;

struct pred_t {
	string pred;
	vector<pred_t> args;
};

typedef map<string, pred_t> env_t;

struct rule_t {
	pred_t head;
	vector<pred_t> body;
};

struct ground_t {
	rule_t src;
	env_t env;
};
typedef vector<ground_t> grounds;

struct proof_trace_item {
	rule_t rule;
	int src, ind;
	proof_trace_item* parent;
	env_t env;
	grounds ground;
};

int builtin ( pred_t, proof_trace_item ) {
/*  if (t.pred == "GND") return 1;
  pred_t t0 = evaluate(t.args[0], c.env), t1 = evaluate(t.args[1], c.env);
  if (t.pred == "log:equalTo") {
    if (t0 != null && t1 != null && t0.pred == t1.pred) return 1;
    else return 0;
  }
  else if (t.pred == "log:notEqualTo") {
    if (t0 != null && t1 != null && t0.pred != t1.pred) return 1;
    else return 0;
  }
  else if (t.pred == "log:semantics") {
    if (typeof(document) == "undefined") {
      defineClass("euler.Support");
      eval("var s = " + new Support().fromWeb(t0.pred));
    }
    else {
      var r = window.XMLHttpRequest?new XMLHttpRequest():new ActiveXObject("Msxml2.XMLHTTP");
      r.open("get", t0.pred, false);
      r.send(null);
      if (r.status == 200) eval("var s = " + r.responseText);
    }
    if (unify(s, c.env, t.args[1], c.env, true)) return 1;
    else return 0;
  }
  else if (t.pred == "math:absoluteValue") {
    if (t0 != null && !isVar(t0.pred)) {
      var a = Math.abs(parseFloat(t0.pred));
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    }
    else return 0;
  }
  else if (t.pred == "math:cos") {
    if (t0 != null && !isVar(t0.pred)) {
      var a = Math.cos(parseFloat(t0.pred));
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    }
    else if (t1 != null && !isVar(t1.pred)) {
      var a = Math.acos(parseFloat(t1.pred));
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[0], c.env, true)) return 1;
    }
    else return 0;
  }
  else if (t.pred == "math:degrees") {
    if (t0 != null && !isVar(t0.pred)) {
      var a = parseFloat(t0.pred) * 180 / Math.PI;
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    }
    else if (t1 != null && !isVar(t1.pred)) {
      var a = parseFloat(t0.pred) * Math.PI / 180;
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[0], c.env, true)) return 1;
    }
    else return 0;
  }
  else if (t.pred == "math:equalTo") {
    if (t0 != null && t1 != null && parseFloat(t0.pred) == parseFloat(t1.pred)) return 1;
    else return 0;
  }
  else if (t.pred == "math:greaterThan") {
    if (t0 != null && t1 != null && parseFloat(t0.pred) > parseFloat(t1.pred)) return 1;
    else return 0;
  }
  else if (t.pred == "math:lessThan") {
    if (t0 != null && t1 != null && parseFloat(t0.pred) < parseFloat(t1.pred)) return 1;
    else return 0;
  }
  else if (t.pred == "math:notEqualTo") {
    if (t0 != null && t1 != null && parseFloat(t0.pred) != parseFloat(t1.pred)) return 1;
    else return 0;
  }
  else if (t.pred == "math:notLessThan") {
    if (t0 != null && t1 != null && parseFloat(t0.pred) >= parseFloat(t1.pred)) return 1;
    else return 0;
  }
  else if (t.pred == "math:notGreaterThan") {
    if (t0 != null && t1 != null && parseFloat(t0.pred) <= parseFloat(t1.pred)) return 1;
    else return 0;
  }
  else if (t.pred == "math:difference" && t0 != null) {
    var a = parseFloat(evaluate(t0.args[0], c.env).pred);
    for (var ti = t0.args[1]; ti.args.length != 0; ti = ti.args[1]) a -= parseFloat(evaluate(ti.args[0], c.env).pred);
    if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    else return 0;
  }
  else if (t.pred == "math:exponentiation" && t0 != null) {
    var a = parseFloat(evaluate(t0.args[0], c.env).pred);
    for (var ti = t0.args[1]; ti.args.length != 0; ti = ti.args[1]) var a = Math.pow(a, parseFloat(evaluate(ti.args[0], c.env).pred));
    if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    else return 0;
  }
  else if (t.pred == "math:integerQuotient" && t0 != null) {
    var a = parseFloat(evaluate(t0.args[0], c.env).pred);
    for (var ti = t0.args[1]; ti.args.length != 0; ti = ti.args[1]) a /= parseFloat(evaluate(ti.args[0], c.env).pred);
    if (unify({pred:Math.floor(a).toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    else return 0;
  }
  else if (t.pred == "math:negation") {
    if (t0 != null && !isVar(t0.pred)) {
      var a = -parseFloat(t0.pred);
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    }
    else if (t1 != null && !isVar(t1.pred)) {
      var a = -parseFloat(t1.pred);
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[0], c.env, true)) return 1;
    }
    else return 0;
  }
  else if (t.pred == "math:product" && t0 != null) {
    var a = parseFloat(evaluate(t0.args[0], c.env).pred);
    for (var ti = t0.args[1]; ti.args.length != 0; ti = ti.args[1]) a *= parseFloat(evaluate(ti.args[0], c.env).pred);
    if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    else return 0;
  }
  else if (t.pred == "math:quotient" && t0 != null) {
    var a = parseFloat(evaluate(t0.args[0], c.env).pred);
    for (var ti = t0.args[1]; ti.args.length != 0; ti = ti.args[1]) a /= parseFloat(evaluate(ti.args[0], c.env).pred);
    if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    else return 0;
  }
  else if (t.pred == "math:remainder" && t0 != null) {
    var a = parseFloat(evaluate(t0.args[0], c.env).pred);
    for (var ti = t0.args[1]; ti.args.length != 0; ti = ti.args[1]) a %= parseFloat(evaluate(ti.args[0], c.env).pred);
    if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    else return 0
  }
  else if (t.pred == "math:rounded") {
    if (t0 != null && !isVar(t0.pred)) {
      var a = Math.round(parseFloat(t0.pred));
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    }
    else return 0;
  }
  else if (t.pred == "math:sin") {
    if (t0 != null && !isVar(t0.pred)) {
      var a = Math.sin(parseFloat(t0.pred));
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    }
    else if (t1 != null && !isVar(t1.pred)) {
      var a = Math.asin(parseFloat(t1.pred));
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[0], c.env, true)) return 1;
    }
    else return 0;
  }
  else if (t.pred == "math:sum" && t0 != null) {
    var a = parseFloat(evaluate(t0.args[0], c.env).pred);
    for (var ti = t0.args[1]; ti.args.length != 0; ti = ti.args[1]) a += parseFloat(evaluate(ti.args[0], c.env).pred);
    if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    else return 0;
  }
  else if (t.pred == "math:tan") {
    if (t0 != null && !isVar(t0.pred)) {
      var a = Math.tan(parseFloat(t0.pred));
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[1], c.env, true)) return 1;
    }
    else if (t1 != null && !isVar(t1.pred)) {
      var a = Math.atan(parseFloat(t1.pred));
      if (unify({pred:a.toString(), args:[]}, c.env, t.args[0], c.env, true)) return 1;
    }
    else return 0;
  }
  else if (t.pred == "rdf:first" && t0 != null && t0.pred == "." && t0.args.length != 0) {
    if (unify(t0.args[0], c.env, t.args[1], c.env, true)) return 1;
    else return 0;
  }
  else if (t.pred == "rdf:rest" && t0 != null && t0.pred == "." && t0.args.length != 0) {
    if (unify(t0.args[1], c.env, t.args[1], c.env, true)) return 1;
    else return 0;
  }
  else if (t.pred == "a" && t1 != null && t1.pred == "rdf:List" && t0 != null && t0.pred == ".") return 1;
  else if (t.pred == "a" && t1 != null && t1.pred == "rdfs:Resource") return 1;
  return -1;
  */
}

typedef map<string, vector<rule_t>> evidence_t;

pred_t evaluate ( pred_t t, env_t& env );
bool unify ( pred_t s, env_t senv, pred_t d, env_t denv, bool f );

template<typename ret_t>
ret_t prove ( pred_t goal, int maxNumberOfSteps ) {
	deque<proof_trace_item> queue;
	proof_trace_item s = { goal, 0, 0, 0, env_t(), grounds() };
	queue.emplace_back ( s );
	size_t step;
	evidence_t evidence;
	while ( queue.size() > 0 ) {
		proof_trace_item c = queue.pop_front();
		grounds g = c.ground;
		step++;
		if ( maxNumberOfSteps != -1 && step >= maxNumberOfSteps ) return "";
		if ( c.ind >= c.rule.body.size() ) {
			if ( !c.parent ) {
				for ( size_t i = 0; i < c.rule.body.size(); i++ ) {
					pred_t pred = evaluate ( c.rule.body[i], c.env);
					evidence[pred.pred].emplace_back ( rule_t{pred, pred_t{ "GND", c.ground} } );
				}
				continue;
			}
			if ( c.rule.body.size() ) g.push_back ( ground_t{c.rule, c.env} );
			proof_trace_item r = {c.parent->rule, c.parent->src, c.parent->ind, c.parent->parent, c.parent->env, g};
			unify ( c.rule.head, c.env, r.rule.body[r.ind], r.env, true );
			r.ind++;
			queue.push_back ( r );
			continue;
		}
		pred_t t = c.rule.body[c.ind];
		size_t b = builtin ( t, c );
		if ( b == 1 ) {
			g.emplace_back ( ground_t{ rule_t{ evaluate ( t, c.env ), vector<pred_t>() }, env_t() } );
			proof_trace_item r = {c.rule, c.src, c.ind, c.parent, c.env, g};
			r.ind++;
			queue.push_back ( r );
			continue;
		} else if ( !b ) continue;
		map<string, vector<rule_t>> cases;
		if ( cases.find ( t.pred ) == cases.end() ) continue;
		for ( size_t src = 0; src < cases[t.pred].size();) {
			rule_t rl = cases[t.pred][src++];
			grounds g = c.ground;
			if ( !rl.body.size() ) g.push_back ( ground_t{rl, vector<pred_t>() } );
			proof_trace_item r = {rl, src, 0, c, {}, g};
			if ( unify ( t, c.env, rl.head, r.env, true ) )
				for ( proof_trace_item ep = c; ; ep = ep.parent ) {
					if ( ep.src == c.src && unify ( ep.rule.head, ep.env, c.rule.head, c.env, false ) ) break;
					if ( !ep.parent ) {
						queue.push_front ( r );
						break;
					}
				}
		}
	}
}

bool unify ( pred_t s, env_t senv, pred_t d, env_t denv, bool f ) {
	if ( s.pred[0] == '?' ) {
		try {
			pred_t sval = evaluate ( s, senv );
			return unify ( sval, senv, d, denv, f );
		} catch ( ... ) {
			return true;
		}
	}
	if ( d.pred[0] == '?' )
		try {
			pred_t dval = evaluate ( d, denv );
			return unify ( s, senv, dval, denv, f );
		} catch ( ... ) {
			if ( f ) denv[d.pred] = evaluate ( s, senv );
			return true;
		}
	if ( s.pred == d.pred && s.args.size() == d.args.size() ) {
		for ( size_t i = 0; i < s.args.size(); i++ ) if ( !unify ( s.args[i], senv, d.args[i], denv, f ) ) return false;
		return true;
	}
	return false;
}

pred_t evaluate ( pred_t t, env_t& env ) {
	if ( t.pred[0] == '?' ) {
		auto it = env.find ( t.pred );
		if ( it != env.end() ) return evaluate ( it->second, env );
		else throw 0;
	} else if ( t.args.size() == 0 ) return t;
	else {
		vector<pred_t> n;
		for ( size_t i = 0; i < t.args.size(); ++i )
			try {
				n.push_back ( evaluate ( t.args[i], env ) );
			} catch ( ... ) {
				n.push_back ( pred_t{ t.args[i].pred, vector<pred_t>() } );
			}
		return {t.pred, n};
	}
}

