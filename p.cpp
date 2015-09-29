#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <string>
#include <functional>
#include <set>
#include <cmath>
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
	vector() : a(0),n(0),c(0)/*,m(*this)*/ {}
	vector(const vector<T, ispod>& t) : a(0),n(0),c(0)/*,m(*this)*/ { copyfrom(t); }
	vec_t& operator=(const vector<T, ispod>& t) { copyfrom(t); return *this; }
	T operator[](size_t k) const 		{ return a[k]; }
	T& operator[](size_t k) 		{ return a[k]; }
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
	vector(size_t sz) : a(0), n(sz), c(ceil(((double)sz) / (double)szchunk)) {
		memset(a = (T*)malloc(c * szchunk), 0, c * szchunk);
	}
/*
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
*/
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
	resid set(string v) {
		if (!v.size()) throw "bidict::set called with a node containing null value";
		map<string, resid, false>::iterator it = pi.find ( v );
		if (it) return it->second;
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

typedef map<int, term*, true> condset;

bool put(int p, term* t, condset& c) {
	static condset::iterator i;
	return ((i = c.find(p)) == c.end()) ? (c[p] = t), true : (i->second == t);
}

bool mkconds(term* h, term* b, condset& c) {
	size_t s = h->args.size();
	if (s != b->args.size()) return false;
	if (h->p < 0) {
		if (b->p < 0) {
			if (h->p != b->p) return false;
		}
		else if (!put(b->p, h, c))
			return false;
	} else if (!put(h->p, b, c))
		return false;
	for (size_t n = 0; n < s; ++n)
		if (!mkconds(h->args[n], b->args[n], c))
			return false;
	return true;
}

map<int, map<int, std::function<void(struct frame*)>, false>, false> conds;

struct frame {
	size_t h, b;
	frame* prev;
	termset env;
	frame() : h(0), b(0), prev(0), env(nvars) {}
	void run() { conds[h][b](this); }
};

void run(termset& heads) {
	const size_t max_frames = 1e+6;
	frame* last = new frame[max_frames];
	for (size_t n = 0; n < heads.size(); ++n)
		for (size_t k = 0; k < heads[n]->body.size(); ++k) {
			map<int, condset, false> cs;
			for (size_t m = 0; m < heads.size(); ++m) {
				map<int, term*,true> c;
				term *b = heads[n]->body[k], *h = heads[m];
				if (mkconds(h, b, c))
					cs[m] = c;
			}
		       	conds[n][k] = [cs, n, k, last](frame* f) mutable {
				if (f->h != n && f->b != k) throw 0;
				for (auto x : cs) {
					bool b = true;
					for (auto y : x.second) {
						int p = y.first;
						term *s = y.second;
						if (f->env[p]) {
							if (f->env[p] != s) {
								b = false;
								break;
							}
						} else
							last->env[p] = s;
					}
					if (b) {
						last->h = x.first, last->b = 0, last->prev = f;
						for (size_t z = 0; z < f->env.size(); ++z)
							if (f->env[z])
								last->env[z] = f->env[z];
						++last;
					}
				}
			};
		}
	vector<size_t> q;
	for (size_t n = 0; n < heads.size(); ++n)
		if (!heads[n]->p)
			q.push_back(n);
	for (size_t n = 0; n < q.size(); ++n)
		last[n].h = q[n];
	while (last->h) last++->run();
}

int main() {
	termset kb = nqparser::readterms(din);
	termset query = nqparser::readterms(din);
	term* q = mkterm(query);
	kb.push_back(q);
	run(kb);
//	compile(kb);
//	for (uint n = 0; n < rules.size(); ++n)
//		for (uint k = 0; k < rules[n].size(); ++k)
//			for (uint m = 0; m < rules[n][k].size(); ++m)
//				dout << "int _" <<n << '_' << k << '_' << m << "(int* env, bool f) { ",
//				rules[n][k][m](), dout << " }" << endl;
//	auto r = frame(rules.size()-1)();
//	dout << r.size() << endl;
//	for (frame* f : r) (*f)();
	
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
