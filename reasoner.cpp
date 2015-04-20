// c++ port of euler (EYE) js reasoner
#include <deque>
#include "jsonld.h"
using namespace std;

bool DEBUG = true;
#define trace(x) if(DEBUG) { x }

struct pred_t {
	string pred;
	vector<pred_t> args;
};

int _indent = 0;

string indent() {
	for ( int i = 0; i < _indent; i++ )
		cout << "##";
	return "";
}

void print ( pred_t p ) {
	cout << p.pred;
	if ( p.args.size() ) {
		cout << "(";
		for ( auto a = p.args.begin();;) {
			print ( *a );
			if ( ++a != p.args.end() ) cout << ", ";
			else break;
		}
		cout << ")";
	}
}
void print ( pred_t *p ) {
	print ( *p );
}

typedef map<string, pred_t> env_t;
typedef std::shared_ptr<env_t> penv_t;

void print ( env_t r ) {
	cout << "env of size " << r.size();
	cout << "{";
	for ( auto& rr : r ) {
		cout << rr.first << ": "; print ( rr.second ); cout << "; ";
	}
	cout << "}";
}

struct rule_t {
	pred_t head;
	vector<pred_t> body;
};

void print ( rule_t r ) {
	print ( r.head );
	cout << " :- ";
	for ( auto b : r.body ) {
		print ( b ); cout << "; ";
	}
}

struct rule_env {
	rule_t src;
	penv_t env;
};
typedef vector<rule_env> ground_t;
typedef std::shared_ptr<ground_t> pground_t;

pground_t aCopy ( pground_t f ) {
	pground_t r = make_shared<ground_t>();
	*r = *f;
	return r;
}

void print ( rule_env s ) {
	print ( s.src ); cout << " {";
	print ( *s.env );
	cout << "} ";
}

void print ( ground_t g ) {
	for ( auto gg : g ) {
		cout << "_"; print ( gg ); cout << "_";
	}
}

struct proof_trace_item {
	rule_t rule;
	int src, ind;
	std::shared_ptr<proof_trace_item> parent;
	std::shared_ptr<env_t> env;
	std::shared_ptr<ground_t> ground;
};
typedef std::shared_ptr<proof_trace_item> ppti;

void print ( proof_trace_item s ) {
	cout << "<<";
	print ( s.rule );
	cout << s.src << "," << s.ind << "(";
	if ( s.parent ) print ( *s.parent );
	cout << ") {env:"; print ( *s.env ); cout << "}[[ground:";
	print ( *s.ground );
	cout << "]]";
}

int builtin ( pred_t, proof_trace_item ) {
	return -1;
}
typedef map<string, vector<rule_t>> evidence_t;
size_t step = 0;

pred_t evaluate ( pred_t t, penv_t env );
bool unify ( pred_t s, penv_t senv, pred_t d, penv_t denv, bool f );

pground_t gnd = make_shared<ground_t>();

bool prove ( pred_t goal, int maxNumberOfSteps, evidence_t& cases, evidence_t& evidence ) {
	deque<ppti> queue;
	ppti s = make_shared<proof_trace_item> ( proof_trace_item{ {goal, {goal}}, 0, 0, 0, make_shared<env_t>(), gnd } ); //TODO: don't deref null parent ;-)
	queue.emplace_back ( s );
	//queue.push_back(s);
	trace ( cout << "Goal: "; print ( goal ); cout << endl; )
	while ( queue.size() > 0 ) {
		trace ( cout << "=======" << endl; )
		ppti c = queue.front();
		trace ( cout << "  c: "; print ( *c ); cout << endl; )
		queue.pop_front();
		pground_t g = aCopy ( c->ground );
		step++;
		if ( maxNumberOfSteps != -1 && step >= maxNumberOfSteps ) {
			trace ( cout << "TIMEOUT!" << endl; )
			return true;
		}
		trace (
		    cout << "c.ind: " << c->ind << endl;
		    cout << "c.rule.body.size(): " << c->rule.body.size() << endl;
		)
		if ( c->ind >= c->rule.body.size() ) {
			if ( !c->parent ) {
				trace ( cout << "no parent!" << endl; )
				//rule_t tmp;
				//pred_t tpred;
				for ( size_t i = 0; i < c->rule.body.size(); i++ ) {
					pred_t t = evaluate ( c->rule.body[i], c->env );
					pred_t tpred = { "GND", {}};
					rule_t tmp = {t, {tpred} };
					trace ( cout << "Adding evidence for " << t.pred << ": "; print ( tmp ); cout << endl; )
					evidence[t.pred].push_back ( tmp );
				}
				continue;
			}
			trace ( cout << "Q parent: "; )
			if ( c->rule.body.size() != 0 ) g->push_back ( {c->rule, c->env} );
			ppti r = make_shared<proof_trace_item> ( proof_trace_item{c->parent->rule, c->parent->src, c->parent->ind, c->parent->parent, c->parent->env, g} );
			unify ( c->rule.head, c->env, r->rule.body[r->ind], r->env, true );
			r->ind++;
			trace ( print ( *r ); cout << endl; )
			queue.push_back ( r );
			continue;
		}
		trace ( cout << "Done q" << endl; )
		pred_t t = c->rule.body[c->ind];
		size_t b = builtin ( t, *c );
		if ( b == 1 ) {
			g->emplace_back ( rule_env{ { evaluate ( t, c->env ), vector<pred_t>() }, make_shared<env_t>() } );
			ppti r = make_shared<proof_trace_item> ( proof_trace_item{c->rule, c->src, c->ind, c->parent, c->env, g} );
			r->ind++;
			queue.push_back ( r );
			continue;
		} else if ( !b ) {
			trace ( cout << "NO BEES PLZ!" << endl; )
			continue;
		}
		trace ( cout << "Checking cases..." << endl; )
		if ( cases.find ( t.pred ) == cases.end() ) {
			trace ( cout << "No Cases!" << endl; )
			continue;
		}
		size_t src = 0;
		for ( size_t k = 0; k < cases[t.pred].size(); k++ ) {
			rule_t rl = cases[t.pred][k];
			src++;
			pground_t g = aCopy ( c->ground );
			trace ( cout << "Check rule: "; print ( rl ); cout << endl; )
			if ( rl.body.size() == 0 )
				g->push_back ( {rl, make_shared<env_t>() } );
			ppti r = make_shared<proof_trace_item> ( proof_trace_item{rl, ( int ) src, 0, c, make_shared<env_t>(), g} );
			if ( unify ( t, c->env, rl.head, r->env, true ) ) {
				ppti ep = c;
				while ( ( ep = ep->parent ) ) {
					trace (
					    cout << "  ep.src: " << ep->src << endl;
					    cout << "  c.src: " << c->src << endl;
					)
					if ( ep->src == c->src && unify ( ep->rule.head, ep->env, c->rule.head, c->env, false ) ) {
						trace ( cout << "  ~~ match ~~ " << endl; )
						break;
					}
					trace ( cout << "  ~~  ~~  ~~" << endl; )
				}
				if ( !ep ) {
					trace ( cout << "Adding to queue: "; print ( *r ); cout << endl << flush; )
					queue.push_front ( r );
					//break;
				} else {
					trace ( cout << "didn't reach top" << endl; )
				}
				trace ( cout << "Done euler loop" << endl; )
			} else {
				trace ( cout << "No loop here" << endl; )
			}
		}
		trace ( cout << "done rule checks, looping" << endl; )
	}
	return false;
}

bool unify ( pred_t s, penv_t senv, pred_t d, penv_t denv, bool f ) {
	trace (
	    cout << indent() << "Unify:" << endl << flush;
	    cout << indent() << "  s: "; print ( s ); cout << " in "; print ( *senv ); cout << endl << flush;
	    cout << indent() << "  d: "; print ( d ); cout << " in "; print ( *denv ); cout << endl << flush;
	)
	if ( s.pred[0] == '?' ) {
		trace ( cout << indent() << "  source is var" << endl << flush; )
		try {
			pred_t sval = evaluate ( s, senv );
			_indent++;
			bool r = unify ( sval, senv, d, denv, f );
			_indent--;
			return r;
		} catch ( ... ) {
			_indent--;
			trace ( cout << indent() << " Match(free var)!" << endl; )
			return true;
		}
	}
	if ( d.pred[0] == '?' ) {
		trace ( cout << indent() << "  dest is var" << endl << flush; )
		try {
			pred_t dval = evaluate ( d, denv );
			trace ( _indent++; )
			bool b = unify ( s, senv, dval, denv, f );
			trace ( _indent--; )
			return b;
		} catch ( ... ) {
			//if ( f )
			( *denv ) [d.pred] = evaluate ( s, senv );
			trace ( _indent--; )
			trace ( cout << indent() << " Match!(free var)" << endl; )
			return true;
		}
	}
	if ( s.pred == d.pred && s.args.size() == d.args.size() ) {
		trace ( cout << indent() << "  Comparison:" << endl << flush; )
		for ( size_t i = 0; i < s.args.size(); i++ ) {
			trace ( _indent++; )
			if ( !unify ( s.args[i], senv, d.args[i], denv, f ) ) {
				trace ( _indent--;
				        cout << indent() << "    "; print ( s.args[i] ); cout << " != "; print ( d.args[i] ); cout << endl << flush;
				      )
				return false;
			}
			trace (
			    _indent--;
			    cout << indent() << "    "; print ( s.args[i] ); cout << " == "; print ( d.args[i] ); cout << endl << flush;
			)
		}
		trace ( cout << indent() << "  Equal!" << endl << flush; )
		return true;
	}
	trace ( cout << indent() << " No match" << endl << flush; )
	return false;
}

pred_t evaluate ( pred_t t, penv_t env ) {
	trace ( cout << indent() << "Eval "; print ( t ); cout << " in "; print ( *env ); cout << endl; )
	if ( t.pred[0] == '?' ) {
		trace ( cout << "("; print ( t ); cout << " is a var..)" << endl; );
		auto it = env->find ( t.pred );
		if ( it != env->end() )
			return evaluate ( it->second, env );
		else
			throw 0;
	} else if ( t.args.size() == 0 )
		return t;
	else {
		pred_t tmp;
		vector<pred_t> n;
		for ( size_t i = 0; i < t.args.size(); ++i ) {
			try {
				n.push_back ( evaluate ( t.args[i], env ) );
			} catch ( ... ) {
				n.push_back ( tmp = { t.args[i].pred, vector<pred_t>() } );
			}
		}
		return {t.pred, n};
	}
}

inline pred_t mk_res(string r) { return {r, {}}; }
/*
void prove(pred_t goal) {
	pred_t Socrates = mk_res("Socrates"), Man = mk_res("Man"), Mortal = mk_res("Mortal"), Morrtal = mk_res("Morrtal"), Male = mk_res("Male"), _x = mk_res("?x"), _y = mk_res("?y");
	cases["a"].push_back ( {{"a", {Socrates, Male}}, {}} );
	cases["a"].push_back ( {
		{"a", {_x, Mortal}},
		{ {"a", {_x, Man}}, }
	} );
	cases["a"].push_back ( {
		{"a", {_x, Man}},
		{ {"a", {_x, Male}}, }
	} );

	bool p = prove ( goal, -1, cases, evidence );
	cout << "Prove returned " << p << endl;
	cout << "evidence: " << evidence.size() << " items..." << endl;
	for ( auto e : evidence ) {
		cout << "  " << e.first << ":" << endl;
		for ( auto ee : e.second ) {
			cout << "    "; print ( ee ); cout << endl;
		}
		cout << endl << "---" << endl;
	}
	cout << "QED!" << endl;
}
*/
int main(int argc, char** argv) {
	if ( argc != 2 && argc != 6) {
		cout << "Usage:"<<endl<<"\ttau <JSON-LD kb file> <Graph Name> <Goal's subject> <Goal's predicate> <Goal's object>" << endl;
		cout << "Or to list all available graphs:"<<endl<<"\ttau <JSON-LD input file>"<< endl;
		return 1;
	}
	auto kb = jsonld::load_jsonld(argv[1]);

	if (argc == 2) return 0;
	auto it = kb.find(argv[2]) ;
	if (it == kb.end()) { cerr<<"No such graph."<<endl; return 1; }

	evidence_t evidence, cases;
	for (auto quad : *it->second) {
		const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
		cases[p].push_back( rule_t{ pred_t{ p, { pred_t{s,{}}, pred_t{o,{}}}},{}});
	}
	bool p = prove(pred_t{argv[4],{{argv[3],{}},{argv[5],{}}}}, -1, evidence, cases);
	cout << "Prove returned " << p << endl;
	cout << "evidence: " << evidence.size() << " items..." << endl;
	for ( auto e : evidence ) {
		cout << "  " << e.first << ":" << endl;
		for ( auto ee : e.second ) {
			cout << "    "; print ( ee ); cout << endl;
		}
		cout << endl << "---" << endl;
	}
	return 0;
}
