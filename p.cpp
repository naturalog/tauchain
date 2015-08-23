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
string lower ( const string& s_ ) {
	string s = s_;
	std::transform ( s.begin(), s.end(), s.begin(), ::towlower );
	return s;
}
pstring pstr ( const string& s ) {
	return std::make_shared<string>(s);
}
pstring wstrim(string s) {
	trim(s);
	return pstr(s);
}
pstring wstrim(const wchar_t* s) {
	return wstrim(string(s));
}
string _gen_bnode_id() {
	static int id = 0;
	std::wstringstream ss;
	ss << "_:b" << id;
	return ss.str();
}
bool startsWith ( const string& x, const string& y ) { return x.size() >= y.size() && x.substr ( 0, y.size() ) == y; }

struct term {
	struct body_t {
		term* t;
		struct match {
			termid t;
			subs s;
		}* matches = 0;
		match* begin() {
			return matches;
		}
		match* end() {
			return matches ? &matches[nmatches] : 0;
		}
		void addmatch(termid t, const subs& s) {
			if (!matches) {
				*(matches = new match[1]) = { t, s };
				return;
			}
			match* m = new match[1+nmatches];
			memcpy(m, matches, sizeof(match*)*nmatches);
			delete[] matches;
			matches = m;
			matches[nmatches++] = { t, s };
		}
		size_t nmatches = 0;
		~body_t() {
			if (matches) delete[] matches;
		}
	};
	body_t* begin() {
		return body;
	}
	body_t* end() {
		return body ? &body[nbody] : 0;
	}
	const body_t* begin() const {
		return body;
	}
	const body_t* end() const {
		return body ? &body[nbody] : 0;
	}
	term();
	resid p;//, s, o;
	term *s, *o;
	term(resid _p, term* _s = 0, term* _o = 0) : p(_p), s(_s), o(_o) {}
	~term() {
		if (body) delete[] body;
	}
	bool state = 0;
	body_t *it = 0;
	subs ds;
	bool match(const subs& s) {
		if (!state) {
			it = body;
			state = true;
		} else {
			++it;
			ds.clear();
		}
		while (it) {
			if (it && unify(s, it->t, ds))
				return state = true;
			else {
				ds.clear();
				++it;
				continue;
			}
		}
		return state = false;
	}
	void addbody(term* t) {
		if (!body) {
			(body = new body_t[++nbody])->t = t;
			return;
		}
		body_t *b = new body_t[1+nbody];
		memcpy(b, body, sizeof(body_t*)*nbody);
		delete[] body;
		body = b;
		body[nbody++].t = t;
	}
	size_t szbody() const {
		return nbody;
	}
	const body_t& getbody(int n) const {
		return body[n];
	}
	void trymatch(uint b, term* t) {
		static subs d;
		if (body[b].t->unify(subs(), t, d))
			body[b].addmatch(t, d);
		d.clear();
	}
	void trymatch(termset& t) {
		for (uint b = 0; b < nbody; ++b)
			for (termid x : t)
				trymatch(b, x);
	}
	termid evvar(const subs& ss) {
		static subs::const_iterator it;
		return ((it = ss.find(p)) == ss.end()) ? 0 : it->second->p < 0 ? 0 : it->second->evaluate(ss);
	}
	term* ev(const subs& ss) {
		termid a = s->evaluate(ss), b = o->evaluate(ss);
		return new term(p, a ? a : s, b ? b : o);
	}

	termid evpred(const subs&) {
		return this;
	}

	termid evaluate(const subs& ss) {
		if (p < 0) return evvar(ss);
		if (!s && !o) return evpred(ss);
		if (!s || !o) throw 0;
		return ev(ss);
	}

#define UNIFVAR(x) { \
		if (!_d) return false; \
		static termid v; \
		if ((v = evaluate(ssub))) \
			return v->x(ssub, _d, dsub); \
		static subs::const_iterator it; \
		if ((it = dsub.find(p)) != dsub.end() && it->second != _d && it->second->p > 0) return false; \
		dsub[p] = _d; \
		return true; \
	}
#define UNIFVAREP(x) { \
		if (!_d) return false; \
		static termid v; \
		if ((v = evaluate(ssub))) return v->x(ssub, _d, dsub); \
		return true; \
	}
	bool unifvar(const subs& ssub, term* _d, subs& dsub) UNIFVAR(unify)
	bool unifvar_ep(const subs& ssub, term* _d, const subs& dsub) UNIFVAREP(unify_ep)

	bool unif(const subs& ssub, term* _d, subs& dsub) {
		if (!_d) return false;
		static termid v;
		term& d = *_d;
		if (!d.s) return false;
		if (d.p < 0) {
			if ((v = d.evaluate(dsub))) return unify(ssub, v, dsub);
//		if ((v = d.evaluate(ssub))) return unify(ssub, v, dsub);
			dsub.emplace(d.p, evaluate(ssub));
			if (dsub[d.p]->s) throw 0;
			return true;
		}
		return p == d.p && s->unify(ssub, d.s, dsub) && o->unify(ssub, d.o, dsub);
	}

	bool unifpred(const subs& ssub, term* _d, subs& dsub) {
		if (!_d) return false;
		static termid v;
		term& d = *_d;
		if (d.s) return false;
		if (d.p < 0) {
			if ((v = d.evaluate(dsub))) return unify(ssub, v, dsub);
			dsub.emplace(d.p, evaluate(ssub));
			if (dsub[d.p]->s) throw 0;
			return true;
		}
		return p == d.p;
	}

	bool unif_ep(const subs& ssub, term* _d, const subs& dsub) {
		if (!_d) return false;
		static termid v;
		term& d = *_d;
		if (!d.s) return false;
		if (d.p < 0) {
			if ((v = d.evaluate(dsub))) return unify_ep(ssub, v, dsub);
			return true;
		}
		return p == d.p && s->unify_ep(ssub, d.s, dsub) && o->unify_ep(ssub, d.o, dsub);
	}

	bool unifpred_ep(const subs& ssub, term* _d, const subs& dsub) {
		if (!p) return false;
		if (!_d) return false;
		static termid v;
		term& d = *_d;
		if (d.s) return false;
		if (d.p < 0) {
			if ((v = d.evaluate(dsub))) return unify_ep(ssub, v, dsub);
			return true;
		}
		return p == d.p;
	}
	bool unify_ep(const subs& ssub, term* _d, const subs& dsub) {
		if (!p) return false;
		if (p < 0) return unifvar_ep(ssub, _d, dsub);
		if (!s) return unifpred_ep(ssub, _d, dsub);
		return unif_ep(ssub, _d, dsub);
	}
	bool unify(const subs& ssub, term* _d, subs& dsub) {
		bool r;
		if (!p) return false;
		if (p < 0) r = unifvar(ssub, _d, dsub);
		else if (!s) r = unifpred(ssub, _d, dsub);
		else r = unif(ssub, _d, dsub);
		return r;
	}
private:
	body_t *body = 0;
	size_t nbody = 0;
};
string format(const term* t);
string format(const termset& t);

resid file_contents_iri, marpa_parser_iri, marpa_parse_iri, logequalTo, lognotEqualTo, rdffirst, rdfrest, A, Dot, rdfsType, GND, rdfssubClassOf, False, rdfnil, rdfsResource, rdfsdomain;

class bidict {
	std::map<resid, string> ip;
	std::map<string, resid> pi;
public:
	resid operator[] ( pstring s ) {
		return (*this)[*s];
	}
	void init() {
#ifdef with_marpa
		file_contents_iri = set(L"http://idni.org/marpa#file_contents");
		marpa_parser_iri = set(L"http://idni.org/marpa#parser");
		marpa_parse_iri = set(L"http://idni.org/marpa#parse");
#endif
		GND = set( L"GND" );
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
		//	_dlopen = set(L"dlfcn:dlopen")));
		//	_dlerror = set(L"dlfcn:dlerror")));
		//	_dlsym = set(L"dlfcn:dlsym")));
		//	_dlclose = set(L"dlfcn:dlclose")));
		//	_invoke = set(L"dlfcn:invoke")));
		//	False = set(new term(dict[L"false"), XSD_BOOLEAN, 0));
	}

	resid set ( string v ) {
		if (!v.size()) throw std::runtime_error("bidict::set called with a node containing null value");
		auto it = pi.find ( v );
		if ( it != pi.end() )
			return it->second;
		resid k = pi.size() + 1;
		if ( v[0] == L'?' ) k = -k;
		pi[v] = k;
		ip[k] = v;
		return k;
	}

	string operator[] ( resid k ) {
		return ip[k];
	}
	resid operator[] ( string v ) {
		return set(v);
	}
	bool has ( resid k ) const {
		return ip.find ( k ) != ip.end();
	}
	bool has ( string v ) const {
		return pi.find ( v ) != pi.end();
	}
} dict;

class nqparser {
private:
	wchar_t *t;
	const wchar_t *s;
	int pos;
	term* dotterm;
public:
	nqparser() : t(new wchar_t[4096*4096]), dotterm(new term(Dot)) {}
	~nqparser() {
		delete[] t;
	}

	bool readcurly(termset& ts) {
		while (iswspace(*s)) ++s;
		if (*s != L'{') return false;
		++s;
		while (iswspace(*s)) ++s;
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
		term* head = new term(Dot);
		term* pn = head;
		++s;
		while (iswspace(*s)) ++s;
		while (*s != L')') {
			while (iswspace(*s)) ++s;
			if (*s == L')') break;
//		if (!(pn = readany(true)))
//			throw wruntime_error(string(L"expected iri or bnode or list in list: ") + string(s,0,48));
//		lists.emplace_back(cons, mkiri(RDF_FIRST), pn);
//		qlists[head].push_back(pn);
			++lpos;
			pn->o = readany(true);
			pn->s = dotterm;
			pn = pn->o;
			while (iswspace(*s)) ++s;
			if (*s == L')') pn->o = pn->s = 0;//lists.emplace_back(cons, mkiri(RDF_REST), mkiri(RDF_NIL));
//		else lists.emplace_back(cons, mkiri(RDF_REST), mkbnode(pstr(id())));
			if (*s == L'.') while (iswspace(*s++));
			if (*s == L'}') throw wruntime_error(string(L"expected { inside list: ") + string(s,0,48));
		};
		do {
			++s;
		} while (iswspace(*s));
		return head;
	}

	term* readiri() {
		while (iswspace(*s)) ++s;
		if (*s == L'<') {
			while (*++s != L'>') t[pos++] = *s;
			t[pos] = 0;
			pos = 0;
			++s;
			return new term(dict[t]);
		}
		if (*s == L'=' && *(s+1) == L'>') {
			++++s;
			return new term(dict[*pimplication]);
		}
		while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') t[pos++] = *s++;
		t[pos] = 0;
		pos = 0;
		pstring iri = wstrim(t);
		if (lower(*iri) == L"true")
			return new term(dict[L"true"]);
		if (lower(*iri) == L"false")
			return new term(dict[L"false"]);
		return new term(dict[iri]);
	}

	term* readbnode() {
		while (iswspace(*s)) ++s;
		if (*s != L'_' || *(s+1) != L':') return 0;
		while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') t[pos++] = *s++;
		t[pos] = 0;
		pos = 0;
		return new term(dict[wstrim(t)]);
	}

	term* readvar() {
		while (iswspace(*s)) ++s;
		if (*s != L'?') return 0;
		while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') t[pos++] = *s++;
		t[pos] = 0;
		pos = 0;
		return new term(dict[wstrim(t)]);
	}

	term* readlit() {
		while (iswspace(*s)) ++s;
		if (*s != L'\"') return 0;
		++s;
		do {
			t[pos++] = *s++;
		} while (!(*(s-1) != L'\\' && *s == L'\"'));
		string dt, lang;
		++s;
		while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')') {
			if (*s == L'^' && *++s == L'^') {
				if (*++s == L'<') {
					++s;
					while (*s != L'>') dt += *s++;
					++s;
					break;
				}
			} else if (*s == L'@') {
				while (!iswspace(*s)) lang += *s++;
				break;
			} else throw wruntime_error(string(L"expected langtag or iri:") + string(s,0,48));
		}
		t[pos] = 0;
		pos = 0;
		string t1 = t;
		boost::replace_all(t1, L"\\\\", L"\\");
		return new term(dict[wstrim(t1)]);//, pstrtrim(dt), pstrtrim(lang);
	}

	term* readany(bool lit = true) {
		term* pn;
		termset ts;
		if (!(pn = readbnode()) && !(pn = readvar()) && (!lit || !(pn = readlit())) && !(pn = readlist()) && !readcurly(ts) && !(pn = readiri()) )
			return (term*)0;
		return pn;
	}

	termset operator()(const wchar_t* _s, string ctx = L"@default") {
		std::list<std::pair<term*, termset>> preds;
		s = _s;
		string graph;
		term *subject, *pn;
		termset ts, heads;
		pos = 0;
		auto pos1 = preds.rbegin();

		while(*s) {
			if (!(subject = readany(false)))
				throw wruntime_error(string(L"expected iri or bnode subject:") + string(s,0,48));
			do {
				while (iswspace(*s) || *s == L';') ++s;
				if (*s == L'.' || *s == L'}') break;
				if ((pn = readiri()) || (readcurly(ts))) {
					preds.emplace_back(pn, termset());
					pos1 = preds.rbegin();
				} else throw wruntime_error(string(L"expected iri predicate:") + string(s,0,48));
				do {
					while (iswspace(*s) || *s == L',') ++s;
					if (*s == L'.' || *s == L'}') break;
					if ((pn = readany(true))) {
						pos1->second.push_back(pn);
					} else throw wruntime_error(string(L"expected iri or bnode or literal object:") + string(s,0,48));
					while (iswspace(*s)) ++s;
				} while (*s == L',');
				while (iswspace(*s)) ++s;
			} while (*s == L';');
			if (*s != L'.' && *s != L'}' && *s) {
				if (!(pn = readbnode()) && !(pn = readiri()))
					throw wruntime_error(string(L"expected iri or bnode graph:") + string(s,0,48));
				graph = dict[pn->p];
			} else
				graph = ctx;
			while (iswspace(*s)) ++s;
			while (*s == '.') ++s;
			while (iswspace(*s)) ++s;
			if (*s == L'}') {
				++s;
				return heads;
			}
			if (*s == L')') throw wruntime_error(string(L"expected ) outside list: ") + string(s,0,48));
		}
		for (auto x : preds)
			for (term* o : x.second)
				heads.emplace_back(new term(x.first->p, subject, o));
		preds.clear();
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

string format(const term* t) {
	if (!t) return L"";
	std::wstringstream ss;
	ss << dict[t->p] << L'(' << format(t->s) << L',' << format(t->o) << L')';
	return ss.str();
}

string format(const termset& t) {
	std::wstringstream ss;
	for (auto x : t) ss << format(x) << endl;
	return ss.str();
}

int main() {
	dict.init();
	dout << format(readqdb(din)) << endl;
	return 0;
}
