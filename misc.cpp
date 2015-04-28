#include "reasoner.h"

extern bidict dict;

int bidict::set ( const string& v ) {
	if ( m2.find ( v ) != m2.end() ) return m2[v];
	int k = m1.size();
	if ( v[0] == '?' ) k = -k;
	m1[k] = v;
	m2[v] = k;
	return k;
}

const string bidict::operator[] ( const int& k ) {
	auto v = m1[k];
	m2[m1[k]];
	return v;
}

int bidict::operator[] ( const string& v ) {
	m1[m2[v]];
	return m2[v];
}

bool bidict::has ( int k ) const {
	return m1.find ( k ) != m1.end();
}

bool bidict::has ( const string& v ) const {
	return m2.find ( v ) != m2.end();
}

string bidict::tostr() {
	stringstream s;
	for ( auto x : m1 ) s << x.first << " := " << x.second << endl;
	return s.str();
}

predicate& predicate::init ( int _p, predlist _args ) {
	pred = _p;
	args = _args;
	return *this;
}

ostream& operator<< ( ostream& o, const predicate& p ) {
	if ( deref ) o << dict[p.pred];
	else o << p.pred;
	return p.args.empty() ? o : o << p.args;
}

ostream& operator<< ( ostream& o, const rule& r ) {
	if ( r.body.empty() ) o << "{}";
	else o << r.body;
	o << " => ";
	if ( r.head ) return o << ( *r.head );
	else return o << "{}";
}

ostream& operator<< ( ostream& o, const subst& s ) {
	for ( auto x : s ) {
		if ( deref ) o << dict[x.first];
		else o << x.first;
		o << " := " << *x.second << endl;
	}
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
		o << "} => ";
		if ( deref ) o << dict[x.first];
		else o << x.first;
		o << endl;
	}
	return o;
}

ostream& operator<< ( ostream& o, const cases_t& e ) {
	for ( auto x : e ) {
		o << '[';
		for ( auto y : x.second ) o << *y << " | ";
		o << ']';
		o << " => ";
		if ( deref ) o << dict[x.first];
		else o << x.first;
		o << endl;
	}
	return o;
}

ostream& operator<< ( ostream& o, const frame& f ) {
	o << "src: " << f.src << "\tind: " << f.ind << "\tparent: ";
	if ( f.parent ) o << *f.parent;
	else o << "(null)";
	o << "\tsubst: " << f.substitution << "\tgnd: " << f.ground;
	return o << "\trule: " << *f.rul;
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

