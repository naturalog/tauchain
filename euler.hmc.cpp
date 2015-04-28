/*
 * euler.cpp
 *
 *  Created on: Apr 28, 2015
 *      Author: troy
 */

#include <forward_list>
#include <deque>
#include <climits>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <map>
#include <list>
#include <vector>
#include <cstdio>
using namespace std;

const uint K1 = 1024, M1 = K1 * K1;
const uint max_predicates = K1, max_rules = K1, max_frames = K1;

typedef /*forward_*/vector<class predicate*> predlist;
typedef /*forward_*/vector<class rule*> rulelist;
ostream& operator<< ( ostream&, const predlist& );
ostream& operator<< ( ostream&, const rulelist& );
//ostream& operator<< ( ostream&, const vector<predicate*>& );

template<typename T> void print ( T t ) {
	cout << t << endl;
}


class bidict {
	map<int, string> m1;
	map<string, int> m2;
	//	size_t lastk = 0;
public:
	int set ( const string& v ) {
		int k = m1.size() + 1 ;//( int ) lastk++;
		if ( v[0] == '?' ) k = -k;
		m1[k] = v;
		m2[v] = k;
		return k;
	}
	const string operator[] ( const int& k ) {
		auto v = m1[k];
		m2[m1[k]];
		return v;
	}
	const int operator[] ( const string& v ) {
		m1[m2[v]];
		return m2[v];
	}
	bool has ( int k ) const {
		return m1.find ( k ) != m1.end();
	}
	bool has ( const string& v ) const {
		return m2.find ( v ) != m2.end();
	}
	string tostr() {
		stringstream s;
		for ( auto x : m1 ) s << x.first << " := " << x.second << endl;
		return s.str();
	}
} dict;

struct predicate {
	int pred = 0;
	predlist args;
	predicate& init ( int _p = 0, predlist _args = predlist() ) {
		pred = _p;
		args = _args;
		return *this;
	}
	bool& null = ( bool& ) pred;
	friend ostream& operator<< ( ostream& o, const predicate& p ) {
		o << dict[p.pred];
		return p.args.empty() ? o : o << p.args;
	}
} *predicates = new predicate[max_predicates];

struct rule {
	predicate* head = 0;
	predlist body;
	rule& init ( predicate* h, predlist b = predlist() ) {
		head = h;
		body = b;
		return *this;
	}
	friend ostream& operator<< ( ostream& o, const rule& r ) {
		if ( r.body.empty() ) o << "{}";
		else o << r.body;
		o << " => ";
		if ( r.head ) return o << ( *r.head );
		else return o << "{}";
	}
} *rules = new rule[max_rules];

uint npredicates = 0, nrules = 0, nframes = 0;

typedef map<int, predicate*> subst;
typedef list<pair<rule*, subst>> ground_t;
typedef map<int, forward_list<pair<rule*, subst>>> evidence_t;
typedef map<int, vector<rule*>> cases_t;

int builtin ( predicate* p ) {
	if ( p && p->pred == dict["GND"] ) return 1;
	return -1;
}

rulelist to_rulelist ( const ground_t& g ) {
	rulelist r;
	for ( auto x = g.cbegin(); x != g.cend(); ++x ) r.push_back ( x->first );
	return r;
}

ostream& operator<< ( ostream& o, const subst& s ) {
	for ( auto x : s ) o << dict[x.first] << " := " << *x.second << endl;
	return o;
}

ostream& operator<< ( ostream& o, const ground_t& s ) {
	for ( auto x : s ) o << *x.first << ": " << x.second << '|';
	return o;
}

ostream& operator<< ( ostream& o, const evidence_t& e ) {
	for ( auto x : e ) {
		o << '{';
		for ( auto y : x.second ) o << *y.first << y.second << " | ";
		o << "} => " << dict[x.first];
		o << endl;
	}
	return o;
}

ostream& operator<< ( ostream& o, const cases_t& e ) {
	for ( auto x : e ) {
		o << dict[x.first] << " <= ";
		for ( auto y : x.second ) o << *y << " | ";
		o << endl;
	}
	return o;
}

struct frame {
	rule* rul = 0;
	uint src = 0, ind = 0;
	frame* parent = 0;
	subst substitution;
	ground_t ground;
	frame& init ( const frame& f );
	frame& init ( rule* _r = 0, uint _src = 0, uint _ind = 0, frame* p = 0, subst _s = subst(), ground_t _g = ground_t() );
	friend ostream& operator<< ( ostream& o, const frame& f ) {
		o << "src: " << f.src << "\tind: " << f.ind << "\tparent: ";
		if ( f.parent ) o << *f.parent;
		else o << "(null)";
		o << "\tsubst: " << f.substitution << "\tgnd: " << f.ground;
		return o << "\trule: " << *f.rul;
	}
} *frames = new frame[max_frames];

frame& frame::init ( const frame& f ) {
	if ( nframes >= max_frames ) throw "Buffer overflow";
	if ( !f.parent ) return init ( f.rul, f.src, f.ind, 0, f.substitution, f.ground );
	return init ( f.rul, f.src, f.ind, &frames[nframes++].init ( *f.parent ), f.substitution, f.ground );
}

frame& frame::init ( rule* _r, uint _src, uint _ind, frame* p, subst _s, ground_t _g ) {
	if ( nframes >= max_frames ) throw "Buffer overflow";
	rul = _r;
	src = _src;
	ind = _ind;
	parent = p;
	substitution = _s;
	ground = _g;
	return *this;
}

void printkb() {
	static bool pause = false;
	cout << endl << "dumping kb with " << npredicates << " predicates, " << nrules << " rules and " << nframes << " frames. " << endl;
	cout << "predicates: " <<  endl;
	for ( uint n = 0; n < npredicates; ++n ) cout << predicates[n] << endl;
	cout << "rules: " << endl;
	for ( uint n = 0; n < nrules; ++n ) cout << rules[n] << endl;
	cout << "frames: " << endl;
	for ( uint n = 0; n < nframes; ++n ) cout << frames[n] << endl;
	if (pause) cout << "type <enter> to continue or <c><enter> to stop pausing...";
	cout << endl;
	if (pause && getchar() == 'c') pause = false;
}

ostream& operator<< ( ostream& o, const rulelist& l ) {
	if ( l.empty() ) return o << "[]";
	o << '[';
	for ( auto it = l.cbegin();; ) {
		o << *it;
		if ( ++it == l.end() ) break;
		else o << ',';
	}
	return o << ']';
}

ostream& operator<< ( ostream& o, const predlist& l ) {
	if ( l.empty() ) return o << "[]";
	o << '[';
	for ( predlist::const_iterator it = l.cbegin();; ) {
		o << **it;
		if ( ++it == l.end() ) break;
		else o << ',';
	}
	return o << ']';
}
/*
    ostream& operator<< ( ostream& o, const vector<predicate*> l ) {
	for ( auto it = l.cbegin();; ) {
		o << *it;
		if ( ++it == l.end() ) break;
		else o << ',';
	}
	return o;
    }
*/

predicate* evaluate ( predicate& t, const subst& sub ) {
	predicate* r;
	if ( t.pred < 0 ) {
		auto it = sub.find ( t.pred );
		r = it == sub.end() ? 0 : evaluate ( *it->second, sub );
	}
	else if ( t.args.empty() ) r = &t;
	else {
		predicate* p;
		r = &predicates[npredicates++].init ( t.pred );
		for ( auto x : t.args ) r->args.emplace_back ( ( p = evaluate ( *x, sub ) ) ? p : &predicates[npredicates++].init ( x->pred ) );
	}
//	cout << "eval ( " << t << '/' << sub << " ) returned ";
//	if (!r) cout << "(null)" << endl;
//	else cout << *r << endl;
	return r;
}

bool unify ( predicate& s, const subst& ssub, predicate& d, subst& dsub, bool f ) {
	predicate* p;
	bool rval;
	if ( s.pred < 0 ) rval = ( p = evaluate ( s, ssub ) ) ? unify ( *p, ssub, d, dsub, f ) : true;
	else if ( d.pred < 0 ) {
		p = evaluate ( d, dsub );
		if ( ( p = evaluate ( d, dsub ) ) ) rval = unify ( s, ssub, *p, dsub, f );
		else {
			/*if ( f )*/ dsub[d.pred] = evaluate ( s, ssub );
			rval = true;
		}
	}
	else 
		rval = (s.pred == d.pred) &&  equal ( s.args.begin(), s.args.end(), d.args.begin(), d.args.end(), [&ssub, &dsub, &f] ( predicate * p1, predicate * p2 ) {
		return unify ( *p1, ssub, *p2, dsub, f );
		});
//	cout << "unification of "<<s<<'/'<<ssub<<" with " << d << '/' << dsub << " returned " << (rval ? "true" : "false" ) << endl;
	return rval;
}

predlist to_predlist ( const ground_t& g ) {
	//typedef list<pair<rule*, subst>> gnd;
	predlist r;
	for ( auto x : g ) r.push_back ( x.first->head );
	return r;
}

evidence_t prove ( /*rule* goal*/predicate* goal, int maxNumberOfSteps, cases_t& cases ) {
	deque<frame*> queue;
	queue.emplace_back ( &frames[nframes++].init ( &rules[nrules++].init ( goal, {goal} ) ) );
	evidence_t evidence;
	uint step = 0;

	cout << "goal: " << *goal << endl;
	while ( !queue.empty() ) {
		frame& current_frame = *queue.front();
//		printkb();
		queue.pop_front();
		ground_t g = current_frame.ground;
		step++;
		if ( maxNumberOfSteps != -1 && ( int ) step >= maxNumberOfSteps ) return evidence_t();
		if ( current_frame.ind >= current_frame.rul->body.size() ) {
			if ( !current_frame.parent ) {
				for ( auto x : current_frame.rul->body ) {
					auto t = evaluate ( *x, current_frame.substitution );
					if ( evidence.find ( t->pred ) == evidence.end() ) evidence[t->pred] = {};
					evidence[t->pred].emplace_front ( &rules[nrules++].init ( t, {&predicates[npredicates++].init ( dict["GND"], to_predlist ( current_frame.ground ) ) } ) , subst() );
				}
				continue;
			}
			if ( !current_frame.rul->body.empty() ) g.emplace_front ( current_frame.rul, current_frame.substitution );
			frame& new_frame = frames[nframes++].init ( *current_frame.parent );
			new_frame.ground = ground_t();
			unify ( *current_frame.rul->head, current_frame.substitution, *new_frame.rul->body[new_frame.ind], new_frame.substitution, true );
			new_frame.ind++;
			queue.push_back ( &new_frame );
			continue;
		}
		predicate* t = current_frame.rul->body[current_frame.ind];
		//cout << *t << endl;
		int b = builtin ( t ); // ( t, c );
		if ( b == 1 ) {
			g.emplace_back ( &rules[nrules++].init ( evaluate ( *t, current_frame.substitution ) ), subst() );
			frame& r = frames[nframes++].init ( current_frame );
			r.ground = ground_t();
			r.ind++;
			queue.push_back ( &r );
			continue;
		} else if ( b == 0 ) continue;

		if ( cases.find ( t->pred ) == cases.end() || cases[t->pred].empty() ) continue;

		uint src = 0;
		//		for ( size_t k = 0; k < cases[t->p].size(); k++ ) {
		for ( rule* _rl : cases[t->pred] ) {
			rule& rl = *_rl;
			src++;
			ground_t ground = current_frame.ground;
			if ( rl.body.empty() ) ground.emplace_back ( &rl, subst() );
			frame& candidate_frame = frames[nframes++].init ( &rl, src, 0, &current_frame, subst(), ground );
			if ( unify ( *t, current_frame.substitution, *rl.head, candidate_frame.substitution, true ) ) {
				frame& ep = current_frame;
				while ( ep.parent ) {
					ep = *ep.parent;
					if ( (ep.src == current_frame.src) && unify ( *ep.rul->head, ep.substitution, *current_frame.rul->head, current_frame.substitution, false ) ) break;
				}
				if ( !ep.parent ) {
//					cout << "pushing frame: " << candidate_frame << endl;
					queue.push_front ( &candidate_frame );
				}
			}
		}
	}
	return evidence;
}

predicate* mkpred ( string s, const vector<predicate*>& v = vector<predicate*>() ) {
	uint p;
	p = dict.has ( s ) ? dict[s] : dict.set ( s );
	return &predicates[npredicates++].init ( p, v );
}

//rule* mkrule(string s) { return &rules[nrules++].init(dict.set(s)); }
rule* mkrule ( predicate* p, const vector<predicate*>& v = vector<predicate*>() ) {
	return &rules[nrules++].init ( p, v );
}

typedef predicate* ppredicate;

int main() {
	dict.set ( "a" );
	dict.set ( "GND" );
	evidence_t evidence;
	cases_t cases;
	ppredicate Socrates = mkpred ( "Socrates" ), Man = mkpred ( "Man" ), Mortal = mkpred ( "Mortal" ), Morrtal = mkpred ( "Morrtal" ), Male = mkpred ( "Male" ), _x = mkpred ( "?x" ), _y = mkpred ( "?y" ), _z = mkpred ( "?z" );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {Socrates, Male} ) ) );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {_x, Mortal} ), { mkpred ( "a", {_x, Man } )  } ) );
	cases[dict["a"]].push_back ( mkrule ( mkpred ( "a", {_x, Man   } ), { mkpred ( "a", {_x, Male} )  } ) );

	cout << "cases:" << endl << cases << endl;
	/*
		cases["a"].push_back ( {{"a", {Socrates, Male}}, {}} );
		cases["a"].push_back ( {{"a", {_x, Mortal}}    , { {"a", {_x, Man}}, } } );
		cases["a"].push_back ( {
			{"a", {_x, Man}},
			{ {"a", {_x, Male}}, }
		} );
		bool p = prove ( pred_t {"a", {_y, Mortal}}, -1, cases, evidence );
	*/
	evidence = prove ( /*mkrule ( 0, {*/mkpred ( "a", { _y, Mortal } ) /*} )*/, -1, cases );
	cout << "evidence: " << evidence.size() << " items..." << endl;
	cout << evidence << endl;
	cout << "QED!" << endl;
	cout << evidence.size() << endl;
	return 0;
}
