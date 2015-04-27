// Euler proof mechanism -- Jos De Roo
//version = '$Id: Euler4.js 1398 2007-07-20 16:41:33Z josd $'
#include <vector>
#include <cstdlib>
#include <cmath>
#include <map>
#include <stdexcept>
#include <climits>
#include <deque>
#include <iostream>
#include <sstream>
#include <strings.h>
using namespace std;

const size_t max_preds = 1024 * 1024;
const size_t GND = INT_MAX;

typedef map<int, uint> subst;
typedef vector<pair<uint, subst>> gnd_t;

class bidict {
	map<int, string> m1;
	map<string, int> m2;
	size_t lastk = 1;
public:
	const int set ( const string& v ) {
		int k = ( int ) lastk++;
		if ( v[0] == '?' ) k = -k;
		m1[k] = v;
		m2[v] = k;
		return k;
	}
	void set ( const int& k, const string& v ) {
		m1[k] = v;
		m2[v] = k;
	}
	const string operator[] ( const int& k ) {
		return m1[k];
	}
	const int operator[] ( const string& v ) {
		return m2[v];
	}
	bool has ( const int& k ) const {
		return m1.find ( k ) != m1.end();
	}
	bool has ( const string& v ) const {
		return m2.find ( v ) != m2.end();
	}
	string tostr() {
		stringstream s;
		for ( auto x : m1 ) cout << x.first << " := " << x.second << endl;
		return s.str();
	}
} dict;

class predicate {
	static predicate *preds;
	static size_t npreds;
public:
	uint loc;
	int pred = 0;
	vector<uint> args;
	uint& head = * ( unsigned int* )&pred;
	vector<uint>& body = args;
	static predicate* _ps() {
		return preds;
	}
	operator uint() {
		return loc;
	}
	string tostr() const {
		stringstream o;
		o << loc << '\t';
		if ( args.size() ) {
			auto it = args.begin();
			for ( ;; ) {
				const predicate& p = preds[*it];
				o << dict[p.pred] << '(' << p.pred << ')';
				++it;
				if ( it != args.end() ) o << " | ";
				else break;
			}
		} else o << "Ground";
		o << " => " << dict[pred] << '(' << pred << ')';
		return o.str();
	}
	static string str() {
		stringstream ss;
		for ( size_t n = 1; n < npreds; ++n ) ss << preds[n].tostr() << endl;
		return ss.str();
	}
	static predicate& mkpred ( int p = 0, vector<uint> a = vector<uint>() ) {
		predicate& r = preds[npreds];
		r.loc = npreds++;
		r.pred = p;
		r.args = a;
		return r;
	}
};
predicate *predicate::preds = []() {
	auto r = new predicate[max_preds];
	predicate::npreds = 1;
	return r;
}();
size_t predicate::npreds = 1;
#define P predicate::_ps()
typedef predicate rule_t;

uint mkpred ( int p = 0, vector<uint> a = vector<uint>() ) {
	predicate& r = predicate::mkpred ( p, a );
	return r.loc;
}

uint mkpred ( int p, int q ) {
	return mkpred ( p, vector<uint> ( 1, q ) );
}

uint mkpred ( int p, gnd_t g ) {
	uint r = mkpred ( p );
	for ( auto x : g ) P[p].args.push_back ( x.first );
	return r;
}

uint mkpred ( string s )  {
	return mkpred ( dict.has ( s )  ? dict[s] : dict.set ( s ) );
}

//uint mkpred(uint p, vector<uint> a) { return mkpred(preds[p].pred, a); }

uint eval ( const uint _t, const subst s ) {
	const predicate& t = P[_t];
	if ( t.pred < 0 ) {
		auto it = s.find ( t.pred );
		return it == s.end() ? ( uint ) predicate ( ) : eval ( it->second, s );
	} else if ( !t.args.size() == 0 ) return _t;
	uint _r = mkpred ( t.pred );
	predicate& r = P[_r];
	uint a;
	for ( uint x : t.args ) r.args.emplace_back ( ( a = eval ( x, s ) ) ? mkpred ( P[x].pred ) : a );
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
int builtin() {
	return -1;
}
typedef map<int, vector<uint>> evidence_t;

ostream& operator<< ( ostream& o, const subst& s ) {
	for ( auto x : s ) o << x.first << " := " << x.second << endl;
	return o;
}
ostream& operator<< ( ostream& o, const gnd_t& s ) {
	for ( auto x : s ) o << x.first << " := " << x.second << endl;
	return o;
}

evidence_t prove ( uint goal, int max_steps, const evidence_t& cases ) {
	static bool first = true;
	if ( first ) {
		cout << "prove called, dictionary: " << endl << dict.tostr() << endl;
		first = false;
	}
	typedef uint uint;
	cout << "kb:" << endl << predicate::str() << endl;

	struct proof_element {
		uint rule, src, ind;
		proof_element* parent;
		subst env;
		gnd_t ground;
		proof_element ( uint r = 0, uint s = 0, uint i = 0, proof_element* p = 0, subst e = subst(), gnd_t g = gnd_t() ) :
			rule ( r ), src ( s ), ind ( i ), parent ( p ), env ( e ), ground ( g ) {
			cout << "new proof_element with: ";
			cout << "rule" << '(' << rule << "): " << P[rule].tostr() << ", ";
			cout << "src: " << src << ", ";
			cout << "ind: " << ind << ", ";
			cout << "parent: " << parent << ", ";
			cout << "env: " << env << ", ";
			cout << "gnd: " << ground << endl;
		}
	};
	deque<proof_element> proof_trace;

	proof_trace.emplace_back ( mkpred(0, {goal}) );
	evidence_t evidence;
	size_t step = 0;
	while ( proof_trace.size() ) {
		proof_element& pe = proof_trace.front();
		proof_trace.pop_front();
		gnd_t gnd = pe.ground;
		step++;
		if ( max_steps != -1 && ( int ) step >= max_steps ) return evidence_t();
		if ( pe.ind >= P[pe.rule].body.size() ) {
			if ( !pe.parent ) {
				for ( size_t i = 0; i < P[pe.rule].body.size(); ++i ) {
					uint t = eval ( P[pe.rule].body[i], pe.env );
					if ( evidence.find ( P[t].pred ) == evidence.end() ) evidence[P[t].pred] = {};
					evidence[P[t].pred].emplace_back ( mkpred ( t, vector<uint> {mkpred ( GND, pe.ground ) } ) );
				}
			} else {
				if ( P[pe.rule].body.size() ) gnd.emplace_back ( P[pe.rule], pe.env );
				proof_element r = *pe.parent;
				r.ground = gnd;
				unify ( P[pe.rule].head, pe.env, P[r.rule].body[r.ind], r.env, true );
				r.ind++;
				proof_trace.emplace_back ( r );
			}
		} else {
			uint t = P[pe.rule].body[pe.ind];
			int b = builtin ();// t, pe );
			if ( b == 1 ) {
				gnd.emplace_back ( eval ( t, pe.env ), subst() );
				proof_element r ( mkpred ( P[pe.rule].head, P[pe.rule].body ), pe.src, pe.ind, pe.parent, pe.env, gnd );
				r.ind++;
				proof_trace.push_back ( r );
				continue;
			} else if ( b == 0 ) continue;
			cout << "now looking in cases for " << t << endl;
			cout << "cases:" << endl << predicate::str() << endl;
			if ( cases.find ( P[t].pred ) != cases.end() ) {
				size_t src = 0;
				for ( size_t k = 0; k < cases.at ( P[t].pred ).size(); k++ ) {
					uint rl = cases.at ( P[t].pred ) [k];
					src++;
					gnd_t gnd = pe.ground;
					if ( !P[rl].body.size() ) gnd.emplace_back ( mkpred ( rl ), subst() );
					proof_element r ( rl, src, 0, &pe, subst(), gnd );
					if ( unify ( t, pe.env, P[rl].head, r.env, true ) ) {
						auto ep = pe;  // euler path
						while ( ep.parent ) {
							ep = *ep.parent;
							if ( ep.src == pe.src && unify ( P[ep.rule].head, ep.env, P[pe.rule].head, pe.env, false ) ) break;
						}
						if ( !ep.parent ) proof_trace.emplace_front ( r );

					}
				}
			}
		}
	}
	return evidence;
}


int main() {
	evidence_t evidence, cases;
	uint Socrates = mkpred ( "Socrates" ), Man = mkpred ( "Man" ), Mortal = mkpred ( "Mortal" ), Morrtal = mkpred ( "Morrtal" ), Male = mkpred ( "Male" ), _x = mkpred ( "?x" ), _y = mkpred ( "?y" );
	cases[dict["a"]].push_back ( mkpred(mkpred ( dict["a"], vector<uint>{Socrates, Male} )) );
	cases[dict["a"]].push_back ( mkpred(mkpred ( dict["a"], vector<uint>{_x, Mortal} ),vector<uint>{ mkpred ( dict["a"], vector<uint>{_x, Man} )  }) );
	cases[dict["a"]].push_back ( mkpred(mkpred ( dict["a"], vector<uint>{_x, Man} ),vector<uint>{ mkpred ( dict["a"], vector<uint>{_x, Male} )  }) );

	cout << "cases:" << endl << predicate::str() << endl;

	evidence = prove ( mkpred ( dict["a"], mkpred ( _y, Mortal ) ), -1, cases );
	cout << "evidence: " << evidence.size() << " items..." << endl;
	for ( auto e : evidence ) {
		//		cout << "  " << e.first << ":" << endl;
		//		for ( auto ee : e.second ) cout << "    " << ( string ) ee << endl;
		//		cout << endl << "---" << endl;
	}
	cout << "QED!" << endl;
	cout << evidence.size() << endl;
	return 0;
}
