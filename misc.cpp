#include "reasoner.h"

bidict& dict = *new bidict;

bidict::bidict() {
}

void bidict::set ( const vector<string>& v ) {
	for ( auto x : v ) set ( x );
}

int bidict::set ( const string& v ) {
	auto it = m.right.find ( v );
	if ( it != m.right.end() ) return it->second;
	int k = m.size();
	if ( v[0] == '?' ) k = -k;
	m.insert ( bm::value_type ( k, v ) );
	return k;
}

const string bidict::operator[] ( const int& k ) {
	return m.left.find ( k )->second;
}

int bidict::operator[] ( const string& v ) {
	return m.right.find ( v )->second;
}

bool bidict::has ( int k ) const {
	return m.left.find ( k ) != m.left.end();
}

bool bidict::has ( const string& v ) const {
	return m.right.find ( v ) != m.right.end();
}

string bidict::tostr() {
	stringstream s;
	for ( auto x : m.right ) s << x.first << " := " << x.second << endl;
	return s.str();
}

predicate& predicate::init ( int _p, predlist _args ) {
	pred = _p;
	args = _args;
	return *this;
}

ostream& operator<< ( ostream& o, const predicate& p ) {
	if ( p.args.size() == 2 ) {
		o << "{ ";
		if ( deref ) o << dict[p.args[0]->pred];
		else o << p.args[0]->pred;
		if ( dict[p.pred] == implication ) o << " <= ";
		else {
			o << ' ';
			if ( deref ) o << dict[p.pred];
			else o << p.pred;
			o << ' ';
		}
		return ( deref ? o << dict[p.args[1]->pred] : o << p.args[1]->pred ) << " . } ";
	}
	if ( deref ) o << dict[p.pred];
	else o << p.pred;
	return p.args.empty() ? o : o << p.args;
}

ostream& operator<< ( ostream& o, const rule& r ) {
	o << "{ ";
	for ( auto x : r.body ) o << *x << ' ';
	o << "} => ";
	if ( r.head ) return o << *r.head;
	else return o << "{ }";
	//	if ( r.body.empty() ) o << "{}";
	//	else o << r.body;
	//	o << " => ";
	//	return r.head ? o << ( *r.head ) : o << "{}";
}

ostream& operator<< ( ostream& o, const subst& s ) {
	for ( pair<int, predicate*> x : s )
		( deref ? o << dict[x.first] : o << x.first ) << " / " << *x.second << "; ";
	return o;
}

ostream& operator<< ( ostream& o, const ground_t& s ) {
	for ( pair<rule*, subst> x : s )
		o << *x.first << " ( " << x.second << " ) ";
	return o;
}

ostream& operator<< ( ostream& o, const evidence_t& e ) {
	for ( pair<int, list<pair<rule*, ground_t>>> x : e ) {
		( deref ? o << dict[x.first] : o << x.first ) << ':' << endl;
		for ( pair<rule*, ground_t> y : x.second )
			o << '\t' << *y.first << y.second << endl;
	}
	return o;
}

ostream& operator<< ( ostream& o, const cases_t& e ) {
	for ( pair<int, list<rule*>> x : e ) {
		( deref ? o << dict[x.first] : o << x.first ) << "( " << endl;
		for ( rule* y : x.second ) o << '\t' << *y << endl;
		o << " ) " << endl;
	}
	return o;
}

ostream& operator<< ( ostream& o, const frame& f ) {
	o << "src: " << f.src << "\tind: " << f.ind << "\tparent: ";
	if ( f.parent ) o << "(" << *f.parent << ")";
	else o << "(null)";
	o << "\tsubst:( " << f.substitution << ")\tgnd: (" << f.ground;
	return o << ")\trule: " << *f.rul;
}

ostream& operator<< ( ostream& o, const rulelist& l ) {
	if ( l.empty() ) return o << "[]";
	o << '[';
	for ( auto it = l.cbegin();; ) {
		o << *it;
		if ( ++it == l.end() ) break;
		else o << ',';
	}
	return o << ']' << endl;
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

void menu() {
	/*	 static map<char, string> menu1 = {
			{'d', "print dict"}
			};
		cout << "now what?" << endl;
		for (auto x : menu1) cout << x.first << ":\t" << x.second << endl;
	*/
}
