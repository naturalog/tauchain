#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <ctime>
#include <algorithm>
#include <functional>

template<typename T>
struct sp { // smart pointer
	T* p = 0;
	int* r = 0;
	sp() {}
	sp(T* v) : p(v) 			{ *(r = new int) = 1; }
	sp(const sp<T>& s) : p(s.p), r(s.r) 	{ if (r) ++*r; }

	~sp() { if (r && !--*r) delete p , delete r; }

	T& operator*()  { return *p; } 
	T* operator->() { return p; }
    
	sp<T>& operator=(const sp<T>& s) {
		if (this == &s) return *this;
		if (r && !--*r) delete p , delete r;
		p = s.p;
		if ((r = s.r)) ++*r;
		return *this;
	}
};
typedef sp<struct frame> sp_frame;

const size_t chunk = 4;
template<typename T, bool ispod = std::is_pod<T>::value>
class vector {
protected:
	T* a;
	const size_t szchunk = chunk * sizeof(T);
	size_t n, c;
public:
	explicit vector() : a(0),n(0),c(0) {}
	vector(const vector<T>& t) : a(0),n(0),c(0) { copyfrom(t); }
	vector<T>& operator=(const vector<T>& t) { copyfrom(t); return *this; }

	typedef T* iterator;
	T operator[](size_t k) const { return a[k]; }
	size_t size() const	{ return n; }
	T* begin() const	{ return a; }
	T* end() const		{ return a ? &a[n] : 0; }
	void clear()		{
		n = 0; c = 0;
		if (!a) return;
		free(a);
		a = 0;
	}
	bool empty() const	{ return !a; }
	~vector()		{ clear(); }
	iterator back()		{ return n ? &a[n-1] : 0; }
	T& push_back(const T& t) {
		if (!(n % chunk))
			a = (T*)realloc(a, szchunk * ++c);
		if (ispod) a[n] = t;
		else new (&a[n])(T)(t);
		return a[n++];
	}
protected:	
	void copyfrom(const vector<T>& t) {
		clear();
		if (!(n = t.n)) return; 
		memcpy(a = (T*)malloc(t.c * szchunk), t.a, (c = t.c) * szchunk);
//		if (!ispod)
//			for (size_t k = 0; k < n; ++k)
//				new (&a[k])(T);
	}
};

template<typename K, typename V>
struct mapelem {
	K first;
	V second;
	mapelem(){}
	mapelem(const mapelem& m) : first(m.first), second(m.second) {}
	mapelem(const K& _k, const V& _v) : first(_k), second(_v) {} 
};

template<typename K, typename V>
struct map : public vector<mapelem<K, V> > {
	typedef mapelem<K, V> vtype;
	typedef vector<mapelem<K, V> > base;
public:
	map(const vector<vtype>& t) { copyfrom(t); }
	map() : base() {}
	map(const map<K, V>& _s) : base()      	{ base::copyfrom(_s); }
	map<K, V>& operator=(const map<K, V>& m){ base::copyfrom(m); return *this; }
//	const V& operator[](const K& k) const	{ mapelem<K, V>* z = find(k); return z->second; } // ? z->second : set(k, V()); }
	V& operator[](const K& k)		{ mapelem<K, V>* z = find(k); return z ? z->second : set(k, V()); }
	V& set(const K& k, const V& v) {
		vtype* z = find(k);
		if (z) { return z->second = v; }
		return base::push_back(vtype(k, v)).second;
//		qsort(base::a, base::n, sizeof(vtype), compare);
	}
	typedef vtype* iterator;
	iterator find(const K& k) const {
		if (!base::n) return 0;
		for (vtype& x : *this) if (x.first == k) return &x;
		return 0;
//		vtype v(k, V());
//		vtype* z = (vtype*)bsearch(&v, base::a, base::n, sizeof(vtype), compare);
//		return z;
	}
private:
	static int compare(const void* x, const void* y) { return ((vtype*)x)->first - ((vtype*)y)->first; }
};

typedef std::wstring string;
void trim(string& s) {
	string::iterator i = s.begin();
	while (iswspace(*i)) {
		s.erase(i);
		i = s.begin();
	}
	size_t n = s.size();
	if (n) {
		while (iswspace(s[--n]));
		s = s.substr(0, ++n);
	}
}

typedef int resid;
std::wistream &din = std::wcin;
std::wostream &dout = std::wcout;
struct term;
typedef map<resid, term*> subs;
typedef vector<term*> termset;
string lower ( const string& s_ ) { string s = s_; std::transform ( s.begin(), s.end(), s.begin(), ::towlower ); return s; }
string wstrim(string s) { trim(s); return s; }
string format(const term* t, bool body = false);
string format(const termset& t, int dep = 0);
string format(const subs& s);
bool startsWith ( const string& x, const string& y ) { return x.size() >= y.size() && x.substr ( 0, y.size() ) == y; }

class wruntime_error : public std::exception {
	string msg;
public:
	wruntime_error(string s) : msg(s){}
	virtual const char* what() const _GLIBCXX_USE_NOEXCEPT { return std::string(msg.begin(), msg.end()).c_str(); }
	virtual ~wruntime_error() _GLIBCXX_USE_NOEXCEPT {}
};

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

class bidict {
	map<resid, string> ip;
	map<string, resid> pi;
public:
	void init() {
	}

	resid set ( string v ) {
		if (!v.size()) throw std::runtime_error("bidict::set called with a node containing null value");
		map<string, resid>::iterator it = pi.find ( v );
		if ( it ) return it->second;
		resid k = pi.size() + 1;
		if ( v[0] == L'?' ) k = -k;
//		pi.set(v, k), ip.set(k, v);
		pi[v] = k, ip[k] = v;
		return k;
	}

	string operator[] ( resid k ) { return ip[k]; }
	resid operator[] ( string v ) { return set(v); }
} dict;

const resid implies = dict.set(L"=>");
const resid Dot = dict.set(L".");

term* mkterm();
term* mkterm(termset& kb, termset& query);
term* mkterm(resid p);
term* mkterm(resid p, const termset& args);

struct term {
	term() : p(0) { throw 0; }
	term(termset& kb, termset& query) : p(0) { addbody(query); trymatch(kb); }
	term(resid _p, const termset& _args = termset()) : p(_p), args(_args) {
		TRACE(if (!p) throw 0);
		static auto evvar = [](term& t, const subs& ss) {
			auto v = ss.find(t.p);
			return v ? v->second->evaluate(*v->second, ss) : (term*)0; 
		};
		static auto evnoargs = [](term& t, const subs&) { return &t; };
		static auto ev = [](term& t, const subs& ss) {
			static term *v;
			termset ts;
			for (term* a : t.args) {
				term& b = *a;
				ts.push_back(((v = b.evaluate(b, ss))) ? v : mkterm(b.p));
			}
			return mkterm(t.p, ts);
		};
		static auto u1 = [](term& s, const subs& ssub, term& d, subs& dsub) {
//			term* v;
			auto v = ssub.find(s.p);
			if (v) {
				term *e =  v->second->evaluate(*v->second, ssub);
				return e ? e->unify(*e, ssub, d, dsub) : true;
			}
			return true;
		};
		static auto u2 = [](term& s, const subs& ssub, term& d, const subs& dsub) { 
			auto v = ssub.find(s.p);
			if (v) {
				term *e =  v->second->evaluate(*v->second, ssub);
				return e ? e->unify_ep(*e, ssub, d, dsub) : true;
			}
			return true;
		};
		static auto u3 = [](term& s, const subs& ssub, term& d, subs& dsub) {
			if (d.p < 0) {
				auto v = dsub.find(d.p);
				term* e = v ? v->second->evaluate(*v->second, dsub) : 0;
				if (e) return s.unify(s, ssub, *e, dsub);
				dsub.set(d.p, s.evaluate(s, ssub));
//				TRACE(dout << "new sub: " << dict[d.p] << '\\' << format(v) << std::endl);
				return true;
			}
			if (s.p != d.p) return false;
			auto& ar = s.args;
			size_t sz = ar.size();
			if (sz != d.args.size()) return false;
			termset::iterator dit = d.args.begin();
			for (term* t : ar)
				if (!t->unify(*t, ssub, **dit++, dsub)) 
					return false;
			return true;
		};
		static auto u4 = [](term& s, const subs& ssub, term& d, const subs& dsub) {
			if (d.p < 0) {
				auto v = dsub.find(d.p);
				term* e = v ? v->second->evaluate(*v->second, dsub) : 0;
				return e ? s.unify_ep(s, ssub, *e, dsub) : true;
			}
			if (s.p != d.p) return false;
			if (s.p != d.p) return false;
			auto& ar = s.args;
			size_t sz = ar.size();
			if (sz != d.args.size()) return false;
			termset::iterator dit = d.args.begin();
			for (term* t : ar)
				if (!t->unify_ep(*t, ssub, **dit++, dsub)) 
					return false;
			return true;
		};
		if (p < 0) {
			evaluate = evvar;
			unify = u1;
			unify_ep = u2;
		}
		else {
			if (args.empty()) evaluate = evnoargs;
			else evaluate = ev;
			unify = u3;
			unify_ep = u4;
		}
	}
	const resid p;
	const termset args;
	term* (*evaluate)(term&, const subs&);
	bool (*unify)(term& s, const subs& ssub, term& d, subs& dsub);
	bool (*unify_ep)(term& s, const subs& ssub, term& d, const subs& dsub);
//	term *s, *o;
	struct body_t {
		friend struct term;
		term* t;
		bool state;
		body_t(term* _t) : t(_t), state(false) {}
		termset matches;
	};
	typedef vector<body_t*> bvec;
	bvec body;

	// add term(s) to body
	term* addbody(const termset& t) { for (termset::iterator it = t.begin(); it != t.end(); ++it) addbody(*it); return this; }
	term* addbody(term* t) {
		TRACE(dout << "added body " << format(t) << " to " << format(this) << std::endl);
		body.push_back(new body_t(t));
		return this;
	}

	// indexer: given rule's heads, see which can match to each body
	void trymatch(termset& heads) {
		for (bvec::iterator b = body.begin(); b != body.end(); ++b)
			for (termset::iterator it = heads.begin(); it != heads.end(); ++it)
				trymatch(**b, *it);
	}/*
	static term* evaluate(term* _t, const subs& ss) {
		static term *v, *r;
		if (!_t) throw 0;
		term& t = *_t;
		if (t.p < 0)
			return (v = ss[t.p]) ? r = v->evaluate(v, ss) : 0;
		if (t.args.empty())
			return &t;
		termset ts;
		for (term* a : t.args) {
			if ((v = a->evaluate(a, ss))) ts.push_back(v);
			else ts.push_back(mkterm(a->p));
		}
		return mkterm(t.p, ts);
		TRACE(dout<<"evaluate " << format(&t) << " under " << format(ss) << " returned " << format(r) << std::endl);
		return r;
	};
	bool unify(const subs& ssub, term& d, subs& dsub) {
		term* v;
		if (p < 0) return ((v=ssub[p]) && (v = v->evaluate(*v, ssub))) ? v->unify(ssub, d, dsub) : true;
		if (d.p < 0) {
			if ((v = dsub[d.p]) && (v = d.evaluate(d, dsub))) return unify(ssub, *v, dsub);
			dsub.set(d.p, v = evaluate(*this, ssub));
//			TRACE(dout << "new sub: " << dict[d.p] << '\\' << format(v) << std::endl);
			return true;
		}
		size_t sz = args.size();
		if (p != d.p || sz != d.args.size()) return false;
		for (size_t n = 0; n < sz; ++n)
			if (!args[n]->unify(ssub, *d.args[n], dsub)) 
				return false;
		return true;
	}
	bool unify_ep(const subs& ssub, term& d, const subs& dsub) {
		static term* v;
		if (p < 0) return ((v=ssub[p]) && (v = v->evaluate(*v, ssub))) ? v->unify_ep(ssub, d, dsub) : true;
		if (d.p < 0) return ((v=dsub[d.p]) && (v = v->evaluate(*v, dsub))) ? unify_ep(ssub, *v, dsub) : true;
		size_t sz = args.size();
		if (p != d.p || sz != d.args.size()) return false;
		for (size_t n = 0; n < sz; ++n)
			if (!args[n]->unify_ep(ssub, *d.args[n], dsub)) 
				return false;
		return true;
	}*/
	void _unify(const subs& ssub, termset& ts, sp_frame f, sp_frame& lastp);
private:
//	static term* v; // temp var for unifiers

	// indexer: match a term (head) to a body's term by trying to unify them.
	// if they cannot unify without subs, then they will not be able to
	// unify also during inference.
	void trymatch(body_t& b, term* t) {
		if (t == this) return;
		//b.addmatch(t, subs()); return; // unremark to disable indexing
		TRACE(dout << "trying to match " << format(b.t) << " with " << format(t) << std::endl);
		static subs d;
		if (b.t->unify(*b.t, subs(), *t, d)) {
			TRACE(dout << "added match " << format(t) << " to " << format(b.t) << std::endl);
			b.matches.push_back(t);
		}
		d.clear();
	}
};

const size_t tchunk = 8192, nch = tchunk / sizeof(term);
size_t bufpos = 0;
char* buf = (char*)malloc(tchunk);

#define MKTERM if (bufpos == nch) { buf = (char*)malloc(tchunk); bufpos = 0; } new (&((term*)buf)[bufpos])(term); return &((term*)buf)[bufpos++];
#define MKTERM1(x) if (bufpos == nch) { buf = (char*)malloc(tchunk); bufpos = 0; } new (&((term*)buf)[bufpos])(term)(x); return &((term*)buf)[bufpos++];
#define MKTERM2(x, y) if (bufpos == nch) { buf = (char*)malloc(tchunk); bufpos = 0; } new (&((term*)buf)[bufpos])(term)(x, y); return &((term*)buf)[bufpos++];

term* mkterm() { MKTERM }
term* mkterm(termset& kb, termset& query) { MKTERM2(kb, query); }
//	return new term(kb, query); }
term* mkterm(resid p, const termset& args) { MKTERM2(p, args); }
//	return new term(p, args); }
term* mkterm(resid p) { MKTERM1(p); }
//	return new term(p); }

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
		termset items;
		term* pn;
		while (*s != L')') {
			SKIPWS;
			if (*s == L')') break;
			if (!(pn = readany(true)))
				EPARSE(L"couldn't read next list item: ");
			items.push_back(pn);
			SKIPWS;
//			pn = pn->o = mkterm(Dot);
			if (*s == L'.') while (iswspace(*s++));
			if (*s == L'}') EPARSE(L"expected { inside list: ");
		};
		do { ++s; } while (iswspace(*s));
		return mkterm(Dot, items);
	}

	term* readiri() {
		while (iswspace(*s)) ++s;
		if (*s == L'<') {
			while (*++s != L'>') t[pos++] = *s;
			t[pos] = 0; pos = 0; ++s;
			return mkterm(dict[t]);
		}
		if (*s == L'=' && *(s+1) == L'>') {
			++++s;
			return mkterm(implies);
		}
		PROCEED;
		string iri = wstrim(t);
		if (lower(iri) == L"true") return mkterm(dict[L"true"]);
		if (lower(iri) == L"false") return mkterm(dict[L"false"]);
		return mkterm(dict[iri]);
	}

	term* readbnode() { SKIPWS; if (*s != L'_' || *(s+1) != L':') return 0; PROCEED; return mkterm(dict[wstrim(t)]); } 
	term* readvar() { SKIPWS; RETIFN(L'?'); PROCEED; return mkterm(dict[wstrim(t)]); }

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
//boost::replace_all(t1, L"\\\\", L"\\");
		return mkterm(dict[wstrim(t1)]);//, pstrtrim(dt), pstrtrim(lang);
	}

	term* readany(bool lit = true) {
		term* pn;
		return ((pn = readbnode()) || (pn = readvar()) || (lit && (pn = readlit())) || (pn = readlist()) || (pn = readiri()) ) ? pn : 0;
	}

	void addhead(termset& ts, term* t) {
		if (!t) throw 0;
		if (!t->p) throw 0;
//		if (!t->s != !t->p) throw 0;
		ts.push_back(t);
	}

	termset operator()(const wchar_t* _s) {
		//typedef vector<std::pair<term*, termset> > preds_t;
		typedef map<term*, termset> preds_t;
		preds_t preds;
		s = _s;
//		string graph;
		term *subject, *pn;
		termset subjs, objs, heads;
		pos = 0;
//		auto pos1 = preds.back();
		termset dummy;
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
//				graph = dict[pn->p];
			} //else graph = ctx;
			SKIPWS; while (*s == '.') ++s; SKIPWS;
			if (*s == L')') EPARSE(L"expected ) outside list: ");
			if (subject)
				for (auto x = preds.begin(); x != preds.end(); ++x)
					for (termset::iterator o = x->second.begin(); o != x->second.end(); ++o) {
						termset ts;
						ts.push_back(subject);
						ts.push_back(*o);
						addhead(heads, mkterm(x->first->p, ts));
					}
			else for (termset::iterator o = objs.begin(); o != objs.end(); ++o) addhead(heads, (*o)->addbody(subjs));
			if (*s == L'}') { ++s; return heads; }
			preds.clear();
			subjs.clear();
			objs.clear();
		}
		return heads;
	}
};

termset readterms ( std::wistream& is) {
	string s, c;
	nqparser p;
	std::wstringstream ss;
	while (getline(is, s)) {
		trim(s);
		if (s[0] == '#') continue;
		if (startsWith(s, L"fin") && wstrim(s.c_str() + 3) == L".") break;
		ss << ' ' << s << ' ';
	}
	return p((wchar_t*)ss.str().c_str());
}

string formatlist(const term* t, bool in = false) {
	if (!t) return L"";
	if (t->p != Dot)
		throw 0;
	std::wstringstream ss;
	if (!in) ss << L'(';
	ss << format(t->args, true);// << L' ' << format(t->s) << L' ';
	if (!in) ss << L')';
	return ss.str();
}

string format(const term* t, bool body) {
	if (!t || !t->p) return L"";
	std::wstringstream ss;
	if (body && t->p == implies) {
		ss << L'{';
		for (term::bvec::iterator x = t->body.begin(); x != t->body.end(); ++x) ss << format((*x)->t) << L';';
		ss << L'}';
		ss << format(t, false);
	}
	else if (!t->p) return L"";
//	else if (t->p != Dot) {
		ss << dict[t->p];
		if (t->args.size()) {
			ss << L'(';
			for (term* y : t->args) ss << format(y) << L' ';
			ss << L") ";
		}
//	}
//	else return formatlist(t);
	return ss.str();
}

string format(const subs& s) {
	std::wstringstream ss;
	vector<mapelem<resid, term*> > v((const vector<mapelem<resid, term*> >&)s);
	for (size_t n = 0; n < s.size(); ++n)
		ss << dict[v[n].first] << L'\\' << format(v[n].second) << L' ';
	return ss.str();
}

#define IDENT for (int n = 0; n < dep; ++n) ss << L'\t'

string format(const termset& t, int dep) {
	std::wstringstream ss;
	for (termset::iterator _x = t.begin(); _x != t.end(); ++_x) {
		term* x = *_x;
		if (!x || !x->p) continue;
		IDENT;
		ss << format(x, true);
		if (x->body.size()) 
			ss << L" implied by: ";
		else
			ss <<  L" a fact.";
		ss << std::endl;
		for (term::bvec::iterator y = x->body.begin(); y != x->body.end(); ++y) {
			IDENT;
			ss << L"\t" << format((*y)->t, true) << L" matches to heads:" << std::endl;
			for (termset::iterator z = (*y)->matches.begin(); z != (*y)->matches.end(); ++z) {
				IDENT;
				ss << L"\t\t" << format(*z, true) << std::endl;
			}
		}
	}
	return ss.str();
}

typedef vector<mapelem<term*, subs> > ground;
typedef map<resid, vector<mapelem<term*, ground> > > evidence;

struct frame {
	term* rule;
	term::bvec::iterator b;
	sp_frame prev, creator, next;
	subs s;
	ground g() const { // calculate the ground
		if (!creator.p) return ground();
		ground r = creator.p->g();
		termset empty;
		frame& cp = *creator.p;
		typedef mapelem<term*, subs> elem;
		if (btterm)
			r.push_back(elem(btterm, subs()));
		else if (cp.b != cp.rule->body.end()) {
			if (!rule->body.size())
				r.push_back(elem(rule, subs()));
		} else if (cp.rule->body.size())
			r.push_back(elem(creator.p->rule, creator.p->s));
		return r;	
	}
	term* btterm;
	frame(sp_frame c, term* r, term::bvec::iterator* l, sp_frame p, const subs&  _s = subs())
		: rule(r), b(l ? *l : rule->body.begin()), prev(p), creator(c), s(_s), btterm(0) { }
	frame(sp_frame c, frame& p, const subs& _s) 
		: rule(p.rule), b(p.b), prev(p.prev), creator(c), s(_s), btterm(0) { }
};
size_t steps = 0;

sp_frame prove(sp_frame _p, sp_frame& lastp) {
	if (!lastp.p) lastp = _p;
	evidence e;
	while (_p.p) {
		if (++steps % 1000000 == 0) (dout << "step: " << steps << std::endl);
		sp_frame ep = _p;
		frame& p = *_p.p;
		term* t = p.rule, *epr;
		if (t) { // check for euler path
			while ((ep = ep.p->prev).p) {
				epr = ep->rule;
				if (epr == p.rule && epr->unify_ep(*epr, ep->s, *t, p.s)) {
					_p = _p->next;
					t = 0;
					break;
				}
			}
			if (!t) { _p = p.next; continue; }
		}
		if (p.b != p.rule->body.end()) (*p.b)->t->_unify(p.s, (*p.b)->matches, _p, lastp);
		else if (!p.prev.p) {
			#ifndef NOTRACE
			term* _t; // push evidence
			for (term::bvec::iterator r = p.rule->body.begin(); r != p.rule->body.end(); ++r) {
				if (!(_t = ((*r)->t->evaluate(*(*r)->t, p.s)))) continue;
				e[_t->p].push_back(mapelem<term*, ground>(_t, p.g()));
				dout << "proved: " << format(_t) << std::endl;
			}
			#endif
		}
		else { // if body is done, go up the tree
			frame& ppr = *p.prev;
			sp_frame r(new frame(_p, ppr, ppr.s));
			p.rule->unify(*p.rule, p.s, *(*((r->b)++))->t, r->s);
			prove(r, lastp);
		}
		_p = p.next;
	}
//	dout << "steps: " << steps << std::endl;
	return sp_frame();
}

void term::_unify(const subs& ssub, termset& ts, sp_frame f, sp_frame& lastp) {
	subs dsub;
	for (term* _d : ts)  {
		term& d = *_d;
		if (args.size() != d.args.size()) continue;
		termset::iterator it = args.begin(), end = args.end(), dit = d.args.begin();
		while (it != end) {
			term& x = **it++;
			if (!x.unify(x, ssub, **dit++, dsub)) {
				--it;
				break;
			}
		}
		if (it == end) 
			lastp = lastp->next = sp_frame(new frame(f, _d, 0, f, dsub));
		dsub.clear();
	}
}

int main() {
	dict.init();
	termset kb = readterms(din);
	termset query = readterms(din);

	// create the query term and index it wrt the kb
	term* q = mkterm(kb, query);
	// now index the kb itself wrt itself
	for (termset::iterator it = kb.begin(); it != kb.end(); ++it)
		(*it)->trymatch(kb);
	kb.push_back(q);
	TRACE(dout << "kb:" << std::endl << format(kb) << std::endl);

	// create the initial frame with the query term as the frame's rule
	sp_frame p(new frame(sp_frame(), q, 0, sp_frame(), subs())), lastp;
	clock_t begin = clock(), end;
	prove(p, lastp); // the main loop
	end = clock();
	dout << "steps: " << steps << " elapsed: " << (1000. * double(end - begin) / CLOCKS_PER_SEC) << "ms" << std::endl;

	return 0;
}
