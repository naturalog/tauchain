#include "prover.h"
#include "parsers.h"
#include "misc.h"

bidict& dict = *new bidict;
bool deref = true, shorten = false;
int level = 1;

extern int _indent;

bidict::bidict() {
	GND = set ( L"GND" );
	logequalTo = set ( L"log:equalTo");
	lognotEqualTo = set (L"log:notEqualTo");
	rdffirst = set(L"rdf:first"); 
	rdfrest = set(L"rdf:rest");
	A = set(L"a");
	rdfsResource = set(L"rdfs:Resource"); 
	rdfList = set(L"rdf:List");
	Dot = set(L".");
	rdfsType = set(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
	rdfssubClassOf = set(L"rdfs:subClassOf");
}

void bidict::set ( const std::vector<string>& v ) {
	for ( auto x : v ) set ( x );
}

int bidict::set ( const string& v ) {
	auto it = m.right.find ( v );
	if ( it != m.right.end() ) return it->second;
	int k = m.size() + 1;
	if ( v[0] == L'?'/* || v[0] == '_'*/ ) k = -k;
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
	std::wstringstream s;
	for ( auto x : m.right ) s << x.first << L" := " << x.second << std::endl;
	return s.str();
}

string dstr ( int p ) {
	if ( !deref ) return tostr ( p );
	string s = dict[p];
	if (s == L"GND") throw 0;
	if ( !shorten ) return s;
	if ( s.find ( L"#" ) == string::npos ) return s;
	return s.substr ( s.find ( L"#" ), s.size() - s.find ( L"#" ) );
}
/*
std::wostream& operator<< ( std::wostream& o, const predicate& p ) {
	if ( p.args.size() == 2 ) 
	return o << dstr ( p.args[0]->pred ) << ' ' << dstr ( p.pred ) << ' ' << dstr ( p.args[1]->pred ) << " .";
	o << dstr ( p.pred );
	return p.args.empty() ? o : o << p.args;
}

std::wostream& operator<< ( std::wostream& o, const predlist& l ) {
	if ( l.empty() ) return o << L"[]";
	o << L'[';
	for ( predlist::const_iterator it = l.cbegin();; ) {
		o << **it;
		if ( ++it == l.end() ) break;
		else o << ',';
	}
	return o << L']';
}
*/
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

string KNRM = L"\x1B[0m";
string KRED = L"\x1B[31m";
string KGRN = L"\x1B[32m";
string KYEL = L"\x1B[33m";
string KBLU = L"\x1B[34m";
string KMAG = L"\x1B[35m";
string KCYN = L"\x1B[36m";
string KWHT = L"\x1B[37m";

string prover::format(termid id) {
	if (!id) return L"";
	const term p = get(id);
	std::wstringstream ss;
	if (p.s)
		ss << format (p.s);
	ss << L' ' << dstr(p.p) << L' ';
	if (p.o)
		ss << format (p.o);
	return ss.str();
}

void prover::printp(proof* p) {
	if (!p) return;
	dout << KCYN;
	dout << indent() << L"rule:   " << format(p->rul) <<std::endl<<indent();
	if (p->prev)
		dout << L"prev:   " << p->prev <<std::endl<<indent()<< L"subst:  ";
	else
		dout << L"prev:   (null)"<<std::endl<<indent()<<"subst:  ";
	prints(p->s);
	dout <<std::endl<<indent()<< L"ground: " << std::endl;
	++_indent;
	printg(p->g);
	--_indent;
	dout << L"\n";
	dout << KNRM;
}

//void prover::printq(const queue& q) {
//	for (auto x : q) {
//		printp(x);
//		dout << std::endl;
//	}
//}

void prover::prints(const subst& s) {
	for (auto x : s)
		dout << dstr(x.first) << L" / " << format(x.second) << ' ';
}

string prover::format(const termset& l) {
	std::wstringstream ss;
	auto x = l.begin();
	while (x != l.end()) {
		ss << format (*x);
		if (++x != l.end())
			ss << L',';
	}
	return ss.str();
}

void prover::printterm_substs(termid id, const subst& s) {
	const term p = get(id);
	if (p.s) {
		printterm_substs(p.s, s);
		dout << L' ';
	}
	dout << dstr(p.p);
	if(s.find(p.p) != s.end()) {
		dout << L" (" << format(s.at(p.p)) << L" )";
	}
	if (p.o) {
		dout << L' ';
		printterm_substs(p.o, s);
	}
}

void prover::printl_substs(const termset& l, const subst& s) {
	auto x = l.begin();
	while (x != l.end()) {
		printterm_substs(*x, s);
		if (++x != l.end())
			dout << L',';
	}
}

void prover::printr_substs(int r, const subst& s) {
	printl_substs(kb.body()[r], s);
	dout << L" => ";
	printterm_substs(kb.head()[r], s);
}

string prover::formatr(int r) {
	std::wstringstream ss;
	if (!kb.body()[r].empty()) ss << format(kb.body()[r]) << L" => ";
	if (kb.head()[r]) 	ss << format(kb.head()[r]);
	if (kb.body()[r].empty()) 	ss << L".";
	return ss.str();
}

string prover::formatkb() {
	std::wstringstream ss;
	for (uint n = 0; n < kb.size(); ++n) ss << formatr(n) << std::endl;
	return ss.str();
}

void prover::printg(const ground& g) {
	for (auto x : g) {
		dout << indent();
		printr_substs(x.first, x.second);
		dout << std::endl;
	}
}

void prover::printe() {
	for (auto y : e)
		for (auto x : y.second) {
			dout << indent() << format(x.first) << L":" << std::endl;
			++_indent;
			printg(x.second);
			--_indent;
			dout << std::endl;
		}
}

boost::container::list<string> proc;

string indent() {
	if (!_indent) return string();
	std::wstringstream ss;
	for (auto it = proc.rbegin(); it != proc.rend(); ++it) {
		string str = L"(";
		str += *it;
		str += L") ";
		ss << std::setw(8) << str;
	}
	ss << "    " << std::setw(_indent * 2);
	return ss.str();
}

_setproc:: _setproc(const string& p) {
	proc.push_front(p);
}
_setproc:: ~_setproc() {
	proc.pop_front();
}
