#include <string>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <deque>
#include <iostream>
using namespace std;

//#define DEBUG 1
#define trace(x) if(DEBUG) { x }

struct pred_t {
	string pred;
	vector<pred_t> args;
};

int _indent = 0;

string indent() {
  for(int i=0;i<_indent;i++) {
    cout << "##";
  }
  return "";
}

void print(pred_t p) {
  cout << p.pred << "(";
  for(auto a : p.args) {
    print(a);
  }
  cout << ")";
}
void print(pred_t *p) {print(*p);}

typedef map<string, pred_t> env_t;

void print(env_t r) {
  cout << "{";
  for(auto& rr : r) {
    cout << rr.first << ": "; print(rr.second); cout << "; ";
  }
  cout << "}";
}
void print(env_t *r) {print(*r);}

struct rule_t {
	pred_t head;
	vector<pred_t> body;
};

void print(rule_t r) {
  print(r.head);
  cout << " :- ";
  for(auto b : r.body) {
    print(b); cout << "; ";
  }
}
void print(rule_t *r) {print(*r);}

struct s2 {
	rule_t src;
	env_t *env;
};
typedef vector<s2*> ground_t;

ground_t* aCopy(ground_t *f) {
  ground_t *r = new ground_t;
  *r = *f;
  return r;
}

void print(s2 s) {
  print(s.src); cout << " {";
  print(s.env);
  cout << "} ";
}
void print(s2 *s) {print(*s);}

void print(ground_t g) {
  for(auto gg : g) {
    cout << "_"; print(gg); cout << "_";
  }
}
void print(ground_t *g) {print(*g);}

struct s1 {
	rule_t rule;
	int src, ind;
	s1* parent;
	env_t *env;
	ground_t *ground;
};

void print(s1 s) {
  cout << "<<";
  print(s.rule);
  cout << s.src << "," << s.ind << "(";
  if(s.parent != (s1*)0)
    print(*s.parent);
  cout << ") {"; print(s.env); cout << "}[[";
  print(s.ground);
  cout << "]]";
}
void print(s1 *s) {print(*s);}

int builtin ( pred_t, s1 ) { return -1; }
typedef map<string, vector<rule_t>> evidence_t;
evidence_t evidence;
evidence_t cases;
size_t step=0;

pred_t evaluate ( pred_t t, env_t *env );
bool unify ( pred_t s, env_t *senv, pred_t d, env_t *denv, bool f );

ground_t *gnd = new ground_t();

bool prove ( pred_t goal, int maxNumberOfSteps ) {
	deque<s1*> queue;
	s1 *s = new s1({ {goal,{goal}}, 0, 0, (s1*)0, new env_t(), gnd }); //TODO: don't deref null parent ;-)
	queue.emplace_back ( s );
	//queue.push_back(s);
	trace(cout << "Goal: "; print(goal); cout << endl;)
	while ( queue.size() > 0 ) {
	  trace(cout << "=======" << endl;)
		s1 *c = queue.front();
		trace(cout << "  c: "; print(*c); cout << endl;)
		queue.pop_front();
		ground_t *g = aCopy(c->ground);
		step++;
		if ( maxNumberOfSteps != -1 && step >= maxNumberOfSteps ) {
		  trace(cout << "TIMEOUT!" << endl;)
		  return true;
		}
		trace(
  		cout << "c.ind: " << c->ind << endl;
  		cout << "c.rule.body.size(): " << c->rule.body.size() << endl;
  	)
		if ( c->ind >= c->rule.body.size() ) {
			if ( !c->parent ) {
			  trace(cout << "no parent!" << endl;)
				//rule_t tmp;
				//pred_t tpred;
				for ( size_t i = 0; i < c->rule.body.size(); i++ ) {
					pred_t t = evaluate ( c->rule.body[i], c->env );
					pred_t tpred = { "GND", {}};
					rule_t tmp = {t, {tpred} };
					trace(cout << "Adding evidence for " << t.pred << ": "; print(tmp); cout << endl;)
					evidence[t.pred].push_back ( tmp );
				}
				continue;
			}
			trace(cout << "Q parent: ";)
			s2 *tmp = new s2({c->rule, c->env});
			if ( c->rule.body.size() != 0 ) g->push_back ( tmp );
			s1 *r = new s1({c->parent->rule, c->parent->src, c->parent->ind, c->parent->parent, c->parent->env, g});
			unify ( c->rule.head, c->env, r->rule.body[r->ind], r->env, true );
			r->ind++;
			trace(print(r); cout << endl;)
			queue.push_back ( r );
			continue;
		}
		trace(cout << "Done q" << endl;)
		pred_t t = c->rule.body[c->ind];
		size_t b = builtin ( t, *c );
		if ( b == 1 ) {
			rule_t rtmp = { evaluate ( t, c->env ), vector<pred_t>()};
			s2 *tmp = new s2({ rtmp, new env_t() });
			g->emplace_back ( tmp );
			s1 *r = new s1({c->rule, c->src, c->ind, c->parent, c->env, g});
			r->ind++;
			queue.push_back ( r );
			continue;
		} else if ( !b ) {
  		trace(cout << "NO BEES PLZ!" << endl;)
		  continue;
		}
		trace(cout << "Checking cases..." << endl;)
		if ( cases.find ( t.pred ) == cases.end() ) {
		  trace(cout << "No Cases!" << endl;)
		  continue;
		}
		size_t src = 0;
		for ( size_t k = 0; k < cases[t.pred].size(); k++ ) {
			rule_t rl = cases[t.pred][k];
			src++;
			ground_t *g = aCopy(c->ground);
			s2 *tmp = new s2({rl, new env_t() });
			trace(cout << "Check rule: "; print(rl); cout << endl;)
			if ( rl.body.size() == 0 )
			  g->push_back( tmp );
			s1 *r =new s1({rl, (int)src, 0, c, new env_t(), g});
			if ( unify ( t, c->env, rl.head, r->env, true ) ) {
			  s1 *ep = c;
			  while((ep = ep->parent) != (s1*)0) {
			    trace(
  				  cout << "  ep.src: " << ep->src << endl;
	  			  cout << "  c.src: " << c->src << endl;
	  			  )
					if ( ep->src == c->src && unify ( ep->rule.head, ep->env, c->rule.head, c->env, false ) ) {
					  trace(cout << "  ~~ match ~~ " << endl;)
					  break;
					}
					trace(cout << "  ~~  ~~  ~~" << endl;)
				}
				if ( ep == (s1*)0 ) {
				  trace(cout << "Adding to queue: "; print(r); cout << endl << flush;)
					queue.push_front ( r );
					//break;
				} else {
				  trace(cout << "didn't reach top" << endl;)
				}
				trace(cout << "Done euler loop" << endl;)
			} else {
			  trace(cout << "No loop here" << endl;)
			}
		}
		trace(cout << "done rule checks, looping" << endl;)
	}
	return false;
}

bool unify ( pred_t s, env_t *senv, pred_t d, env_t *denv, bool f ) {
  trace(
    cout << indent() << "Unify:" << endl << flush;
    cout << indent() << "  s: "; print(s); cout << " in "; print(*senv); cout << endl << flush;
    cout << indent() << "  d: "; print(d); cout << " in "; print(*denv); cout << endl << flush;
  )
	if ( s.pred[0] == '?' ) {
	  trace(cout << indent() << "  source is var" << endl << flush;)
		try {
			pred_t sval = evaluate ( s, senv );
			_indent++;
			bool r = unify ( sval, senv, d, denv, f );
			_indent--;
			return r;
		} catch ( ... ) {
		  _indent--;
		  trace(cout << indent() << " Match!" << endl;)
			return true;
		}
	}
	if ( d.pred[0] == '?' ) {
	  trace(cout << indent() << "  dest is var" << endl << flush;)
		try {
			pred_t dval = evaluate ( d, denv );
			trace(_indent++;)
			bool b = unify ( s, senv, dval, denv, f );
			trace(_indent--;)
			return b;
		} catch ( ... ) {
			//if ( f )
			  (*denv)[d.pred] = evaluate ( s, senv );
			trace(_indent--;)
		  trace(cout << indent() << " Match!" << endl;)
			return true;
		}
	}
	if ( s.pred == d.pred && s.args.size() == d.args.size() ) {
	  trace(cout << indent() << "  Comparison:" << endl << flush;)
		for ( size_t i = 0; i < s.args.size(); i++ ) {
		  trace(_indent++;)
		  if ( !unify ( s.args[i], senv, d.args[i], denv, f ) ) {
		    trace(_indent--;
		      cout << indent() << "    "; print(s.args[i]); cout << " != "; print(d.args[i]); cout << endl << flush;
		    )
		    return false;
		  }
		  trace(
  		  _indent--;
        cout << indent() << "    "; print(s.args[i]); cout << " == "; print(d.args[i]); cout << endl << flush;
      )
		}
		trace(cout << indent() << "  Equal!" << endl << flush;)
		return true;
	}
	trace(cout << indent() << " No match" << endl << flush;)
	return false;
}

pred_t evaluate ( pred_t t, env_t *env ) {
  trace(cout << indent() << "Eval "; print(t); cout << " in "; print(env); cout << endl;)
	if ( t.pred[0] == '?' ) {
		auto it = env->find ( t.pred );
		if ( it != env->end() ) { return evaluate ( it->second, env ); }
		else {throw 0; }
	} else if ( t.args.size() == 0 ) {return t;}
	else {
		pred_t tmp;
		vector<pred_t> n;
		for ( size_t i = 0; i < t.args.size(); ++i ) {
			try {
				n.push_back ( evaluate ( t.args[i], env ) );
			} catch ( ... ) {
				n.push_back ( tmp = { t.args[i].pred, vector<pred_t>() } );
		}}
		return {t.pred, n};
	}
}

void funtest() {
  vector<rule_t> kb;
  pred_t socrates = {":socrates", {}};
  pred_t Male = {":Male",{}};
  pred_t Man = {":Man",{}};
  pred_t Mortal = {":Mortal", {}};
  pred_t Morrrrtal = {":Morrrrtal", {}};
  pred_t _x = {"?x",{}};
  pred_t _y = {"?y",{}};
  vector<pred_t> True;
  cases["a"].push_back({{"a", {socrates, Male}},True});
  cases["a"].push_back({
    {"a", {_x, Mortal}},
    {
      {"a", {_x, Man}},
    }});
  cases["a"].push_back({
    {"a", {_x, Man}},
    {
      {"a", {_x, Male}},
    }});

  bool p = prove({"a", {_y, Mortal}}, 10);
  cout << "Prove returned " << p << endl;
  cout << "evidence: " << evidence.size() << " items..." << endl;
  for(auto e : evidence) {
    cout << "  " << e.first << ":" << endl;
    for(auto ee : e.second) {
      cout << "    "; print(ee); cout << endl;
    }
    cout << endl << "---" << endl;
  }
  cout << "QED!" << endl;
}

int main() {
  funtest();
  return 0;
}
