#include "prover.h"
#include "misc.h"
#include <iomanip>
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;

bidict& dict = *new bidict;
bool deref = true, shorten = false;
int level = 1;

extern int _indent;
resid file_contents_iri, marpa_parser_iri, marpa_parse_iri, logequalTo, lognotEqualTo, rdffirst, rdfrest, A, Dot, rdfsType, GND, rdfssubClassOf, rdfnil, False, rdfsResource, rdfsdomain;
//resid rdfList, _dlopen, _dlclose, _dlsym, _dlerror, _invoke, rdfnil, False;

void bidict::init() {
#ifdef with_marpa
	file_contents_iri = set(mkiri(pstr(L"http://idni.org/marpa#file_contents")));
	marpa_parser_iri = set(mkiri(pstr(L"http://idni.org/marpa#parser")));
	marpa_parse_iri = set(mkiri(pstr(L"http://idni.org/marpa#parse")));
#endif
	GND = set (mkiri(pstr( L"GND" )));
	logequalTo = set (mkiri(pstr( L"log:equalTo")));
	lognotEqualTo = set (mkiri(pstr(L"log:notEqualTo")));
	rdffirst = set(mkiri(RDF_FIRST/*Tpstr(L"rdf:first")*/));
	rdfrest = set(mkiri(RDF_REST/*pstr(L"rdf:rest")*/));
	rdfnil = set(mkiri(RDF_NIL/*Tpstr(L"rdf:nil")*/));
	A = set(mkiri(pstr(L"a")));
	rdfsResource = set(mkiri(pstr(L"rdfs:Resource")));
	rdfsdomain = set(mkiri(pstr(L"rdfs:domain")));
//	rdfList = set(mkiri(pstr(L"rdf:List")));
	Dot = set(mkiri(pstr(L".")));
	rdfsType = set(mkiri(pstr(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#type")));
	rdfssubClassOf = set(mkiri(pstr(L"rdfs:subClassOf")));
//	_dlopen = set(mkiri(pstr(L"dlfcn:dlopen")));
//	_dlerror = set(mkiri(pstr(L"dlfcn:dlerror")));
//	_dlsym = set(mkiri(pstr(L"dlfcn:dlsym")));
//	_dlclose = set(mkiri(pstr(L"dlfcn:dlclose")));
//	_invoke = set(mkiri(pstr(L"dlfcn:invoke")));
	False = set(mkliteral(pstr(L"false"), XSD_BOOLEAN, 0));
}

void bidict::set ( const std::vector<node>& v ) {
	for ( auto x : v ) set ( x );
}

resid bidict::set ( node v ) {
	if (!v.value) throw std::runtime_error("bidict::set called with a node containing null value");
	auto it = pi.find ( v );
	if ( it != pi.end() )
	{
		assert (v._type == it->first._type);
		return it->second;
	}
	resid k = pi.size() + 1;
	if ( v._type == node::IRI && (*v.value)[0] == L'?' ) k = -k;
	pi[v] = k;
	ip[k] = v;
	return k;
}

node bidict::operator[] ( resid k ) {
//	if (!has(k)) set(::tostr(k));
#ifdef DEBUG
	if (ip.find(k) == ip.end()) throw std::runtime_error("bidict[] called with nonexisting resid");
#endif
//	dout << k << ' ' << ip[k] << endl;
	return ip[k];
}

resid bidict::operator[] ( node v ) {
	return pi[v];
}

bool bidict::has ( resid k ) const {
	return ip.find ( k ) != ip.end();
}

bool bidict::has ( node v ) const {
	return pi.find ( v ) != pi.end();
}

string bidict::tostr() {
	std::wstringstream s;
	for ( auto x : pi ) s << x.first.tostring() << L" := " << x.second << endl;
	return s.str();
}

string dstr ( resid p, bool escape ) {
	if ( !deref ) return *tostr ( p );
	string s = dict[p].tostring();
	if (escape) {
		replace_all(s, L"\\", L"\\\\");
		replace_all(s, L"\"", L"\\\"");
		replace_all(s, L"'", L"\\'");
	}
	if ( !shorten ) return s;
	if ( s.find ( L"#" ) == string::npos ) return s;
	return s.substr ( s.find ( L"#" ), s.size() - s.find ( L"#" ) );
}
bool endsWith ( const string& x, const string& y ) { return x.size() >= y.size() && x.substr ( x.size() - y.size(), y.size() ) == y; } 
bool startsWith ( const string& x, const string& y ) { return x.size() >= y.size() && x.substr ( 0, y.size() ) == y; }

string lower ( const string& s_ ) {
	string s = s_;
	std::transform ( s.begin(), s.end(), s.begin(), ::towlower );
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

string prover::format(termid id, bool json) {
	if (!id) return L"{}";
	if (!id->p) return L"";
	return format(*id,json);
}

string prover::format(term& p, bool json) {
	if (!json) {
		std::wstringstream ss;
		if (level > 100) ss << L" [" <</* id << ':' <<*/ p.p << ']';
		ss << dstr(p.p, false);
		if (p.s) ss << L'('<< prover::format(p.s) << L',' << prover::format(p.o) << L')';
		return ss.str();
	}
	std::wstringstream ss;
	ss << L"{pred:\"" << dstr(p.p, true) << L"\",args:[";
	if (p.s) ss << format (p.s, true) << L",";
	if (p.o) ss << format (p.o, true);
	ss << L"]}";
	return ss.str();
}

string prover::formatp(shared_ptr<proof> p) {
	std::wstringstream ss;
	ss 	<< L"rule:   " << format(p->rule) << endl
		<< L"prev:   " << p->prev << endl
		<< L"subst:  " << formats(p->s) << endl
		<< L"ground: " << endl << formatg(p->g(this)) << endl;
	return ss.str();
}
void prover::printp(shared_ptr<proof> p) {
	dout << KCYN /*<< indent()*/ << L"rule:   " << formatr(p->rule, false) <<endl<<indent();
	if (p->prev) dout << L"prev:   " << p->prev <<endl<<indent()<< L"subst:  ";
	else dout << L"prev:   (null)"<<endl<<indent()<<"subst:  ";
	prints(p->s);
	dout <<endl<<indent()<< L"ground: " << endl;
	++_indent;
	printg(p->g(this));
	--_indent;
	dout << endl << KNRM;
}

string prover::formats(const subs  & s, bool json) {
	if (s.empty()) return L"";
	std::wstringstream ss;
	std::map<string, string> r;
	for (auto x : s)
		r[dstr(x.first)] = format(x.second, json);
	for (auto x = r.rbegin(); x != r.rend(); ++x) ss << x->first << '\\' << x->second << ',';
	return ss.str();
}

void prover::prints(const subs  & s) {
	dout << formats(s, false);
//	for (auto x : s)
//		dout << dstr(x.first) << L" / " << format(x.second) << ' ';
}

string prover::format(termset& l, bool json) {
	std::wstringstream ss;
	auto x = l.begin();
	if (json) ss << L'[';
	while (x != l.end()) {
		ss << format (*x, json);
		if (++x != l.end()) ss << L',';
	}
	if (json) ss << L']';
	return ss.str();
}
/*
void prover::printterm_subs(termid id, const subs  & s) {
	term& p = *id;
	dout << dstr(p.p) << L'(';
	if (p.s) {
		printterm_subs(p.s, s);
		dout << L' ';
	}
	if(s.find(p.p) != s.end()) {
		dout << L" (" << format(s.get(p.p)) << L" )";
	}
	if (p.o) {
		dout << L',';
		printterm_subs(p.o, s);
	}
	dout << L')';
}

void prover::printl_subs(termset& l, const subs  & s) {
	auto x = l.begin();
	while (x != l.end()) {
		printterm_subs(*x, s);
		if (++x != l.end())
			dout << L',';
	}
}

void prover::printr_subs(ruleid r, const subs  & s) {
	printl_subs(kb.body()[r], s);
	dout << L" => ";
	printterm_subs(kb.head()[r], s);
}

string prover::format(const ruleset::conds& c) {
	std::wstringstream ss;
	for (auto& y : c) {
		//ss << format(heads[y]) << ',';
		ss << formatr(y.first) << L':' << formats(y.second);
	}
	return ss.str();
}
string prover::format(const vector<pair<termid, ruleset::conds>>& v, bool r) {
	std::wstringstream ss;
	for (auto& x : v)
		ss << format(x.first, r) << L' ' << format(x.second) << L';';
	return ss.str();
}
*/
string prover::formatr(termid r, bool json) {
	std::wstringstream ss;
	if (!json) {
		ss << L"{ ";
		if (r->szbody())
			for (auto& b : *r)
				ss << format(b.t) << L';';
		ss  << L"} => ";
		if (r->p) ss << format(r);
	 	ss << L".";
		return ss.str();
	}
/*	ss << L"{head:" << format(r,true) << L",body:";
//	for (size_t n = 0; n < kb.bodies().size(); ++n) {
		ss << format(kb.body()[r], true);
//		if (n != (kb.bodies().size()-1)) ss << L',';
//	}
 	ss << L"}";*/
	return ss.str();
}

string prover::formatkb(bool json) {
	std::wstringstream ss;
	if (json) ss << L'[';
	for (uint n = 0; n < kb.size(); ++n) {
		ss << formatr(heads[n], json);
		if (json && n != (kb.size()-1)) ss << L',';
		ss << endl;
	}
	if (json) ss << L']';
	return ss.str();
}

string prover::formatg(const ground& g, bool json) {
	std::wstringstream ss;
	for (auto x : g) {
		ss << formatr(x.first, json) << tab << formats(x.second, json);
		ss << endl;
	}
	return ss.str();
}

void prover::printg(const ground& g) {
	for (auto x : g) 
		dout << indent() << formatr(x.first) << tab << formats(x.second) << endl;
}
#ifdef JSON
pobj term::json(const prover& pr) const {
	pobj r = mk_somap_obj(), l;
	(*r->MAP())[L"pred"] = mk_str_obj(dict[p].tostring());
	(*r->MAP())[L"args"] = l = mk_olist_obj();
	if (s) l->LIST()->push_back(s->json(pr));
	if (o) l->LIST()->push_back(o->json(pr));
	return r;
}
#endif

string prover::fsubs(const prover::ground& g) {
	subs s;
	for (auto x : g)
		if (x.second)
			for (auto y : *x.second)
				//s[y.first] = y.second;
				s.emplace(y.first, y.second);
	return prover::formats(s);
}

void prover::printe() {
	for (auto y : e)
		for (auto x : y.second) {
			dout << indent() << format(x.first) << " under subst: " << fsubs(x.second) << L" <= " << endl;
			TRACE(
				MARPA(if (x.second.size() > 1))
				printg(x.second);
				dout << endl
			);
		}
}

std::list<string>& proc = *new std::list<string>;

string indent() {
	if (!_indent) return string();
	std::wstringstream ss;
//	size_t sz = proc.size();
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
	++_indent;
}

_setproc:: ~_setproc() {
	proc.pop_front();
	--_indent;
}

pstring wstrim(string s) {
	trim(s);
	return pstr(s);
}

pstring pstrtrim ( const string& s ) { string ss = s; trim(ss);return std::make_shared<string> ( ss ); } 
pstring pstrtrim ( const wchar_t* s ) { if (!s) return 0; return pstrtrim ( string(s) ); }
string ws(const std::string& s) { return string(s.begin(), s.end()); }
std::string ws(const string& s) { return std::string(s.begin(), s.end()); }
std::string ws(pstring s) { return ws(*s); }
pstring wstrim(const wchar_t* w) { string s = w; return wstrim(s); }

struct cmpstr { 
	bool operator()(const pstring& x, const pstring& y) const {
		if (!x && !y) return false;
		if (!x && y) return true;
		if (x && !y) return false;
		return *x < *y;
	}
};

pstring pstr ( const string& s ) {
	static std::set<pstring, cmpstr> strings;
	auto ps = std::make_shared<string> ( s );
	auto it = strings.find(ps);
	if (it != strings.end()) return *it;
	strings.insert(ps);
	return ps;
} 
#ifdef JSON
pobj prover::json(termset& ts) const {
	polist_obj l = mk_olist_obj(); 
	for (termid t : ts) l->LIST()->push_back(t->json(*this));
	return l;
}
#endif
/*
pobj prover::json(const subs  & s) const {
	psomap_obj o = mk_somap_obj();
	for (auto x : s) (*o->MAP())[dstr(x.first)] = x.second->json(*this);
	return o;
}

pobj prover::json(ruleid t) const {
	pobj m = mk_somap_obj();
	(*m->MAP())[L"head"] = kb.head()[t]->json(*this);
	(*m->MAP())[L"body"] = json(kb.body()[t]);
	return m;
}

pobj prover::json(const ground& g) const {
	pobj l = mk_olist_obj();
	for (auto x : g) {
		psomap_obj o = mk_somap_obj();
		(*o->MAP())[L"src"] = json(x.first);
		if (x.second) (*o->MAP())[L"env"] = json(*x.second);
		l->LIST()->push_back(o);
	}
	return l;
}

pobj prover::ejson() const {
	pobj o = mk_somap_obj();
	for (auto x : e) {
		polist_obj l = mk_olist_obj();
		for (auto y : x.second) {
			psomap_obj t = mk_somap_obj(), t1;
			(*t->MAP())[L"head"] = y.first->json(*this);
			(*t->MAP())[L"body"] = t1 = mk_somap_obj();
			(*t1->MAP())[L"pred"] = mk_str_obj(L"GND");
			(*t1->MAP())[L"args"] = json(y.second);
			l->LIST()->push_back(t);
		}
		(*o->MAP())[dstr(x.first)] = l;
	}
	return o;
}

string prover::ruleset::format() const {
	setproc(L"ruleset::format");
	std::wstringstream ss;
	ss << L'['<<endl;
	for (auto it = r2id.begin(); it != r2id.end();) {
		ss <<tab<< L'{' << endl <<tab<<tab<<L'\"'<<(it->first ? *dict[it->first].value : L"")<<L"\":[";
		for (auto iit = it->second.begin(); iit != it->second.end();) {
			ss << p->formatr(heads[*iit], true);
			if (++iit != it->second.end()) ss << L',';
			ss << endl;
		}
		ss << L"]}";
		if (++it != r2id.end()) ss << L',';
	}
	ss << L']';
	return ss.str();
}*/
