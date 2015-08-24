#include <cstdlib>
#include <vector>
#include <map>
#include <iostream>
#include <set>
#include <cstring>
#include <string>
#include <sstream>
#include <stdexcept>
#include "strings.h"
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;
using std::vector;
using std::map;
using std::wstringstream;
using std::endl;
auto &din = std::wcin;
auto &dout = std::wcout;
typedef std::wstring string;
struct term;
typedef vector<term*> termset;
typedef int resid;
typedef term* termid;
typedef std::map<resid, termid> subs;

string lower ( const string& s_ ) { string s = s_; std::transform ( s.begin(), s.end(), s.begin(), ::towlower ); return s; }
pstring pstr ( const string& s ) { return std::make_shared<string>(s); }
pstring wstrim(string s) { trim(s); return pstr(s); }
pstring wstrim(const wchar_t* s) { return wstrim(string(s)); }
string _gen_bnode_id() { static int id = 0; std::wstringstream ss; ss << "_:b" << id; return ss.str(); }
bool startsWith ( const string& x, const string& y ) { return x.size() >= y.size() && x.substr ( 0, y.size() ) == y; }
resid file_contents_iri, marpa_parser_iri, marpa_parse_iri, logequalTo, lognotEqualTo, rdffirst, rdfrest, A, Dot, rdfsType, GND, rdfssubClassOf, False, rdfnil, rdfsResource, rdfsdomain, implies;

struct term {
	term();
	resid p;
	term *s, *o;
	term(resid _p, term* _s = 0, term* _o = 0) : p(_p), s(_s), o(_o) {}
	~term() { if (body) delete[] body; }
	struct body_t {
		friend term;
		term* t;
		size_t nmatches = 0;
		struct match 	{ termid t; subs s; }* matches = 0;
		~body_t() 	{ if (matches) delete[] matches; }
		match* begin() 	{ return matches; }
		match* end() 	{ return matches ? &matches[nmatches] : 0; }
	private:
		void addmatch(termid t, const subs& s) {
			if (!matches) { *(matches = new match[1]) = { t, s }; return; }
			match* m = new match[1+nmatches];
			memcpy(m, matches, sizeof(match*)*nmatches);
			delete[] matches;
			matches = m;
			matches[nmatches++] = { t, s };
		}
	};
	body_t *it = 0;
	subs ds;
	body_t* begin() 		{ return body; }
	body_t* end() 			{ return body ? &body[nbody] : 0; }
	const body_t* begin() const 	{ return body; }
	const body_t* end() const 	{ return body ? &body[nbody] : 0; }
	size_t szbody() const 		{ return nbody; }
	const body_t& getbody(int n) const { return body[n]; }

	bool match(const subs& s) {
		if (!state) { it = body; state = true; }
		else { ++it; ds.clear(); }
		while (it) {
			if (it && unify(s, it->t, ds)) return state = true;
			else { ds.clear(); ++it; continue; }
		}
		return state = false;
	}

	term* addbody(term* t) {
		if (!body) { (body = new body_t[++nbody])->t = t; return this; }
		body_t *b = new body_t[1+nbody];
		memcpy(b, body, sizeof(body_t*)*nbody);
		delete[] body;
		body = b;
		body[nbody++].t = t;
		return this;
	}

	void trymatch(termset& t) { for (uint b = 0; b < nbody; ++b) for (termid x : t) trymatch(b, x); }

	termid evaluate(const subs& ss) {
		static subs::const_iterator it;
		if (p < 0) return ((it = ss.find(p)) == ss.end()) ? 0 : it->second->p < 0 ? 0 : it->second->evaluate(ss);
		if (!s && !o) return this;
		if (!s || !o) throw 0;
		termid a = s->evaluate(ss), b = o->evaluate(ss);
		return new term(p, a ? a : s, b ? b : o);
	}
	bool unify_ep(const subs& ssub, term* _d, const subs& dsub) {
		return !(!p || !_d || !_d->p) && (p < 0) ? unifvar_ep(ssub, _d, dsub) : unif_ep(ssub, *_d, dsub, !s);
	}
	bool unify(const subs& ssub, term* _d, subs& dsub) {
		return !(!p || !_d || !_d->p) && (p < 0) ? unifvar(ssub, _d, dsub) : unif(ssub, *_d, dsub, !s);
	}
private:
	static termid v; // temp var for unifiers

	void trymatch(uint b, term* t) {
		static subs d;
		if (body[b].t->unify(subs(), t, d)) body[b].addmatch(t, d);
		d.clear();
	}

	bool unifvar(const subs& ssub, term* _d, subs& dsub) {
		if (!_d) return false;
		static subs::const_iterator it;
		if ((v = evaluate(ssub))) return v->unify(ssub, _d, dsub);
		if ((it = dsub.find(p)) != dsub.end() && it->second != _d && it->second->p > 0) return false;
		dsub[p] = _d;
		return true;
	}
	bool unifvar_ep(const subs& ssub, term* _d, const subs& dsub) {
		return _d && ((v = evaluate(ssub)) ? v->unify_ep(ssub, _d, dsub) : true);
	}

	bool unif(const subs& ssub, term& d, subs& dsub, bool pred) {
		if (!d.p) return false;
		if (d.p < 0) {
			if ((v = d.evaluate(dsub))) return unify(ssub, v, dsub);
			dsub.emplace(d.p, evaluate(ssub));
			return true;
		}
		return p == d.p && (pred || s->unify(ssub, d.s, dsub) && o->unify(ssub, d.o, dsub) );
	}

	bool unif_ep(const subs& ssub, term& d, const subs& dsub, bool pred) {
		if (!d.p) return false;
		if (d.p < 0) return ((v = d.evaluate(dsub))) ? unify_ep(ssub, v, dsub) : true;
		return p == d.p && (pred || s->unify_ep(ssub, d.s, dsub) && o->unify_ep(ssub, d.o, dsub));
	}

	body_t *body = 0;
	size_t nbody = 0;
	bool state = false;
};

term* term::v;

string format(const term* t, bool body = true);
string format(const termset& t);

class bidict {
	std::map<resid, string> ip;
	std::map<string, resid> pi;
public:
	resid operator[] ( pstring s ) { return (*this)[*s]; }
	void init() {
		GND = set( L"GND" );
		implies = set(L"=>");
		logequalTo = set( L"log:equalTo");
		lognotEqualTo = set(L"log:notEqualTo");
		rdffirst = set(*RDF_FIRST/*Tpstr(L"rdf:first")*/);
		rdfrest = set(*RDF_REST/*pstr(L"rdf:rest")*/);
		rdfnil = set(*RDF_NIL/*Tpstr(L"rdf:nil")*/);
		A = set(L"a");
		rdfsResource = set(L"rdfs:Resource");
		rdfsdomain = set(L"rdfs:domain");
		//	rdfList = set(L"rdf:List")));
		Dot = set(L".");
		rdfsType = set(L"http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
		rdfssubClassOf = set(L"rdfs:subClassOf");
	}

	resid set ( string v ) {
		if (!v.size()) throw std::runtime_error("bidict::set called with a node containing null value");
		auto it = pi.find ( v );
		if ( it != pi.end() ) return it->second;
		resid k = pi.size() + 1;
		if ( v[0] == L'?' ) k = -k;
		pi[v] = k;
		ip[k] = v;
		return k;
	}

	string operator[] ( resid k ) { return ip[k]; }
	resid operator[] ( string v ) { return set(v); }
	bool has ( resid k ) const { return ip.find ( k ) != ip.end(); }
	bool has ( string v ) const { return pi.find ( v ) != pi.end(); }
} dict;

#define EPARSE(x) throw wruntime_error(string(x) + string(s,0,48));
#define SKIPWS while (iswspace(*s)) ++s
#define RETIF(x) if (*s == x) return 0
#define RETIFN(x) if (*s != x) return 0
#define PROCEED1 while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')')
#define PROCEED PROCEED1 t[pos++] = *s++; t[pos] = 0; pos = 0
#define TILL(x) do { t[pos++] = *s++; } while (x)
class nqparser {
private:
	wchar_t *t;
	const wchar_t *s;
	int pos;
	term* dotterm;
public:
	nqparser() : t(new wchar_t[4096*4096]), dotterm(new term(Dot)) {}
	~nqparser() { delete[] t; }

	bool readcurly(termset& ts) {
		while (iswspace(*s)) ++s;
		if (*s != L'{') return false;
		do { ++s; } while (iswspace(*s));
		if (*s == L'}') ++s;
		ts = (*this)(s, _gen_bnode_id());
		return true;
	}

	term* readlist() {
		if (*s != L'(') return (term*)0;
		static int lastid = 0;
		int lpos = 0, curid = lastid++;
		auto id = [&]() {
			std::wstringstream ss;
			ss << L"_:list" << curid << '.' << lpos;
			return ss.str();
		};
		term *head = new term(Dot), *pn = head;
		++s; while (iswspace(*s)) ++s;
		while (*s != L')') {
			while (iswspace(*s)) ++s;
			if (*s == L')') break;
			++lpos;
			pn->o = readany(true);
			pn->s = dotterm;
			pn = pn->o;
			while (iswspace(*s)) ++s;
			if (*s == L')') pn->o = pn->s = 0;
			if (*s == L'.') while (iswspace(*s++));
			if (*s == L'}') EPARSE(L"expected { inside list: ");
		};
		do { ++s; } while (iswspace(*s));
		return head;
	}
	term* readiri() {
		while (iswspace(*s)) ++s;
		if (*s == L'<') {
			while (*++s != L'>') t[pos++] = *s;
			t[pos] = 0; pos = 0; ++s;
			return new term(dict[t]);
		}
		if (*s == L'=' && *(s+1) == L'>') {
			++++s;
			return new term(implies);
		}
		PROCEED;
		pstring iri = wstrim(t);
		if (lower(*iri) == L"true") return new term(dict[L"true"]);
		if (lower(*iri) == L"false") return new term(dict[L"false"]);
		return new term(dict[iri]);
	}
	term* readbnode() {
		SKIPWS;
		if (*s != L'_' || *(s+1) != L':') return 0;
		PROCEED;
		return new term(dict[wstrim(t)]);
	}

	term* readvar() {
		SKIPWS; RETIFN(L'?'); PROCEED;
		return new term(dict[wstrim(t)]);
	}
	term* readlit() {
		SKIPWS; RETIFN(L'\"');
		++s;
		TILL(!(*(s-1) != L'\\' && *s == L'\"'));
		string dt, lang;
		++s;
		PROCEED1 {
			if (*s == L'^' && *++s == L'^') {
				if (*++s == L'<') {
					++s; while (*s != L'>') dt += *s++;
					++s; break;
				}
			} else if (*s == L'@') { while (!iswspace(*s)) lang += *s++; break; }
			else EPARSE(L"expected langtag or iri:");
		}
		t[pos] = 0; pos = 0;
		string t1 = t;
		boost::replace_all(t1, L"\\\\", L"\\");
		return new term(dict[wstrim(t1)]);//, pstrtrim(dt), pstrtrim(lang);
	}

	term* readany(bool lit = true) {
		term* pn;
		return ((pn = readbnode()) || (pn = readvar()) || (lit && (pn = readlit())) || (pn = readlist()) || (pn = readiri()) ) ? pn : 0;
	}

	termset operator()(const wchar_t* _s, string ctx = L"@default") {
		std::list<std::pair<term*, termset>> preds;
		s = _s;
//		string graph;
		term *subject, *pn;
		termset subjs, ts, heads, objs;
		pos = 0;
		auto pos1 = preds.rbegin();

		while(*s) {
			if (readcurly(subjs)) subject = 0;
			else if (!(subject = readany(false))) EPARSE(L"expected iri or bnode subject:"); // read subject
			do { // read predicates
				while (iswspace(*s) || *s == L';') ++s;
				if (*s == L'.' || *s == L'}') break;
				if ((pn = readiri())) { // read predicate
					preds.emplace_back(pn, termset());
					pos1 = preds.rbegin();
				} else EPARSE(L"expected iri predicate:");
				do { // read objects
					while (iswspace(*s) || *s == L',') ++s;
					if (*s == L'.' || *s == L'}') break;
					if (readcurly(objs)) pos1->second = objs;
					else if ((pn = readany(true))) pos1->second.push_back(pn); // read object
					else EPARSE(L"expected iri or bnode or literal object:");
					SKIPWS;
				} while (*s == L','); // end predicates
				SKIPWS;
			} while (*s == L';'); // end objects
//			if (*s != L'.' && *s != L'}' && *s) { // read graph
//				if (!(pn = readbnode()) && !(pn = readiri()))
//					EPARSE(L"expected iri or bnode graph:");
//				graph = dict[pn->p];
//			} else graph = ctx;
			SKIPWS; while (*s == '.') ++s; SKIPWS;
			if (*s == L')') EPARSE(L"expected ) outside list: ");
			for (auto x : preds)
				for (term* o : x.second)
					if (subject) heads.emplace_back(new term(x.first->p, subject, o));
					else for (term* y : subjs) heads.emplace_back((new term(x.first->p, subject, o))->addbody(y));
			if (*s == L'}') { ++s; return heads; }
			preds.clear();
		}
		return heads;
	}
};

termset readqdb ( std::wistream& is) {
	string s, c;
	nqparser p;
	std::wstringstream ss;
	while (getline(is, s)) {
		trim(s);
		if (s[0] == '#') continue;
		if (startsWith(s, L"fin") && *wstrim(s.c_str() + 3) == L".") break;
		ss << ' ' << s << ' ';
	}
	return p((wchar_t*)ss.str().c_str());
}

string format(const term* t, bool body) {
	if (!t) return L"";
	std::wstringstream ss;
	if (body && t->p == implies) {
		ss << L'{';
		for (auto x : *t) ss << format(x.t) << L';';
		ss << L'}';
		ss << format(t, false);
	}
	else ss << dict[t->p] << L'(' << format(t->s) << L',' << format(t->o) << L')';
	return ss.str();
}

string format(const termset& t) {
	std::wstringstream ss;
	for (auto x : t) ss << format(x, true) << endl;
	return ss.str();
}

int main() {
	dict.init();
	termset kb = readqdb(din);
	dout << format(kb) << endl;
	termset query = readqdb(din);
	dout << format(query) << endl;
	for (term* t : kb) t->trymatch(kb);
	for (term* t : query) t->trymatch(kb);
	subs s;
	for (term* t : query) while (t->match(s)) dout << format(t->it->t) << endl;
	return 0;
}
