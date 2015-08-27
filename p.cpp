#include <cstdlib>
#include <map>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <list>
#include <ctime>
#include <algorithm>
#include <functional>

template<typename T>
class vector {
protected:
	T* a;
	size_t n;
public:
	vector() : a(0), n(0) {}
	vector(const vector<T>& t) : a(0), n(0) { copyfrom(t); }
	vector<T>& operator=(const vector<T>& t) { copyfrom(t); return *this; }

	typedef T* iterator;
	T operator[](size_t k) const { return a[k]; }
	size_t size() const	{ return n; }
	T* begin() const	{ return a; }
	T* end() const		{ return a ? &a[n] : 0; }
	void clear()		{ if (!a) return; free(a); a = 0; n = 0; }
	bool empty()		{ return !a; }
	~vector()		{ clear(); }

	void push_back(const T& t) {
		if (!(a = (T*)realloc(a, sizeof(T) * ++n))) throw std::runtime_error("Allocation failed.");
		new (&a[n-1]) (T);
		a[n-1] = t;
	}
protected:	
	void copyfrom(const vector<T>& t) {
		clear();
		if (!(n = t.n)) return; 
		memcpy(a = (T*)realloc(a, sizeof(T) * n), t.a, sizeof(T) * n);
	}
};

template<typename K, typename V>
struct mapelem {
	K first;
	V second;
	mapelem(){}
	mapelem(const K& _k, const V& _v) : first(_k), second(_v) {} 
};

template<typename K, typename V>
struct map : public vector<mapelem<K, V> > {
	typedef mapelem<K, V> vtype;
	typedef vector<mapelem<K, V> > base;
public:
	map() : base() {}
	map(const map<K, V>& _s) : base()      	{ base::copyfrom(_s); }
	V get(const K& k) const			{ mapelem<K, V>* z = find(k); if (!z) return V(); return z->second; }
	map<K, V>& operator=(const map<K, V>& m){ base::copyfrom(m); return *this; }
	V operator[](const K& k) const		{ return get(k); }
	void clear()				{ base::clear(); }
	size_t size() const			{ return base::size(); }
	void set(const K& k, const V& v) {
		vtype* z = find(k);
		if (z) { z->second = v; return; }
		base::push_back(vtype(k, v));
		qsort(base::a, base::n, sizeof(vtype), compare);
	}
	typedef vtype* iterator;
	iterator find(const K& k) const {
		vtype v(k, V());
		vtype* z = (vtype*)bsearch(&v, base::a, base::n, sizeof(vtype), compare);
		return z;
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
	std::map<resid, string> ip;
	std::map<string, resid> pi;
public:
	void init() {
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
	term(resid _p, const termset& _args = termset()) : p(_p), args(_args) { if (!p) throw 0; init();  }

	resid p;
	termset args;
//	term *s, *o;
	struct body_t {
		friend struct term;
		term* t;
		bool state;
		body_t(term* _t) : t(_t), state(false) {}
		termset matches;
		termset::iterator it;
		subs ds;
		// a small coroutine that returns all indexed head that match also given a subst.
		// this process is what happens during inference rather index.
		bool operator()(const subs& s) {
			if (!state) { it = matches.begin(); state = true; }
			else { ++it; ds.clear(); }
			while (it != matches.end()) {
				TRACE(dout << "matching " << format(t, true) << " with " << format(*it, true) << "... ");

				if (it != matches.end() && t->unify(s, *it, ds)) { 
					TRACE(dout << " passed" << std::endl); 
					return state = true; 
				}
				else { 
					ds.clear(), ++it; 
					TRACE(dout << " failed" << std::endl); 
					continue; 
				}
			}
			return state = false;
		}
	private:
		void addmatch(term* x) { // indexer: add matching rule's head
			TRACE(dout << "added match " << format(x) << " to " << format(t) << std::endl);
			matches.push_back(x);
		}
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
	}
	term* evaluate(const subs& ss) {
		static term *v, *r;
		if (p < 0) {
			v = ss[p];
			if (v)
				r = v->evaluate(ss);
			else
				r = 0;
		} else if (args.empty())
			r = this;
		else {
			termset ts;
			for (term* a : args) {
				if ((v = a->evaluate(ss))) ts.push_back(v);
				else ts.push_back(mkterm(a->p));
			}
			return mkterm(p, ts);
		}
		TRACE(dout<<"evaluate " << format(this) << " under " << format(ss) << " returned " << format(r) << std::endl);
		return r;
	};
	bool _unify(const subs& ssub, term& d, subs& dsub) {
		static term* v;
		if (p < 0) {
			if ((v = evaluate(ssub))) return v->unify(ssub, &d, dsub);
			return true;
		} else if (d.p < 0) {
			if ((v = d.evaluate(dsub))) return unify(ssub, v, dsub);
			dsub.set(d.p, v = evaluate(ssub));
			TRACE(dout << "new sub: " << dict[d.p] << '\\' << format(v) << std::endl);
			return true;
		}
		size_t sz = args.size();
		if (p != d.p || sz != d.args.size()) return false;
		for (size_t n = 0; n < sz; ++n)
			if (!args[n]->unify(ssub, d.args[n], dsub)) 
				return false;
		return true;
	}
	bool _unify_ep(const subs& ssub, term& d, const subs& dsub) {
		static term* v;
		if (p < 0) 	return (v = evaluate(ssub)) ? v->unify_ep(ssub, &d, dsub) : true;
		if (d.p < 0) 	return ((v = d.evaluate(dsub))) ? unify_ep(ssub, v, dsub) : true;
		size_t sz = args.size();
		if (p != d.p || sz != d.args.size()) return false;
		for (size_t n = 0; n < sz; ++n)
			if (!args[n]->unify_ep(ssub, d.args[n], dsub)) 
				return false;
		return true;
	}
	void init() {}
	bool unify_ep(const subs& ssub, term* _d, const subs& dsub) {
		return !(!_d || !_d->p || _d->p == implies) && _unify_ep(ssub, *_d, dsub);
	}
	bool unify(const subs& ssub, term* _d, subs& dsub) {
		return !(!_d || !_d->p || _d->p == implies) && _unify(ssub, *_d, dsub);
	}
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
		if (b.t->unify(subs(), t, d)) b.addmatch(t);
		d.clear();
	}
};

term* mkterm() { return new term; }
term* mkterm(termset& kb, termset& query) { return new term(kb, query); }
term* mkterm(resid p, const termset& args) { return new term(p, args); }
term* mkterm(resid p) { return new term(p); }

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
		term *head = mkterm(Dot), *pn = head;
		while (*s != L')') {
			SKIPWS;
			if (*s == L')') break;
			if (!(pn = readany(true)))
				EPARSE(L"couldn't read next list item: ");
			head->args.push_back(pn);
			SKIPWS;
//			pn = pn->o = mkterm(Dot);
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
	vector<mapelem<resid, term*> > v(s);
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

template<typename T>
struct sp { // smart pointer
	T* p;
	struct ref {
		size_t r; ref() : r(0) {}
		void inc() { ++r; }
		size_t dec() { return --r; }
	} *r;
	sp() : p(0) 				{ (r = new ref)->inc(); }
	sp(T* v) : p(v) 			{ (r = new ref)->inc(); }
	sp(const sp<T>& s) : p(s.p), r(s.r) 	{ r->inc(); }

	~sp() { if (!r->dec()) delete p , delete r; }

	T& operator* ()  { return *p; } 
	T* operator-> () { return p; }
    
	sp<T>& operator=(const sp<T>& s) {
		if (this == &s) return *this;
		if(!r->dec()) delete p , delete r;
		p = s.p;
		(r = s.r)->inc();
		return *this;
	}
};

typedef sp<struct frame> sp_frame;
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
	frame(sp_frame c, frame& p) 
		: rule(p.rule), b(p.b), prev(p.prev), creator(c), btterm(0) { }
};
size_t steps = 0;

sp_frame prove(sp_frame _p, sp_frame& lastp) {
	if (!lastp.p) lastp = _p;
	evidence e;
	while (_p.p) {
		if (++steps % 1000000 == 0) (dout << "step: " << steps << std::endl);
		sp_frame ep = _p;
		frame& p = *_p.p;
		term* t = p.rule;
		if (t) { // check for euler path
			while ((ep = ep.p->prev).p)
				if (ep->rule == p.rule && ep->rule->unify_ep(ep->s, t, p.s)) {
					_p = _p->next;
					t = 0;
					break;
				}
			if (!t) { _p = p.next; continue; }
		}
		if (p.b != p.rule->body.end()) {
			term::body_t& pb = **p.b;
			// ask the body item's term to try to match to its indexed heads
			while (pb(p.s))
				lastp = lastp->next = sp_frame(new frame(_p, *pb.it, 0, _p, pb.ds));
		}
		else if (!p.prev.p) {
			#ifndef NOTRACE
			term* _t; // push evidence
			for (term::bvec::iterator r = p.rule->body.begin(); r != p.rule->body.end(); ++r) {
				if (!(_t = ((*r)->t->evaluate(p.s)))) continue;
				e[_t->p].push_back(mapelem<term*, ground>(_t, p.g()));
				dout << "proved: " << format(_t) << std::endl;
			}
			#endif
		}
		else { // if body is done, go up the tree
			frame& ppr = *p.prev;
			sp_frame r(new frame(_p, ppr));
			r->s = ppr.s;
			p.rule->unify(p.s, (*r->b)->t, r->s);
			++r->b;
			prove(r, lastp);
		}
		_p = p.next;
	}
//	dout << "steps: " << steps << std::endl;
	return sp_frame();
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
