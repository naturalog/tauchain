struct pred_t {
	string pred;
	vector<pred_t> args;
};

struct rule_t {
	head;
	vector<rule_t> body;
};

struct s2 {
	rule_t src;
	env;
};
typedef vector<s2> ground_t;

struct s1 {
	rule_t rule;
	src;
	ind;
	s1* parent;
	env;
	ground_t ground;
};


typedef map evidence_t;

function prove(goal, maxNumberOfSteps) {
  s1.push_back({ goal, 0, 0, 0, {}, [] });
  size_t step;
  evidence_t<pred_t, rule_t> evidence;
  while (queue.size() > 0) {
    s1 c = queue.pop_front();
    ground_d g = c.ground;
    step++;
    if (maxNumberOfSteps != -1 && step >= maxNumberOfSteps) return '';
    if (c.ind >= c.rule.body.size()) {
      if (!c.parent) {
        for (size_t i = 0; i < c.rule.body.size(); i++) {
          var t = evaluate(c.rule.body[i], c.env);
	  evidence[t.pred] = vector();
          evidence[t.pred].emplace_back( {t, {pred:'GND', args:c.ground} });
        }
        continue;
      }
      if (c.rule.body.size() != 0) g.push({c.rule, c.env});
      s1 r{c.parent.rule, c.parent->src, c.parent->ind, c.parent->parent, c.parent.env, g};
      unify(c.rule.head, c.env, r.rule.body[r.ind], r.env, true);
      r.ind++;
      queue.push_back(r);
      continue;
    }
    rule_t t = c.rule.body[c.ind];
    size_t b = builtin(t, c);
    if (b == 1) {
      g.emplace_back({{evaluate(t, c.env), vector() }, env_t()});
      var r = {c.rule, c.src, c.ind, c.parent, c.env, g};
      r.ind++;
      queue.push_back(r);
      continue;
    }
    else if (!b) continue;
    map<,vector<rule_t>> cases;
    if (cases.find(t.pred) == cases.end()) continue;
    var src = 0;
    for (var k = 0; k < cases[t.pred].size(); k++) {
      rule_t rl = cases[t.pred][k];
      src++;
      ground_t g = c.ground;
      if (rl.body.size() == 0) g.push({rl, {}});
      s1 r{rl, src, 0, c, {}, g};
      if (unify(t, c.env, rl.head, r.env, true)) {
        var ep = c;  // euler path
        while (ep = ep.parent) if (ep.src == c.src && unify(ep.rule.head, ep.env, c.rule.head, c.env, false)) break;
        if (ep == null) queue.unshift(r);
      }
    }
  }
}

function unify(s, senv, d, denv, f) {
  if (s.pred[0]=='?') {
    var sval = evaluate(s, senv);
    if (sval != null) return unify(sval, senv, d, denv, f);
    else return true;
  }
  else if (d.pred[0]=='?') {
    var dval = evaluate(d, denv);
    if (dval != null) return unify(s, senv, dval, denv, f);
    else {
      if (f != null) denv[d.pred] = evaluate(s, senv);
      return true;
    }
  }
  else if (s.pred == d.pred && s.args.size() == d.args.size()) {
    for (var i = 0; i < s.args.size(); i++) if (!unify(s.args[i], senv, d.args[i], denv, f)) return false;
    return true;
  }
    return false;
}

pred_t evaluate(t, env) {
  if (isVar(t.pred)) {
    var a = env[t.pred];
    if (a != null) return evaluate(a, env);
    else return null;
  }
  else if (t.args.size() == 0) return t
  else {
    vector<pred_t> n;
    for (var i = 0; i < t.args.size(); i++) {
      var a = evaluate(t.args[i], env);
      if (a != null) n.back(a);
      else n.push({pred:t.args[i].pred, args:[]});
    }
    return {t.pred, n};
  }
}

