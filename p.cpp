//#include <cstdlib>
#include <vector>
#include <map>
#include <iostream>
#include <set>
#include <cstring>
#include <string>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
using namespace boost::algorithm;
/*using std::std::vector;
using std::std::map;
using std::wstringstream;
using std::std::endl;
using boost::shared_ptr;*/
//using namespace std;
//using namespace boost;
std::wistream &din = std::wcin;
std::wostream &dout = std::wcout;
typedef std::wstring string;
struct term;
typedef std::vector<term*> termset;
typedef int resid;
typedef std::map<resid, term*> subs;
typedef boost::shared_ptr<string> pstring;
string lower ( const string& s_ ) { string s = s_; std::transform ( s.begin(), s.end(), s.begin(), ::towlower ); return s; }
pstring pstr ( const string& s ) { return pstring(new string(s)); }
pstring wstrim(string s) { trim(s); return pstr(s); }
pstring wstrim(const wchar_t* s) { return wstrim(string(s)); }
string format(const term* t, bool body = false);
string format(const termset& t, int dep = 0);
bool startsWith ( const string& x, const string& y ) { return x.size() >= y.size() && x.substr ( 0, y.size() ) == y; }
resid file_contents_iri, marpa_parser_iri, marpa_parse_iri, logequalTo, lognotEqualTo, rdffirst, rdfrest, A, Dot, rdfsType, GND, rdfssubClassOf, False, rdfnil, rdfsResource, rdfsdomain, implies;
class wruntime_error : public std::exception {
	string msg;
public:
	wruntime_error(string s) : msg(s){}
	virtual const char* what() const _GLIBCXX_USE_NOEXCEPT {
		return std::string(msg.begin(), msg.end()).c_str();
	}
	virtual ~wruntime_error() _GLIBCXX_USE_NOEXCEPT {}
};

struct term {
	term() : p(0), s(0), o(0) { throw 0; }
	term(termset& kb, termset& query) : p(0), s(0), o(0) { addbody(query); trymatch(kb); }
	term(resid _p, term* _s = 0, term* _o = 0) : p(_p), s(_s), o(_o) { if (!p) throw 0;  }

	resid p;
	term *s, *o;
	struct body_t {
		friend struct term;
		term* t;
		bool state;
		body_t(term* _t) : t(_t), state(false) {}
		struct match { match(term* _t) : t(_t) {} term* t; };
		typedef std::vector<match*> mvec;
		mvec matches;
		mvec::iterator begin() 	{ return matches.begin(); }
		mvec::iterator end() 	{ return matches.end(); }
		mvec::iterator it;
		subs ds;
		bool operator()(const subs& s) {
			if (!state) { it = begin(); state = true; }
			else { ++it; ds.clear(); }
			while (it != end()) {
				dout << "matching " << format(t, true) << " with " << format((*it)->t, true) << "... ";
				if (it != end() && t->unify(s, (*it)->t, ds)) { dout << " passed" << std::endl; return state = true; }
				else { ds.clear(); ++it; dout << " failed" << std::endl; continue; }
			}
			return state = false;
		}
	private:
		void addmatch(term* x) {
			dout << "added match " << format(x) << " to " << format(t) << std::endl;
			matches.push_back(new match(x));
		}
	};
	typedef std::vector<body_t*> bvec;
	bvec body;
	typedef bvec::iterator bvecit;
	typedef bvec::const_iterator bveccit;
	bvecit begin()	 	{ return body.begin(); }
	bvecit end() 		{ return body.end(); }
	bveccit begin() const 	{ return body.begin(); }
	bveccit end() const 	{ return body.end(); }
//	size_t szbody() const 		{ return body.size(); }
//	const body_t& getbody(int n) const { return *body[n]; }

	term* addbody(const termset& t) { for (termset::const_iterator it = t.begin(); it != t.end(); ++it) addbody(*it); return this; }
	term* addbody(term* t) {
		dout << "added body " << format(t) << " to " << format(this) << std::endl;
		body.push_back(new body_t(t));
		return this;
	}

	void trymatch(termset& t) {
		for (bvecit b = begin(); b != end(); ++b)
			for (termset::iterator it = t.begin(); it != t.end(); ++it)
				trymatch(**b, *it);
	}

	term* evaluate(const subs& ss) {
		static subs::const_iterator it;
		if (p < 0) return ((it = ss.find(p)) == ss.end()) ? 0 : it->second->p < 0 ? 0 : it->second->evaluate(ss);
		if (!s && !o) return this;
		if (!s || !o) throw 0;
		term *a = s->evaluate(ss), *b = o->evaluate(ss);
		return new term(p, a ? a : s, b ? b : o);
	}
	bool unify_ep(const subs& ssub, term* _d, const subs& dsub) {
		return (!(!p || !_d || !_d->p)) && ((p < 0) ? unifvar_ep(ssub, _d, dsub) : unif_ep(ssub, *_d, dsub, !s));
	}
	bool unify(const subs& ssub, term* _d, subs& dsub) {
		return (!(!p || !_d || !_d->p)) && ((p < 0) ? unifvar(ssub, _d, dsub) : unif(ssub, *_d, dsub, !s));
	}
private:
	static term* v; // temp var for unifiers

	void trymatch(body_t& b, term* t) {
		if (t == this) return; 
	//	b.addmatch(t, subs()); return;
		dout << "trying to match " << format(b.t) << " with " << format(t) << std::endl;
		static subs d;
		if (b.t->unify(subs(), t, d)) b.addmatch(t);
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
		if (!d.p || d.p == implies) return false;
		if (d.p < 0) {
			if ((v = d.evaluate(dsub))) return unify(ssub, v, dsub);
			dsub[d.p] = evaluate(ssub);
			return true;
		}
		return p == d.p && (pred || (s->unify(ssub, d.s, dsub) && o->unify(ssub, d.o, dsub) ));
	}

	bool unif_ep(const subs& ssub, term& d, const subs& dsub, bool pred) {
		if (!d.p || d.p == implies) return false;
		if (d.p < 0) return ((v = d.evaluate(dsub))) ? unify_ep(ssub, v, dsub) : true;
		return p == d.p && (pred || (s->unify_ep(ssub, d.s, dsub) && o->unify_ep(ssub, d.o, dsub)));
	}
};

term* term::v;

class bidict {
	std::map<resid, string> ip;
	std::map<string, resid> pi;
public:
	resid operator[] ( pstring s ) { return (*this)[*s]; }
	void init() {
		GND = set( L"GND" );
		implies = set(L"=>");
		Dot = set(L".");
	}

	resid set ( string v ) {
		if (!v.size()) throw std::runtime_error("bidict::set called with a node containing null value");
		std::map<string, resid>::iterator it = pi.find ( v );
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
public:
	nqparser() : t(new wchar_t[4096*4096]) {}
	~nqparser() { delete[] t; }

	bool readcurly(termset& ts) {
		while (iswspace(*s)) ++s;
		if (*s != L'{') return false;
		ts.clear();
		do { ++s; } while (iswspace(*s));
		if (*s == L'}') ++s;
		else ts = (*this)(s);
		return true;
	}

	term* readlist() {
		if (*s != L'(') return (term*)0;
		++s; 
		term *head = new term(Dot), *pn = head;
		while (*s != L')') {
			SKIPWS;
			if (*s == L')') break;
			if (!(pn->s = readany(true)))
				EPARSE(L"couldn't read next list item: ");
			SKIPWS;
			pn = pn->o = new term(Dot);
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

	term* readbnode() { SKIPWS; if (*s != L'_' || *(s+1) != L':') return 0; PROCEED; return new term(dict[wstrim(t)]); } 
	term* readvar() { SKIPWS; RETIFN(L'?'); PROCEED; return new term(dict[wstrim(t)]); }

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

	void addhead(termset& ts, term* t) {
		if (!t) throw 0;
		if (!t->p) throw 0;
		if (!t->s != !t->p) throw 0;
		ts.push_back(t);
	}

	termset operator()(const wchar_t* _s) {
		typedef std::list<std::pair<term*, termset> > preds_t;
		preds_t preds;
		s = _s;
//		string graph;
		term *subject, *pn;
		termset subjs, objs, heads;
		pos = 0;
		preds_t::reverse_iterator pos1 = preds.rbegin();

		while(*s) {
			if (readcurly(subjs)) subject = 0;
			else if (!(subject = readany(false))) EPARSE(L"expected iri or bnode subject:"); // read subject
			do { // read predicates
				while (iswspace(*s) || *s == L';') ++s;
				if (*s == L'.' || *s == L'}') break;
				if ((pn = readiri())) { // read predicate
					preds.push_back(std::make_pair(pn, termset()));
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
			if (*s != L'.' && *s != L'}' && *s) { // read graph
				if (!(pn = readbnode()) && !(pn = readiri()))
					EPARSE(L"expected iri or bnode graph:");
//				graph = dict[pn->p];
			} //else graph = ctx;
			SKIPWS; while (*s == '.') ++s; SKIPWS;
			if (*s == L')') EPARSE(L"expected ) outside list: ");
			if (subject)
				for (preds_t::const_iterator x = preds.begin(); x != preds.end(); ++x)
					for (termset::const_iterator o = x->second.begin(); o != x->second.end(); ++o)
						addhead(heads, new term(x->first->p, subject, *o));
			else for (termset::const_iterator o = objs.begin(); o != objs.end(); ++o) addhead(heads, (*o)->addbody(subjs));
			if (*s == L'}') { ++s; return heads; }
			preds.clear();
			subjs.clear();
			objs.clear();
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

string formatlist(const term* t, bool in = false) {
	if (!t || !t->s || !t->o) return L"";
	if (t->p != Dot)
		throw 0;
	std::wstringstream ss;
	if (!in) ss << L'(';
	ss << formatlist(t->o, true) << L' ' << format(t->s) << L' ';
	if (!in) ss << L')';
	return ss.str();
}

string format(const term* t, bool body) {
	if (!t || !t->p) return L"";
	std::wstringstream ss;
	if (body && t->p == implies) {
		ss << L'{';
		for (term::bveccit x = t->begin(); x != t->end(); ++x) ss << format((*x)->t) << L';';
		ss << L'}';
		ss << format(t, false);
	}
	else if (!t->p) return L"";
	else if (t->p != Dot) {
		if (t->s) ss << format(t->s) << L' ' << dict[t->p] << L' ' << format(t->o) << L'.';
		else ss << dict[t->p];
	}
	else return formatlist(t);
	return ss.str();
}

#define IDENT for (int n = 0; n < dep; ++n) ss << L'\t'

string format(const termset& t, int dep) {
	std::wstringstream ss;
	for (termset::const_iterator _x = t.begin(); _x != t.end(); ++_x) {
		term* x = *_x;
		if (!x || !x->p) continue;
		IDENT;
		ss << format(x, true);
		if (x->body.size()) 
			ss << L" implied by: ";
		else
			ss <<  L" a fact.";
		ss << std::endl;
		for (term::bveccit y = x->begin(); y != x->end(); ++y) {
			IDENT;
			const term::body_t* bt = *y;
			dout << format(bt->t) << std::endl;
			ss << L"\t" << format((*y)->t, true) << L" matches to heads:" << std::endl;
			for (std::vector<term::body_t::match*>::iterator z = (*y)->begin(); z != (*y)->end(); ++z) {
				IDENT;
				ss << L"\t\t" << format((*z)->t, true) << std::endl;
			}
		}
	}
	return ss.str();
}

typedef std::list<std::pair<term*, subs> > ground;
typedef std::map<resid, std::list<std::pair<term*, ground> > > evidence;

typedef boost::shared_ptr<struct proof> sp_proof;
struct proof {
	term* rule;
	term::bvecit b;
	sp_proof prev, creator, next;
	subs s;
	ground g() const {
		if (!creator) return ground();
		ground r = creator->g();
		termset empty;
		if (btterm) r.push_back(std::make_pair(btterm, subs()));
		else if (creator->b != creator->rule->end()) {
			if (!rule->body.size()) r.push_back(std::make_pair(rule, subs()));
		} else if (creator->rule->body.size()) r.push_back(std::make_pair(creator->rule, creator->s));
		return r;	
	}
	term* btterm;
	proof(sp_proof c, term* r, term::bvecit* l, sp_proof p, const subs&  _s = subs())
		: rule(r), b(l ? *l : rule->begin()), prev(p), creator(c), s(_s), btterm(0) { }
	proof(sp_proof c, proof& p) 
		: rule(p.rule), b(p.b), prev(p.prev), creator(c), btterm(0) { }
};

sp_proof step(sp_proof _p, sp_proof& lastp) {
	size_t steps = 0;
	if (!lastp) lastp = _p;
//	_p->next = sp_proof();
	evidence e;
//	while (_p) {
		if (++steps % 1000000 == 0) (dout << "step: " << steps << std::endl);
		sp_proof ep = _p;
		proof& p = *_p;
		term* t = p.rule;
		if (t) {
			while ((ep = ep->prev))
				if (ep->rule == p.rule && ep->rule->unify_ep(ep->s, t, p.s)) {
					_p = _p->next;
					t = 0;
					break;
				}
			if (!t) return p.next;
		}
		if (p.b != p.rule->end()) {
			term::body_t& pb = **p.b;
			while (pb(p.s)) {
				sp_proof r(new proof(_p, (*pb.it)->t, 0, _p, pb.ds));
				lastp->next = r;
				lastp = r;
			}
			return p.next;
		}
		else if (!p.prev) {
			term* _t;
			for (term::bvecit r = p.rule->begin(); r != p.rule->end(); ++r) {
				if (!(_t = ((*r)->t->evaluate(p.s)))) continue;
				e[_t->p].push_back(std::make_pair(_t, p.g()));
				dout << "proved: " << format(_t) << std::endl;
			}
			return p.next;
		}
//		else {
			proof& ppr = *p.prev;
			sp_proof r(new proof(_p, ppr));
			r->s = ppr.s;
			p.rule->unify(p.s, (*r->b)->t, r->s);
			++r->b;
			step(r, lastp);
			return p.next;
//		}
//		_p = p.next;
//	}
	dout << "steps: " << steps << std::endl;
	return sp_proof();
}
int main() {
	dict.init();
	termset kb = readqdb(din);
	termset query = readqdb(din);

	term* q = new term(kb, query);
	for (termset::iterator it = kb.begin(); it != kb.end(); ++it)
		(*it)->trymatch(kb);
	kb.push_back(q);
	dout << "kb:" << std::endl << format(kb) << std::endl;

	sp_proof p = sp_proof(new proof(sp_proof(), q, 0, sp_proof(), subs())), lastp;
	while ((p = step(p, lastp)));

	return 0;
}
