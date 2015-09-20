#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <string>
#include <functional>
#include <set>
using std::endl;
typedef std::wstring string;
typedef int resid;
struct term;

#define vec_t vector<T,ispod>
#define map_t map<K, V, ispod>
#define vec_templ template<typename T, bool ispod>
#define map_tmpl template<typename K, typename V, bool ispod>

const size_t chunk = 4;

template<typename T, bool ispod = std::is_pod<T>::value>
struct vector {
protected:
	T* a;
	static const size_t szchunk;
	size_t n, c;
public:
	typedef T* iterator;
	vector() : a(0),n(0),c(0),m(*this) {}
	vector(const vector<T, ispod>& t) : a(0),n(0),c(0),m(*this) { copyfrom(t); }
	vec_t& operator=(const vector<T, ispod>& t) { copyfrom(t); return *this; }
	T operator[](size_t k) const 		{ return a[k]; }
	size_t size() const			{ return n; }
	T* begin() const			{ return a; }
	T* end() const				{ return n ? &a[n] : 0; }
	void clear()				{ n = c = 0; if (!a) return; free(a); a = 0; }
	void clear1() 				{ n = c = 0; }
	bool empty() const			{ return !n; }
	~vector()				{ if(a)free(a); }
	T* back()				{ return n ? &a[n-1] : 0; }
	T& push_back(const T& t) {
		if (!(n % chunk))
			a = (T*)realloc(a, szchunk * ++c);
		if (ispod) a[n] = t;
		else new (&a[n])(T)(t);
		return a[n++];
	}
	void copyfrom(const vector<T, ispod>& t) {
		if (!(n = t.n)) { c = 0; return; }
		memcpy(a = (T*)realloc(a, t.c * szchunk), t.a, (c = t.c) * szchunk);
	}

	struct coro {
		bool state;
		iterator i, e;
		const vector<T, ispod>* t;
		coro(const vector<T, ispod>& _t) : state(false), i(0), e(0), t(&_t) {}
		bool operator()() {
			switch (state) {
			case false: 
				i = t->begin(), e = t->end();
				while (i != e) {
					return state = true;
			case true: 	++i;
				}
				return state = false;
			}
		}
	} m;
};

template<typename K, typename V>
struct mapelem {
	K first;
	V second;
	mapelem(){}
	mapelem(const mapelem& m) : first(m.first), second(m.second) {}
	mapelem(const K& _k, const V& _v) : first(_k), second(_v) {} 
};

template<typename K, typename V, bool ispod>
struct map : public vector<mapelem<K, V>, ispod > {
	typedef mapelem<K, V> vtype;
	typedef vector<mapelem<K, V>, ispod > base;
	typedef map<K, V, ispod > _this;
public:
	map() : base() {}
	map(const base& t)		{ copyfrom(t); }
	map(const _this& _s) : base(){ base::copyfrom(_s); }
	map<K, V, ispod>& operator=(const _this& m)	{ base::copyfrom(m); return *this; }
	V& operator[](const K& k){ mapelem<K, V>* z = find(k); return z ? z->second : set(k, V()); }
//	_this& operator=(const _this& m);
	typedef vtype* iterator;
	V& set(const K& k, const V& v) {
		vtype* z = find(k);
		if (z) { return z->second = v; }
		return base::push_back(vtype(k, v)).second;
	//	qsort(base::a, base::n, sizeof(vtype), compare);
	}
	mapelem<K, V>* find(const K& k) const {
		if (!base::n) return 0;
		iterator e = base::end();
		for (vtype* x = base::begin(); x != e; ++x) if (x->first == k) return x;
		return 0;
	//	vtype v(k, V()); vtype* z = (vtype*)bsearch(&v, base::a, base::n, sizeof(vtype), compare); return z;
	}
private:
	static int compare(const void* x, const void* y) { return ((vtype*)x)->first - ((vtype*)y)->first; }
};

int nvars = 0;

class bidict {
	map<resid, string, false> ip;
	map<string, resid, false> pi;
public:
	resid set ( string v ) {
		if (!v.size()) throw "bidict::set called with a node containing null value";
		map<string, resid, false>::iterator it = pi.find ( v );
		if ( it ) return it->second;
		resid k = pi.size() + 1;
		static int lastres = -1, lastvar = 1;
		if ( v[0] != L'?' ) k = lastres--;
		else nvars = k = lastvar++;
		pi[v] = k, ip[k] = v;
		return k;
	}
	string operator[](resid k) { return ip[k]; }
	resid operator[](string v) { return set(v); }
} dict;

template<typename T, bool ispod /*= std::is_pod<T>::value*/>
const size_t vector<T, ispod>::szchunk = chunk * sizeof(T);

//map_tmpl int map<K, V, ispod>::compare(const void* x, const void* y) { return ((vtype*)x)->first - ((vtype*)y)->first; }
typedef vector<term*, true> termset;

class nqparser {
private:
	wchar_t *t;
	const wchar_t *s;
	int pos;
public:
	nqparser() : t(new wchar_t[4096*4096]) {}
	~nqparser() { delete[] t; }

	bool readcurly(vector<term*, true>& ts);
	term* readlist();
	term* readiri();
	term* readbnode();
	term* readvar();
	term* readlit();
	term* readany(bool lit = true);
	void addhead(vector<term*, true>& ts, term* t, termset* subjs = 0);
	vector<term*, true> operator()(const wchar_t* _s);
	static vector<term*, true> readterms (std::wistream& is);
};

void trim(string& s) {
	string::iterator i = s.begin();
	while (iswspace(*i)) s.erase(i), i = s.begin(); 
	size_t n = s.size();
	if (n) {
		while (iswspace(s[--n]));
		s = s.substr(0, ++n);
	}
}

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

typedef map<resid, term*, true> subs;
string format(const term* t, bool body = false);
string format(const termset& t, int dep = 0);
string format(const subs& s);
void trim(string& s);
string wstrim(string s);
std::wistream &din = std::wcin;
std::wostream &dout = std::wcout;
bool startsWith ( const string& x, const string& y ) { return x.size() >= y.size() && x.substr ( 0, y.size() ) == y; }
string wstrim(string s) { trim(s); return s; }
const resid implies = dict.set(L"=>");
const resid Dot = dict.set(L".");

struct term {
	term(termset& query) : p(0), szargs(0), body(query) { }
	term(resid _p, const termset& _args = termset()) : p(_p), args(_args), szargs(args.size()) {}

	const resid p;
	termset args;
	const size_t szargs;
	termset body;
};
const size_t tchunk = 8192, nch = tchunk / sizeof(term);

struct tcmp {
	bool operator()(term* _x, term* _y) const {
		term &x = *_x, &y = *_y;
		if (x.szargs != y.szargs) return x.szargs < y.szargs;
		if (x.p != y.p) return x.p < y.p;
		for (termset::iterator i = x.args.begin(), e = x.args.end(), j = y.args.begin(); i != e; ++i, ++j) 
			if ((*i)->p != (*j)->p)
				return (*i)->p < (*j)->p;
		return false;
	}
};
std::set<term*, tcmp> terms;
term* mkterm(termset& query) { return new term(query); }
term* mkterm(resid p, const termset& args = termset()) { return new term(p, args); }

#define EPARSE(x) dout << (string(x) + string(s,0,48)) << endl, throw 0
#define SKIPWS while (iswspace(*s)) ++s
#define RETIF(x) if (*s == x) return 0
#define RETIFN(x) if (*s != x) return 0
#define PROCEED1 while (!iswspace(*s) && *s != L',' && *s != L';' && *s != L'.' && *s != L'}' && *s != L'{' && *s != L')')
#define PROCEED PROCEED1 t[pos++] = *s++; t[pos] = 0; pos = 0
#define TILL(x) do { t[pos++] = *s++; } while (x)

bool nqparser::readcurly(termset& ts) {
	while (iswspace(*s)) ++s;
	if (*s != L'{') return false;
	ts.clear();
	do { ++s; } while (iswspace(*s));
	if (*s == L'}') ++s;
	else ts = (*this)(s);
	return true;
}

term* nqparser::readlist() {
	if (*s != L'(') return (term*)0;
	++s;
	termset items;
	term* pn;
	while (*s != L')') {
		SKIPWS;
		if (*s == L')') break;
		if (!(pn = readany(true))) EPARSE(L"couldn't read next list item: ");
		items.push_back(pn);
		SKIPWS;
		if (*s == L'.') while (iswspace(*s++));
		if (*s == L'}') EPARSE(L"unexpected } inside list: ");
	};
	do { ++s; } while (iswspace(*s));
	return mkterm(Dot, items);
}

term* nqparser::readiri() {
	while (iswspace(*s)) ++s;
	if (*s == L'<') {
		while (*++s != L'>') t[pos++] = *s;
		t[pos] = 0, pos = 0, ++s;
		return mkterm(dict[t]);
	}
	if (*s == L'=' && *(s+1) == L'>') return ++++s, mkterm(implies);
	PROCEED;
	string iri = wstrim(t);
	return mkterm(dict[iri]);
}

term* nqparser::readbnode() { SKIPWS; if (*s != L'_' || *(s+1) != L':') return 0; PROCEED; return mkterm(dict[wstrim(t)]); } 
term* nqparser::readvar() { SKIPWS; RETIFN(L'?'); PROCEED; return mkterm(dict[wstrim(t)]); }

term* nqparser::readlit() {
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
//boost:replace_all(t1, L"\\\\", L"\\");
	return mkterm(dict[wstrim(t1)]);//, pstrtrim(dt), pstrtrim(lang);
}

term* nqparser::readany(bool lit) {
	term* pn;
	return ((pn = readbnode()) || (pn = readvar()) || (lit && (pn = readlit())) || (pn = readlist()) || (pn = readiri()) ) ? pn : 0;
}

void nqparser::addhead(termset& heads, term* h, termset* body) {
	if (body) h->body = *body;
	heads.push_back(h);
}

termset nqparser::operator()(const wchar_t* _s) {
	typedef map<term*, termset, false> preds_t;
	preds_t preds;
	s = _s;
//	string graph;
	term *subject, *pn;
	termset subjs, objs, heads;
	pos = 0;
	preds_t::iterator pos1 = 0;

	while(*s) {
		if (readcurly(subjs)) subject = 0;
		else if (!(subject = readany(false))) EPARSE(L"expected iri or bnode subject:"); // read subject
		do { // read predicates
			while (iswspace(*s) || *s == L';') ++s;
			if (*s == L'.' || *s == L'}') break;
			if ((pn = readiri())) { // read predicate
				preds.set(pn, termset());
				pos1 = preds.back();
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
//			graph = dict[pn->p];
		} //else graph = ctx;
		SKIPWS; while (*s == '.') ++s; SKIPWS;
		if (*s == L')') EPARSE(L"expected ) outside list: ");
		if (subject)
			for (preds_t::vtype* x = preds.begin(); x != preds.end(); ++x)
				for (termset::iterator o = x->second.begin(); o != x->second.end(); ++o) {
					termset ts;
					ts.push_back(subject);
					ts.push_back(*o);
					addhead(heads, mkterm(x->first->p, ts));
				}
		else for (termset::iterator o = objs.begin(); o != objs.end(); ++o)
			addhead(heads, *o, &subjs);
		if (*s == L'}') { ++s; return heads; }
		preds.clear();
		subjs.clear();
		objs.clear();
	}
	return heads;
}

termset nqparser::readterms(std::wistream& is) {
	string s, c;
	nqparser p;
	string ss, space = L" ";
	while (getline(is, s)) {
		trim(s);
		if (s[0] == '#') continue;
		if (startsWith(s, L"fin") && wstrim(s.c_str() + 3) == L".") break;
		ss += space + s + space;
	}
	return p((wchar_t*)ss.c_str());
}

string format(const term* t, bool body) {
	if (!t || !t->p) return L"";
	string ss;
	if (body && t->p == implies) {
		ss += L'{';
		for (termset::iterator x = t->body.begin(); x != t->body.end(); ++x) ss += format(*x) + L';';
		ss += L'}';
		ss += format(t, false);
	}
	else if (!t->p) return L"";
	ss += dict[t->p];
	if (t->args.size()) {
		ss += L'(';
		for (term** y = t->args.begin(); y != t->args.end(); ++y) ss += format(*y) += L' ';
		ss += L") ";
	}
	return ss;
}

string format(const subs& s) {
	string ss;
	vector<mapelem<resid, term*>, true > v((const vector<mapelem<resid, term*>, true >&)s);
	for (size_t n = 0; n < s.size(); ++n)
		ss += dict[v[n].first] += L'\\' + format(v[n].second) += L' ';
	return ss;
}

#define IDENT for (int n = 0; n < dep; ++n) ss += L'\t'

string format(const termset& t, int dep) {
	string ss;
	for (termset::iterator _x = t.begin(); _x != t.end(); ++_x) {
		term* x = *_x;
		if (!x || !x->p) continue;
		IDENT;
		ss += format(x, true);
		if (x->body.size()) ss += L" implied by: ";
		ss += L"\r\n";
		for (termset::iterator y = x->body.begin(); y != x->body.end(); ++y) {
			IDENT;
//			((
			ss += L"\t";
			ss += format(*y, true);
			ss += L"\r\n";
//			)/* += L" matches to heads:")*/ += endl;
//			for (termset::iterator z = (*y)->matches.begin(); z != (*y)->matches.end(); ++z) {
//				IDENT;
//				ss += L"\t\t" + format(*z, true) + endl;
//			}
		}
	}
	return ss;
}
size_t steps = 0;

/*
 * retvals:
 *  -1 for success of single resource
 *  -2 for fail of single resource
 *  >=0 index in heads
 */
typedef std::function<int(int*, bool)> comp;

#define EMIT(x) dout << #x << endl, (x);
#define pxy printxy(x, y, env, f)
void printxy(int x, int y, int* env, bool f) {
	dout << "f: " << f << " x: " << x << " y: " << y << ' ';
	dout << "x: " << dict[x] << " y: " << dict[y] << " env: ";
	if (env) 
		for (int n = 0; n < nvars; ++n) 
			dout << dict[n] << '=' << (env[n]?dict[env[n]]:string()) << ' ';
	dout << endl;
}

bool match(int x, int y, comp& r) {
	if (x < 0 && y < 0) {
		if (x != y) return false;
		return r = [](int*,bool){return EMIT(-1);}, true;
	}
	if (x > 0 && y < 0) return r = [x, y](int* env, bool f) { pxy;
		if(!env) return EMIT(-1);
		return EMIT((f ? env[x] = y, -1 : env[x] ? env[x] == y ? -1 : -2 : -1));
	}, true;
	if (y > 0 && x < 0) return r = [x, y](int* env, bool f) { pxy;
		if (!env) return EMIT(-1);
		return EMIT((f ? env[y] = x, -1 : env[y] ? env[y] == x ? -1 : -2 : -1));
	}, true;
//	if (x > 0 && y > 0)
	return r = [x, y](int* env, bool f) { pxy;
		if (!env) return EMIT(-1);
		if (f) return EMIT((env[x] ? env[y] = env[x], -1 : env[x] = env[y], -1));
		return EMIT(((env[x] && env[y]) ? env[y] == env[x] ? -1 : -2 : -1));
	}, true;
}

bool compile(term& x, term& y, vector<comp>& _r, int n, int k, int l) {
	if (!x.p || !y.p || x.args.size() != y.args.size() || &x == &y) return false;
	comp c;
	if (!match(x.p, y.p, c)) {
		dout << "unmatched " << x.p << " with " << y.p << ' ' << dict[x.p] << " " << dict[y.p] << endl;
		return false;
	}
	dout << "matched " << x.p << " with " << y.p << ' ' << dict[x.p] << " " << dict[y.p] << endl;
	vector<comp> r = _r;
	r.push_back(c);
	for (term **it1 = x.args.begin(), **it2 = y.args.begin(), **e = x.args.end(); it1 != e;)
		if (*it1 != *it2)
			if (!compile(**it1++, **it2++, r, -1, -1, -1))
				return false;
	_r.copyfrom(r);
	if (n != -1) {
		_r.push_back([n](int*,bool){return n;}); // push head index
		_r.push_back([k](int*,bool){return k;}); // push body index
		_r.push_back([l](int*,bool){return l;}); // push matching head index
	}
	return true;
}

vector<vector<vector<comp>>> rules; // only bodies, head are compiled inside

void compile(termset& heads) {
	for (int h = 0; h < (int)heads.size(); ++h) {
		vector<comp> c;
		vector<vector<comp>> r;
		int sz = heads[h]->body.size();
		if (!sz)
			c.push_back([](int*,bool){return -3;}), r.push_back(c), c.clear();
		else for (int b = 0; b < sz; ++b) 
			for (int h1 = 0; h1 < (int)heads.size(); ++h1)
				if (compile(*heads[h1], *heads[h]->body[b], c, h, b, h1))
					r.push_back(c), c.clear();
		if (r.size())
			rules.push_back(r);
	}
	dout << rules.size() << endl; 
	for (auto x : rules) { 
		dout << x.size() << ' '; 
		for (auto y : x) 
			dout << y.size() << ' '; 
		dout << endl; 
	}
}

struct frame {
	int *env;
	int h, h1, b; // indices of the matching head and body
	vector<comp> c; // compiled functions for that body item
	frame() : env(new int[nvars]), h(0), h1(0), b(0), c(rules[h][b]) { memset(env, 0, sizeof(int) * nvars); }
	frame(int _h) : frame() { h = _h; }
	frame(int _h, int _b, int _h1, int *e, vector<comp> _c, int from, int to) : frame(_h) {
		h1 = _h1, b = _b;
		if (e) {
			memcpy(env, e, sizeof(int) * nvars);
			while (from <= to) _c[from++](env, true);
		} else memset(env, 0, sizeof(int) * nvars);
		dout << "new frame: h="<<h<<" h1="<<h1<<" b=" << b<<" from="<<from<<" to="<<to<<endl;
	}
	~frame() { delete[] env; }

	vector<frame*> operator()() {
		int r, last = 0, bb, h1;
		vector<frame*> ret;
		dout << "frame..." << endl;
		for (comp* i = c.begin(); i != c.end(); ++i) {
			r = (*i)(env, false);
			dout << "r: " << r << endl;
			if (r == -1) continue;
			if (r == -2)
				do { 
					r = (*++i)(0, false); 
					dout << "r: " << r << endl;
				} while (r < 0);
			else if (r == -3) {
//			if (!rules[r].size()) {
				dout << " ground! " << endl;
				continue;
			}
			bb = (*++i)(0, false); // read body index
			h1 = (*++i)(0, false); // read matching head index
//			ret.push_back(
					(*new frame(r, bb, h1, env, c, last, r))();
			last = r;
		}
		return ret;
	}
};

typedef vector<mapelem<term*, subs>> ground;
typedef map<resid, vector<mapelem<term*, ground>>, false> evidence;

struct frame {
	term* rule;
	termset::iterator b;
	pframe prev, creator, next;
	subs s;
	int ref;
	ground g() const { 
		if (!creator) return ground();
		ground r = creator->g();
		termset empty;
		frame& cp = *creator;
		typedef mapelem<term*, subs> elem;
		if (cp.b != cp.rule->body.end()) {
			if (!rule->body.size())
				r.push_back(elem(rule, subs()));
		} else if (cp.rule->body.size())
			r.push_back(elem(creator->rule, creator->s));
		return r;	
	}
	frame(pframe c, term* r, termset::iterator l, pframe p, const subs& _s)
			: rule(r), b(l ? l : rule->body.begin()), prev(p), creator(c), next(0), s(_s), ref(0) { if(c)++c->ref;if(p)++p->ref; }
	frame(pframe c, term* r, termset::iterator l, pframe p)
			: rule(r), b(l ? l : rule->body.begin()), prev(p), creator(c), next(0), ref(0) { if(c)++c->ref;if(p)++p->ref; }
	frame(pframe c, frame& p, const subs& _s) 
			: rule(p.rule), b(p.b), prev(p.prev), creator(c), next(0), s(_s), ref(0) { if(c)++c->ref; if(prev)++prev->ref; }
	void decref() { if (ref--) return; if(creator)creator->decref();if(prev)prev->decref(); delete this; }
};

void prove(pframe _p, pframe& lastp) {
	if (!lastp) lastp = _p;
	evidence e;
	while (_p) {
		if (++steps % 1000000 == 0) (dout << "step: " << steps << endl);
		pframe ep = _p;
		frame& p = *_p;
		term* t = p.rule, *epr;
		// check for euler path
		ssub = p.s;
		while ((ep = ep->prev)) {
			if ((epr = ep->rule) == p.rule && t->unify_ep(*t, *epr, ep->s)) {
				t = 0;
				break;
			}
		}
		if (!t);
		else if (p.b != p.rule->body.end()) {
			term::coro& m = (**p.b).m;
			while (m())
				lastp = lastp->next = pframe(new frame(_p, m.i, 0, _p, m.dsub));
		}
		else if (p.prev) { // if body is done, go up the tree
			frame& ppr = *p.prev;
			pframe r(new frame(_p, ppr, ppr.s));
			p.rule->unify(*p.rule, **r->b++, r->s);
			prove(r, lastp);
		}
		#ifndef NOTRACE
		else {
			term* _t; // push evidence
			for (termset::iterator r = p.rule->body.begin(); r != p.rule->body.end(); ++r) {
				if (!(_t = ((*r)->evaluate(**r)))) continue;
				e[_t->p].push_back(mapelem<term*, ground>(_t, p.g()));
				dout << "proved: " << format(_t) << endl;
			}
		}
		#endif
		pframe pf = _p; _p = p.next; pf->decref();
	}
}

int main() {
	termset kb = nqparser::readterms(din);
	termset query = nqparser::readterms(din);
	term* q = mkterm(query);
	kb.push_back(q);
	compile(kb);
	int n = 0;
	auto r = frame(rules.size()-1)();
	dout << r.size() << endl;
	for (frame* f : r) (*f)();
	
//	vector<frame*> f = frame()();
//	TRACE(
	dout << "kb:" << endl << format(kb) << endl;

	// create the initial frame with the query term as the frame's rule
//	pframe p(new frame(pframe(), q, 1, pframe(), subs())), lp = 0;
//	clock_t begin = clock(), end;
//	prove(p, lp); // the main loop
//	end = clock();
//	dout << "steps: " << steps << " elapsed: " << (1000. * double(end - begin) / CLOCKS_PER_SEC) << "ms" << endl;

	return 0;
}
