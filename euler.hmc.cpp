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
using namespace std;

const uint M1 = 1024 * 1024;
const uint max_predicates = M1, max_rules = M1, max_frames = M1;
const uint GND = INT_MAX;

typedef /*forward_*/vector<class predicate*> predlist;
typedef /*forward_*/vector<class rule*> rulelist;
ostream& operator<< ( ostream&, const predlist& );
ostream& operator<< ( ostream&, const rulelist& );
//ostream& operator<< ( ostream&, const vector<predicate*>& );

template<typename T> void print ( T t ) {
	cout << t << endl;
}

int builtin() { return -1; }

class bidict {
	map<int, string> m1;
	map<string, int> m2;
	//	size_t lastk = 0;
public:
	int set ( const string& v ) {
		if ( v.size() < 1 ) throw 0;
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
		if ( !v.size() ) throw 0;
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
	int p = 0;
	predlist args;
	predicate& init ( int _p = 0, predlist _args = predlist() ) {
		p = _p;
		args = _args;
		return *this;
	}
	bool& null = ( bool& ) p;
	friend ostream& operator<< ( ostream& o, const predicate& p) {
		o << dict[p.p];
		return p.args.empty() ? o : o << p.args;
	}
} *predicates = new predicate[max_predicates];

struct rule {
	predicate* head = 0;
	predlist body;
	rule& init ( predicate* h, predlist b = predlist() ) {
		if (!h) throw 0;
		head = h;
		body = b;
		return *this;
	}
	friend ostream& operator<< ( ostream& o, const rule& r ) {
		if ( r.head ) o << ( *r.head );
		return r.body.empty() ? o : o << r.body;
	}
} *rules = new rule[max_rules];

uint npredicates = 0, nrules = 0, nframes = 0;

typedef map<int, predicate*> subst;
typedef list<pair<rule*, subst>> gnd;
typedef map<int, forward_list<rule*>> evd;

rulelist to_rulelist(const gnd& g) {
	rulelist r;
	for (auto x = g.cbegin(); x != g.cend(); ++x) r.push_back(x->first);
	return r;
}

ostream& operator<< ( ostream& o, const subst& s ) {
	for ( auto x : s ) o << dict[x.first] << " := " << *x.second << endl;
	return o;
}

ostream& operator<< ( ostream& o, const gnd& s ) {
	for ( auto x : s ) o << *x.first << ": " << x.second << endl;
	return o;
}

ostream& operator<< ( ostream& o, const evd& e ) {
	for ( auto x : e ) {
		o << dict[x.first] << " <= ";
		for (auto y : x.second) o << *y << endl;
	}
	return o;
}

class frame {
	frame() {}
public:
	static frame* frames;
	rule* r = 0;
	uint src = 0, ind = 0;
	frame* parent = 0;
	subst s;
	gnd g;
	frame& init( const frame& f) {
		return init(f.r,f.src,f.ind,f.parent,f.s,f.g);
	}
	frame& init ( rule* _r = 0, uint _src = 0, uint _ind = 0, frame* p = 0, subst _s = subst(), gnd _g = gnd() ) {
		if (!_r) throw 0;
		r = _r;  
		src = _src; 
		ind = _ind;
		parent = p; 
		s = _s;
		g = _g;
		return *this;
	}
	friend ostream& operator<< ( ostream& o, const frame& f ) {
		return o << "rule: " << *f.r << " src: " << f.src << " ind: " << f.ind << " parent: ";
		if (f.parent) o << *f.parent;
		else o << "(null)";
		return o << " subst: " << f.s << " gnd: " << f.g;
	}
};
frame *frame::frames = new frame[max_frames];

ostream& operator<< ( ostream& o, const rulelist& l ) {
	if (l.empty()) return o << "[]";
	o << '[';
	for ( auto it = l.cbegin();; ) {
		o << *it;
		if ( ++it == l.end() ) break;
		else o << ',';
	}
	return o << ']';
}

ostream& operator<< ( ostream& o, const predlist& l ) {
	if (l.empty()) return o << "[]";
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
	if ( t.p < 0 ) {
		auto it = sub.find(t.p);
		return it == sub.end() ? 0 : evaluate ( *it->second, sub );
	}
	else if ( t.args.empty() ) return &t;
	else {
		predicate* p;
		predicate& r = predicates[npredicates++].init ( t.p );
		for ( auto x : t.args ) r.args.emplace_back ( ( p = evaluate ( *x, sub ) ) ? p : &predicates[npredicates++].init ( x->p ) );
		return &r;
	}
}

bool unify ( predicate& s, const subst& ssub, predicate& d, subst& dsub, bool f ) {
	predicate* p;
	if ( s.p < 0 ) return ( p = evaluate ( s, ssub ) ) ? unify ( *p, ssub, d, dsub, f ) : true;
	if ( d.p < 0 ) {
		p = evaluate ( d, dsub );
		if ( ( p = evaluate ( d, dsub ) ) ) return unify ( s, ssub, *p, dsub, f );
		else {
			if ( f ) dsub[d.p] = evaluate ( s, ssub );
			return true;
		}
	}
	return equal ( s.args.begin(), s.args.end(), d.args.begin(), d.args.end(), [&ssub, &dsub, &f] ( predicate * p1, predicate * p2 ) {
		return unify ( *p1, ssub, *p2, dsub, f );
	} );
}

predlist to_predlist(const gnd& g) {
//typedef list<pair<rule*, subst>> gnd;
	predlist r;
	for (auto x : g) r.push_back(x.first->head);
	return r;
}

evd prove ( /*rule* goal*/predicate* goal, int maxNumberOfSteps, evd& cases ) {
	deque<frame*> queue;
	queue.emplace_back ( &frame::frames[nframes++].init( &rules[nrules++].init(goal, {goal}) ));
	evd e;
	uint step = 0;

	cout<<"goal: "<<*goal<<endl;
	while ( queue.size() > 0 ) {
		frame& c = *queue.front();
//		cout << c << endl;
		queue.pop_front();
		gnd g ( c.g );
		step++;
		if ( maxNumberOfSteps != -1 && (int)step >= maxNumberOfSteps ) return evd();
		if ( c.ind == c.r->body.size() ) {
			if ( !c.parent ) {
				for ( size_t i = 0; i < c.r->body.size(); ++i ) {
					auto t = evaluate ( *c.r->body[i], c.s ); //evaluate the statement in the enviornment
					if ( e.find ( t->p ) == e.end() ) e[t->p] = {}; //initialize this predicate's evidence list, if necessary
					e[t->p].emplace_front ( &rules[nrules++].init ( t, {&predicates[npredicates++].init ( GND, to_predlist ( c.g ) ) } ) ); //add the evidence for this statement
				}
				continue;
			}
			if ( !c.r->body.empty() ) g.emplace_front ( c.r, c.s );
			frame& r = frame::frames[nframes++].init (
			    &rules[nrules++].init ( c.parent->r->head, c.parent->r->body ),
			    c.parent->src,
			    c.parent->ind,
			    c.parent->parent,
			    subst ( c.parent->s ),
			    g
			);
			unify ( *c.r->head, c.s, *r.r->body[r.ind], r.s, true );
			r.ind++;
			queue.push_back ( &r );
			continue;
		}
		predicate* t = c.r->body[c.ind];
		cout << *t << endl;
		int b = builtin();// ( t, c );
		if ( b == 1 ) {
			g.emplace_front ( &rules[nrules++].init ( evaluate ( *t, c.s ) ), subst() );
			frame& r = frame::frames[nframes++].init(c);
			r.g = gnd();
			r.ind++;
			queue.push_back ( &r );
			continue;
		} else if ( b == 0 ) continue;

		if ( cases.find ( t->p ) == cases.end() ) continue;

		uint src = 0;
//		for ( size_t k = 0; k < cases[t->p].size(); k++ ) {
		for (rule* _rl : cases[t->p]) {
			rule& rl = *_rl;//*cases[t->p][k];
			cout << rl << endl;
			src++;
			gnd g = c.g;
			if ( rl.body.size() == 0 ) g.emplace_back ( &rl, subst() );
			frame& r = frame::frames[nframes++].init ( &rl, src, 0, &c, subst(), g );
			if ( unify ( *t, c.s, *rl.head, r.s, true ) ) {
				frame& ep = c;
				while ( ep.parent ) {
					ep = *ep.parent;
					if ( ep.src == c.src && unify ( *ep.r->head, ep.s, *c.r->head, c.s, false ) ) break;
				}
				if ( !ep.parent ) queue.push_front ( &r );
			}
		}
	}
	return e;
}

predicate* mkpred(string s, const vector<predicate*>& v = vector<predicate*>()) { 
	uint p; 
	p = dict.has(s) ? dict[s] : dict.set(s); 
	return &predicates[npredicates++].init(p, v); 
}

//rule* mkrule(string s) { return &rules[nrules++].init(dict.set(s)); }
rule* mkrule(predicate* p, const vector<predicate*>& v = vector<predicate*>()) { return &rules[nrules++].init(p, v); }

typedef predicate* ppredicate;

int main() {
	dict.set ( "a" );
	evd evidence, cases;
	ppredicate Socrates = mkpred ( "Socrates" ), Man = mkpred ( "Man" ), Mortal = mkpred ( "Mortal" ), Morrtal = mkpred ( "Morrtal" ), Male = mkpred ( "Male" ), _x = mkpred ( "?x" ), _y = mkpred ( "?y" );
	cases[dict["a"]].push_front ( mkrule ( mkpred ( "a", {Socrates, Male} ) ) );
	cases[dict["a"]].push_front ( mkrule ( mkpred ( "a", {_x, Mortal} ), { mkpred ( "a", {_x, Man } )  } ) );
	cases[dict["a"]].push_front ( mkrule ( mkpred ( "a", {_x, Man   } ), { mkpred ( "a", {_x, Male} )  } ) );

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
	evidence = prove ( /*mkrule ( 0, {*/mkpred( "a", { _y, Mortal } )/*} )*/, -1, cases );
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
