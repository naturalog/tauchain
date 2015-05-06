// c++ port of euler (EYE) js reasoner
#include <deque>
#include "parsers.h"
#include "json_escape.h"

#ifdef DEBUG
#define jst(x) std::clog <<x << ",\n";
void jstq(const char *x){ jst (jsq (x) ) }
#else
#define jst(x)
#define jstq(x)
#endif

struct pred_t {
	string pred;
	vector<pred_t> args;

	friend ostream& operator<< ( ostream& o, const pred_t &t ) {
		o << "{\"pred\":" << t.pred;
		if ( t.args.size() ) {
			o << "\"args\":[";
			for ( auto a = t.args.cbegin();; ) {
				o << *a;
				if ( ++a != t.args.cend() ) o << ",\n";
			}
			o<<"]";
		}
		return o << "}\n";
	}
};

typedef map<string, pred_t> env_t;
typedef std::shared_ptr<env_t> penv_t;

ostream& operator<< ( ostream& o, env_t const& r ) {
	o << "{";
	for ( auto rr = r.cbegin();; ) {
		o << jsq(rr->first) << ": " << rr->second;
		if ( ++rr != r.cend() ) o << ",\n";
	}
	return o << "}\n";
}

typedef vector<pred_t> preds_t;

ostream& operator<< ( ostream& o, preds_t const& r ) {
	o << "[";
	for ( auto rr = r.cbegin();; ) {
		o << *rr;
		if ( ++rr != r.cend() ) o << ",\n";
	}
	return o << "]\n";
}

struct rule_t {
	pred_t head;
	preds_t body;

	friend ostream& operator<< ( ostream& o, const rule_t &t ) {
		o << "{\"head\":" << t.head << "\"body\":" << t.body << "}\n";
		return o;
	}
};

typedef vector<rule_t> rules_t;
typedef map<string, rules_t> evidence_t;

ostream& operator<< ( ostream& o,  const rules_t &r ) {
	o << "[";
	for ( auto rr = r.cbegin();; ) {
		o << *rr;
		if ( ++rr != r.cend() ) o << ",\n";
	}
	return o << "]\n";
}

ostream& operator<< ( ostream& o, evidence_t const& r ) {
	o << "{";
	for ( auto rr = r.cbegin();; ) {
		o << rr->first << ": " << rr->second;
		if ( ++rr != r.cend() ) o << ",\n";
	}
	return o << "}\n";
}

struct rule_env {
	rule_t src;
	penv_t env;
	friend ostream& operator<< ( ostream& o, const rule_env &t ) {
		o << "{\"src\":" << t.src << "\"env\":" << t.env << "}\n";
		return o;
	}
};
typedef vector<rule_env> ground_t;
typedef std::shared_ptr<ground_t> pground_t;

pground_t aCopy ( pground_t f ) {
	pground_t r = make_shared<ground_t>();
	*r = *f;
	return r;
}

ostream& operator<< ( ostream& o, ground_t const& r ) {
	o << "[";
	for ( auto rr = r.cbegin();; ) {
		o << *rr;
		if ( ++rr != r.cend() ) o << ",\n";
	}
	return o << "]\n";
}

struct proof_trace_item {
	rule_t rule;
	int src, ind; // source of what, index of what?
	std::shared_ptr<proof_trace_item> parent;
	std::shared_ptr<env_t> env;
	std::shared_ptr<ground_t> ground;
	friend ostream& operator<< ( ostream& o, const proof_trace_item &t ) {
		o << "{\"rule\":" << t.rule;
		o << "\"src\":" << t.src;
		o << "\"ind\":" << t.ind;
		if ( t.parent ) o << "\"parent\":" << t.parent;
		o << "\"env\":" << t.env;
		o << "\"ground\":" << t.ground;	
		o << "}\n";
		return o;
	}
};
typedef std::shared_ptr<proof_trace_item> ppti;


int builtin ( pred_t, proof_trace_item& ) {
	/*
	    1:it unified
	    -1:no such builtin
	    0:it didnt unify
	*/
	jst(jsq("NO BEES yet PLZ!\n"));
	return -1;
}

pred_t evaluate ( const pred_t t, const penv_t env );
bool unify ( const pred_t s, const penv_t senv, const pred_t d, const penv_t denv );

pground_t gnd = make_shared<ground_t>();


ostream& operator<< ( ostream& o, deque<ppti> const& r ) {
	o << "[";
	for ( auto rr = r.cbegin();; ) {
		o << **rr;
		if ( ++rr != r.cend() ) o << ",\n";
	}
	return o << "]";
}


bool prove ( rule_t goal, int maxNumberOfSteps, evidence_t& cases, evidence_t& evidence ) {
	int step = 0;
	deque<ppti> queue;
	ppti s = make_shared<proof_trace_item> ( proof_trace_item { goal, 0, 0, 0, make_shared<env_t>(), gnd } );
	queue.emplace_back ( s );
	while ( queue.size() > 0 ) {
		jst ( "{\"step\":" << step << ",\"queue\":" << queue << "}" );
		ppti c = queue.front();
		queue.pop_front();
		pground_t g = aCopy ( c->ground );
		step++;
		if ( maxNumberOfSteps != -1 && step >= maxNumberOfSteps ) {
			trace (  "TIMEOUT!" << endl );
			return false;
		}
		if ( ( size_t ) c->ind >= c->rule.body.size() ) {
			if ( !c->parent ) {
				jstq( "no parent!" );
				for ( size_t i = 0; i < c->rule.body.size(); i++ ) {
					pred_t t = evaluate ( c->rule.body[i], c->env );
					rule_t tmp = {t, {{ "GND", {}}} };//well...
					for (auto gnd_item : *c->ground)
						tmp.body[0].args.push_back(gnd_item.src.head);
					//jst( "{\"evidence for \":" << t.pred << ",\n \"ground\": " <<  *c->ground << "}" );
					evidence[t.pred].push_back ( tmp );
				}
				continue;
			}

			//jstq ( "Q with parent, adding:" );
			if ( c->rule.body.size() != 0 ) g->push_back ( {c->rule, c->env} );
			ppti r = make_shared<proof_trace_item> ( proof_trace_item {c->parent->rule, c->parent->src, c->parent->ind, c->parent->parent, make_shared<env_t> ( *c->parent->env ) , g} );
			unify ( c->rule.head, c->env, r->rule.body[r->ind], r->env );
			r->ind++;
			//jst ( *r );
			queue.push_back ( r ); 
			continue;
		}
		//jstq ( "Done q" );
		
		pred_t t = c->rule.body[c->ind];
		
		size_t b = builtin ( t, *c );
		if ( b == 1 ) {
			g->emplace_back ( rule_env { { evaluate ( t, c->env ), vector<pred_t>() }, make_shared<env_t>() } );
			ppti r = make_shared<proof_trace_item> ( proof_trace_item {c->rule, c->src, c->ind, c->parent, c->env, g} );
			r->ind++;
			queue.push_back ( r );
			continue;
		} else if ( b == 0 )
		{   // builtin didnt unify
			continue; // else there is no such builtin, continue...
		}

		jst ( "{\"looking for case\":" << t.pred );
		if ( cases.find ( t.pred ) == cases.end() ) {
			jstq ( "No Cases(no such predicate)!" );
			jstq ( "available cases' keys:" );
			for ( auto x : cases ) jst ( x.first );
			continue;
		}
		size_t src = 0;
		//for each rule with the predicate we are trying to prove...
		for ( rule_t rl : cases[t.pred] ) {
			src++;
			pground_t g = aCopy ( c->ground );
			jst (  "{\"Check rule\":" << rl << "}" );
			if ( rl.body.size() == 0 ) 
				g->push_back ( { rl, make_shared<env_t>() } ); //its a fact
			ppti r = make_shared<proof_trace_item> ( proof_trace_item {rl, ( int ) src, 0, c, make_shared<env_t>(), g} );// why already here and not later?
			//rl could imply our rule...
			if ( unify ( t, c->env, rl.head, r->env ) ) {
				ppti ep = c;
				while ( ( ep = ep->parent ) ) {
					jst ( "{\"ep.src\": " << ep->src << ",  \"c.src\": " << c->src << "}" );
					if ( ep->src == c->src && unify ( ep->rule.head, ep->env, c->rule.head, c->env ) ) {
						jstq ( "  ~~ match ~~ " );
						break;
					}
					jstq ( "  ~~  ~~  ~~" );
				}
				if ( !ep ) {
					jst ( "{\"Adding to queue\": " << *r << "}");
					queue.push_front ( r );
				} else jstq ( "didn't reach top" );
				jstq ( "Done euler loop" );
			} else jstq ( "No loop here" );
		}
		jstq ( "done rule checks, looping" );
	}
	return false;
}

bool prove ( pred_t goal, int maxNumberOfSteps, evidence_t& cases, evidence_t& evidence ) {
	return prove ( rule_t {goal, { goal } }, maxNumberOfSteps, cases, evidence );
}

int _indent = 0;

bool unify ( const pred_t s, const penv_t senv, const pred_t d, const penv_t denv ) {
	//jst ("{\"Unify level\":" << _indent << ",\"s\":" << s << ",\"senv\":" << *senv);

	if ( s.pred[0] == '?' ) {
		try {
			pred_t sval = evaluate ( s, senv );
			_indent++;
			bool r = unify ( sval, senv, d, denv );
			_indent--;
			return r;
		} catch ( int n ) {
			if ( n ) {
				bt();
				throw std::runtime_error ( "Error during unify" );
			}
			if ( _indent ) _indent--;
			jstq ( "Match(free var)!" );
			return true;
		} catch ( exception ex ) {
			bt();
			throw ex;
		} catch ( ... ) {
			bt();
			throw std::runtime_error ( "Error during unify" );
		}
	}
	if ( d.pred[0] == '?' ) {
		try {
			pred_t dval = evaluate ( d, denv );
			bool b = unify ( s, senv, dval, denv );
			return b;
		} catch ( int n ) {
			if ( n ) {
				bt();
				throw std::runtime_error ( "Error during unify" );
			}
			( *denv ) [d.pred] = evaluate ( s, senv );
			jstq ( "Match(free var)!" );

			return true;
		} catch ( exception ex ) {
			throw ex;
		} catch ( ... ) {
			bt();
			throw std::runtime_error ( "Error during unify" );
		}
	}
	if ( s.pred == d.pred && s.args.size() == d.args.size() ) {
		//jstq ("Comparison:"  );
		for ( size_t i = 0; i < s.args.size(); i++ ) {
			_indent++;
			if ( !unify ( s.args[i], senv, d.args[i], denv ) ) {
				_indent--;
				//trace ( indent() << "    " << ( string ) s.args[i] << " != " << ( string ) d.args[i] << endl << flush );
				return false;
			}
			_indent--;
			//trace ( indent() << "    " << ( string ) s.args[i] << " == " << ( string ) d.args[i] << endl << flush );
		}
		jstq ( "Equal!" );
		return true;
	}
	jstq ( "No match" );
	return false;
}

pred_t evaluate ( const pred_t t, const penv_t env ) {
	//trace ( indent() << "Eval " << ( string ) t << " in " << ( *env ) << endl );
	if ( t.pred[0] == '?' ) {
		//trace (  "(" << ( string ) t << " is a var..)" << endl );
		auto it = env->find ( t.pred );
		if ( it != env->end() ) return evaluate ( it->second, env );
		else throw int ( 0 );
	} else if ( t.args.size() == 0 ) return t;
	vector<pred_t> n;
	for ( size_t i = 0; i < t.args.size(); ++i ) {
		try {
			n.push_back ( evaluate ( t.args[i], env ) );
		} catch ( int k ) {
			if ( k ) {
				bt();
				throw std::runtime_error ( "Uncaught exception during evaluate()." );
			}
			n.push_back ( { t.args[i].pred, vector<pred_t>() } );
		} catch ( exception ex ) {
			bt();
			throw ex;
		} catch ( ... ) {
			bt();
			throw std::runtime_error ( "Uncaught exception during evaluate()." );
		}
	}
	return {t.pred, n};
}

inline pred_t mk_res ( string r ) {
	return {r, {}};
}

bool test_reasoner() {
	cout << "QED!" << endl;
}

pred_t triple ( const string& s, const string& p, const string& o ) {
	return pred_t { p, { { s, {}}, { o, {}}}};
};

pred_t triple ( const jsonld::quad& q ) {
	return triple ( q.subj->value, q.pred->value, q.object->value );
};

void add_rule(const pred_t head, const string s, jsonld::rdf_db &kb, evidence_t &cases)
{
	rule_t rule{head, {}};
	//if (s == true)
	//{}
	//else
	//if (s == false)
	//throw ("absurdity")
	//else
	if (kb.find(s) != kb.end())
		for ( const auto& x : *kb[s] )
			rule.body.push_back ( triple ( *x ) );
	else
		throw std::runtime_error (s + "is not a graph in our kb");
	cases[rule.head.pred].push_back ( rule );
}

evidence_t prove ( const qlist& graph, const qlist& query, jsonld::rdf_db &kb ) {
	std::clog << "[";

	evidence_t evidence, cases;
	/*the way we store rules in jsonld is: graph1 implies graph2*/
	for ( const auto& quad : graph ) {
		const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
		cases[p].push_back ( { { p, { mk_res ( s ), mk_res ( o ) }}, {}} );
		if ( p == "http://www.w3.org/2000/10/swap/log#implies" ) {
			/*if (o == true)
				 ?//query? what do? add it as a triple or not?
			else if (o == false)
				{}//negation, ignored, added as triple
			else*/ if (kb.find(o) != kb.end())
				for ( const auto &y : *kb[o] )
					add_rule(triple(*y), s, kb, cases);
			else
				throw std::runtime_error ("HMC_Alpha> bananas is not a formula...");
		}
	}
	rule_t goal;
	for ( auto q : query ) 
		goal.body.push_back ( triple ( *q ) );
	prove ( goal, -1, cases, evidence );
	return evidence;
}


void print_evidence ( evidence_t evidence ) {
	jst ( evidence );
}

const bool use_nquads = false;
