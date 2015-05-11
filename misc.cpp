#include "prover.h"
#include "parsers.h"
#include "misc.h"

bidict& dict = *new bidict;
bool deref = true, shorten = false;

bidict::bidict() {
	GND = set ( "GND" );
	logequalTo = set ( "log:equalTo");
	lognotEqualTo = set ("log:notEqualTo");
	rdffirst = set("rdf:first"); 
	rdfrest = set("rdf:rest");
	A = set("a");
	rdfsResource = set("rdfs:Resource"); 
	rdfList = set("rdf:List");
	Dot = set(".");
}

void bidict::set ( const vector<string>& v ) {
	for ( auto x : v ) set ( x );
}

int bidict::set ( const string& v ) {
	auto it = m.right.find ( v );
	if ( it != m.right.end() ) return it->second;
	int k = m.size();
	if ( v[0] == '?'/* || v[0] == '_'*/ ) k = -k;
	m.insert ( bm::value_type ( k, v ) );
	return k;
}

const string bidict::operator[] ( int k ) {
	if (!has(k)) set(::tostr(k));
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

string dstr ( int p ) {
	if ( !deref ) return tostr ( p );
	string s = dict[p];
	if ( !shorten ) return s;
	if ( s.find ( "#" ) == string::npos ) return s;
	return s.substr ( s.find ( "#" ), s.size() - s.find ( "#" ) );
}

ostream& operator<< ( ostream& o, const predicate& p ) {
	if ( p.args.size() == 2 ) 
	return o << dstr ( p.args[0]->pred ) << ' ' << dstr ( p.pred ) << ' ' << dstr ( p.args[1]->pred ) << " .";
	o << dstr ( p.pred );
	return p.args.empty() ? o : o << p.args;
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
