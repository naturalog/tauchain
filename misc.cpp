#include "proof.h"
#include "parsers.h"

bidict& dict = *new bidict;
bool deref = true, shorten = false;

bidict::bidict() {
	set ( "GND" );
}

void bidict::set ( const vector<string>& v ) {
	for ( auto x : v ) set ( x );
}

int bidict::set ( const string& v ) {
	auto it = m.right.find ( v );
	if ( it != m.right.end() ) return it->second;
	int k = m.size();
	if ( v[0] == '?' || v[0] == '_' ) k = -k;
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

string dstr ( int p ) {
	if ( !deref ) return tostr ( p );
	string s = dict[p];
	if ( !shorten ) return s;
	if ( s.find ( "#" ) == string::npos ) return s;
	return s.substr ( s.find ( "#" ), s.size() - s.find ( "#" ) );
}

ostream& operator<< ( ostream& o, const predicate& p ) {
	if ( p.args.size() == 2 ) {
		o << dstr ( p.args[0]->pred );
		if ( dict[p.pred] == implication ) o << " <= ";
		else o << ' ' << dstr ( p.pred ) << ' ';
		return o << dstr ( p.args[1]->pred ) << " .";
	}
	o << dstr ( p.pred );
	return p.args.empty() ? o : o << p.args;
}

ostream& operator<< ( ostream& o, const rule& r ) {
	if (!r.body.empty()) {
		o << "{ ";
		for ( auto x : r.body ) o << *x << ' ';
		o << "} => ";
	}
	if ( r.head ) return o << *r.head;
//	else return o << "{ }";
	return o;
}

ostream& operator<< ( ostream& o, const subst& s ) {
	for ( pair<int, const predicate*> x : s ) o << dstr ( x.first ) << " / " << *x.second << "; ";
	return o;
}

ostream& operator<< ( ostream& o, const ground_t& s ) {
	o << endl;
	for ( pair<const rule*, subst> x : s ) o << '\t' << *x.first << " ; " << x.second << endl;
	return o;
}

ostream& operator<< ( ostream& o, const evidence_t& e ) {
	for ( auto x : e ) {
		o << "predicate: " << dstr ( x.first ) << endl << "ground: { ";
		for (auto y : x.second) o << endl << '\t' << *y.first << ' ' << y.second;
		o << '}' << endl;
//		if ( !x.second.empty() )
//			o << ": " << endl << '\t' << *x.second.rbegin()->first << x.second.rbegin()->second;
//			o << endl << '\t' << x.second;
//		o << endl;
	}
	return o;
}

ostream& operator<< ( ostream& o, const cases_t& e ) {
	for ( pair<int, list<const rule*>> x : e ) {
		o << dstr ( x.first ) << ": " << endl;
		for ( const rule* y : x.second ) o << '\t' << *y << endl;
	}
	return o;
}

ostream& operator<< ( ostream& o, const proof& p ) {
	o << "Frame: " << &p << " {" << endl << "\tind: " << p.ind << endl << "\tparent pointer: ";
	if ( p.parent ) o << p.parent;
	else o << "null";
	o << endl << "\tsubst: " << p.sub << endl << "\tgnd: " << p.ground;
	o << "\trule: " << ( *p.rul ) << endl << '}' << endl;
	return o;
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

bool endsWith ( const string& x, const string& y ) {
	return x.size() >= y.size() && x.substr ( x.size() - y.size(), y.size() ) == y;
}

bool startsWith ( const string& x, const string& y ) {
	return x.size() >= y.size() && x.substr ( 0, y.size() ) == y;
}

string lower ( const string& s_ ) {
	string s = s_;
	std::transform ( s.begin(), s.end(), s.begin(), ::tolower );
	return s;
}
