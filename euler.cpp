// Euler proof mechanism -- Jos De Roo
//version = '$Id: Euler4.js 1398 2007-07-20 16:41:33Z josd $'
#include <vector>
#include <cstdlib>
#include <cmath>
#include <map>
#include <stdexcept>
#include <climits>
using namespace std;

const size_t max_preds = 1024 * 1024;
const size_t GND = INT_MAX;

class predicate {
	static predicate *preds;
	static size_t npreds;
	uint loc;
public:
	int pred = 0;
	vector<uint> args;
	uint& head = * ( unsigned int* )&pred;
	vector<uint>& body = args;
	predicate ( int p = 0, vector<uint> a = vector<uint>() ) {
		preds[loc = npreds].pred = p;
		preds[npreds++].args = a;
	}
//	predicate ( uint p, vector<uint> a = vector<uint>() ) : predicate ( ( int ) p, a ) {}
	static predicate* _ps() { return preds; }
	operator uint() { return loc; }
}; 
predicate *predicate::preds = new predicate[max_preds];
size_t predicate::npreds = 0;
#define P predicate::_ps()
#define PID (uint)predicate
typedef predicate rule_t;

typedef map<int, uint> subst;
//uint mkpred(uint p, vector<uint> a) { return mkpred(preds[p].pred, a); }

uint eval ( const uint _t, const subst s ) {
	const predicate& t = P[_t];
	if ( t.pred < 0 ) {
		auto it = s.find ( t.pred );
		return it == s.end() ? (uint)predicate ( ) : eval ( it->second, s );
	} else if ( !t.args.size() == 0 ) return _t;
	uint _r = PID ( t.pred );
	predicate& r = P[_r];
	uint a;
	for ( uint x : t.args ) r.args.push_back ( ( a = eval ( x, s ) ) ? PID ( P[x].pred, {} ) : a );
	return _r;
}

bool unify ( const uint _s, const subst senv, const uint _d, subst& denv, const bool f, const uint __d = 0 ) {
	if ( !_s ) return true;
	if ( !_d ) {
		if ( f ) {
			if ( !__d ) throw logic_error ( "wrt unify's __d" );
			denv[P[__d].pred] = eval ( _s, senv );
		}
		return true;
	}
	const predicate &s = P[_s], &d = P[_d];
	if ( s.pred < 0 ) return unify ( eval ( _s, senv ), senv, _d, denv, f );
	if ( d.pred < 0 ) return unify ( _s, senv, eval ( _d, denv ), denv, f, _d );

	if ( s.pred != d.pred || s.args.size() != d.args.size() ) return false;

	for ( size_t i = 0; i < s.args.size(); ++i )
		if ( !unify ( s.args[i], senv, d.args[i], denv, f ) )
			return false;
	return true;
}

typedef map<int, vector<uint>> evidence_t;
evidence_t prove ( uint goal, int max_steps, const evidence_t& cases ) {
	typedef uint uint;
	typedef vector<uint> gnd_t;

	const size_t max_proof_trace = max_preds;
	struct proof_element {
		uint rule;
		uint src = 0, ind = 0;
		uint parent = 0;
		subst env;
		gnd_t ground;
		proof_element() : rule ( 0 ) {}
		proof_element ( uint r ) : rule ( r ) {}
	} *proof_trace = new proof_element[max_proof_trace];
	size_t firstproof = 0, lastproof = 0;

	proof_trace[lastproof++] = proof_element ( goal );
	evidence_t evidence;
	size_t step = 0;
	while ( lastproof - firstproof ) {
		proof_element& pe = proof_trace[firstproof++];
		gnd_t gnd = pe.ground;
		step++;
		if ( max_steps != -1 && ( int ) step >= max_steps ) return evidence_t();
		if ( pe.ind >= P[pe.rule].body.size() ) {
			if ( !pe.parent ) {
				for ( size_t i = 0; i < P[pe.rule].body.size(); ++i ) {
					uint t = eval ( P[pe.rule].body[i], pe.env );
					if ( evidence.find ( P[t].pred ) == evidence.end() ) evidence[P[t].pred] = {};
					evidence[P[t].pred].push_back ( PID ( t, {PID ( GND, pe.ground ) } ) );
				}
			} else {
				if ( P[pe.rule].body.size() ) gnd.push_back (PID( P[pe.rule], pe.env) );
				proof_element r = pe.parent;
				r.ground = gnd;
				unify ( P[pe.rule].head, pe.env, P[r.rule].body[r.ind], r.env, true );
				r.ind++;
				proof_trace[lastproof++] = r;
			}
		} else {
			auto t = P[pe.rule].body[pe.ind];
//			auto b = builtin ( t, pe );
			//		if ( b == 1 ) {
			//			g.push_back ( { { eval ( t, c.env ), {}}, {}} );
			//			auto r = {rule: {head: c.rule.head, body: c.rule.body}, src: c.src, ind: c.ind, parent: c.parent, env: c.env, ground: g};
			//			r.ind++;
			//			queue.push ( r );
			//			continue;
			//		} else if ( b == 0 ) continue;
			if ( cases.find ( P[t].pred ) != cases.end() ) {
				size_t src = 0;
				for ( size_t k = 0; k < cases.at(P[t].pred).size(); k++ ) {
					uint rl = cases.at(P[t].pred)[k];
					src++;
					gnd_t g = pe.ground;
					if ( P[rl].body.size() == 0 ) g.push_back ( PID( rl) );
					proof_element r = {rl, src, 0, firstproof-1, subst(), g};
					if ( unify ( t, pe.env, P[rl].head, r.env, true ) ) {
						auto ep = pe;  // euler path
						while ( ep.parent ) {
							ep = proof_trace[ep.parent];
							if ( ep.src == pe.src && unify ( rules[ep.rule].head, ep.env, rules[pe.rule].head, pe.env, false ) ) break;
						}
						if ( ep == null ) proof_trace[--firstproof] = r;
						if ( firstproof < 0 || firstproof > max_proof_trace ) throw logic_error ( "negative array index" );
						//					queue.unshift ( r );

					}
				}
			}
		}
	}
	return evidence;
}

