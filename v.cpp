//#include <utility>
#include <functional>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include "containers.h"
using std::function;
using std::wostream;
using std::wstring;
using std::wstringstream;
using std::endl;
using std::wcin;
#define isvar(x) ((x).type == L'?')
#define islist(x) ((x).type == L'.')
wostream& dout = std::wcout;
#define FOR(x, y) for (int x = 0; x < (int)y; ++x)

struct res { // iri, var, or list
	wchar_t type; // the only member used in runtime
	union {
		const res** args; // list items, last item must be null.
		const wchar_t *val;
	};
	res(const wchar_t *v) : type(*v), val(wcsdup(v)) { }
	res(vector<const res*> _args)
		: type(L'.'), args(0) {
		int n = 0;
		for (auto x : _args) args[n++] = x;
		args[n] = 0;
	}
	~res() { if (args && type == L'.') delete[] args; if (val) free((wchar_t*)val); }
};

struct triple {
	const res *r[3]; // spo
	triple(const res *s, const res *p, const res *o) { r[0] = s, r[1] = p, r[2] = o; }
	triple(const triple& t) : triple(t.r[0], t.r[1], t.r[2]) {}
};

typedef vector<const triple*> triples;

struct rule {
	const triple* head;
	triples body;
	rule(const triple* h, const triples& b = triples()) : head(h), body(b) {}
	rule(const rule& r) : head(r.head), body(r.body) {}
};
vector<rule> rules;

wostream& operator<<(wostream&, const res&);
wostream& operator<<(wostream&, const triple&t);
wostream& operator<<(wostream&, const struct vm&);
wostream& operator<<(wostream&, const rule&);

// uniquely create resources and triples
const res* mkres(const wchar_t* v) { 
	static map<wstring, const res*> r; 
	static map<wstring, const res*>::iterator i; 
	return ((i = r.find(wstring(v))) == r.end()) ? r.set(v, new res(v)) : i->second; 
}

const res* mkres(const vector<const res*>& v) { 
	static map<vector<const res*>, const res*> r; 
	static map<vector<const res*>, const res*>::iterator i; 
	return ((i = r.find(v)) == r.end()) ? r.set(v, new res(v)) : i->second; 
}

const triple* mktriple(const res *s, const res *p, const res *o) {
	static map<const res*, map<const res*, map<const res*, const triple*>>> spo;
	static map<const res*, map<const res*, map<const res*, const triple*>>>::iterator i;
	static map<const res*, map<const res*, const triple*>>::iterator j;
	static map<const res*, const triple*>::iterator k;
	static const triple* r;
	if ((i = spo.find(s)) == spo.end())
		r = spo.set(s).set(p).set(o, new triple(s, p, o));
	else {
		if ((j = i->second.find(p)) == i->second.end())
				r = i->second[p][o] = new triple(s, p, o);
		else {
			if ((k = j->second.find(o)) == j->second.end())
				r = j->second[o] = new triple(s, p, o);
			else r = k->second;
	}	}
	dout << "new triple: " << *r << endl;
	return r;
}

bool occurs_check(const res *x, const res *y);
struct vm {
	typedef list<const res*> row;
	// struct eq expresses that all listed
	// resources have to be equal: x=?y=?z=...=?t
	struct eq : row {
		using row::row;
		bool bound() const { return !front(); }
		bool free() const { return front(); }
	};
	list<eq*> eqs;
	// function apply(x,y): try to put x,y at the same row and keep the state valid.
	// on our context that means: mark them as required them to be equal.
	// rules of state validation:
	// 1. all rows must contain at most one nonvar.
	// 2. nonvars must be the second element in a row, while
	// the first element in a row containing a nonvar must be null.
	// this is a referred as a bounded row. nulls must not appear anywhere else.
	// 3. every element appears at most once in at most one row.
	// 4. once element was put in a row, it cannot be moved, except for merging 
	// whole rows in a way that doesn't violate the above conditions.
	bool apply(const res* x, bool xvar, const res* y, bool yvar,
			bool checkOnly = false, bool dontEvenCheck = false) {
		// TODO: the condition at the following line must be checked
		// externally ahead of time and not rechecked here.
		list<eq*>::iterator n, k;
		findrow(x, n, y, k);
		if (dontEvenCheck || !check(xvar, yvar, *n, *k)) return false;
		return checkOnly || apply(x, n, xvar, y, k, yvar);
	}
	list<eq*>::iterator findrow(const res* x) {
		for (list<eq*>::iterator e = eqs.begin(); e != eqs.end(); ++e)
			for (const res* r : **e) if (r == x) return e;
		return eqs.end();
	}
	void findrow(const res* x, list<eq*>::iterator& n, const res* y, list<eq*>::iterator& k) {
		char f = 0;
		n = k = eqs.end();
		for (list<eq*>::iterator e = eqs.begin(); e != eqs.end() && f < 2; ++e)
			for (const res* r : **e)
				if 	(r == x){ n = e; ++f; }
				else if (r == y){ k = e; ++f; }
	}
	bool apply(const res* x, list<eq*>::iterator& itn, bool xvar, const res* y, list<eq*>::iterator& itk, bool yvar) {
		eq *n = *itn, *k = *itk;
		// at the following logic we take
		// into account that check() returned
		// true so we avoid duplicate checks.
		if (!n) { // x never appears
			if (!k) { // y never appears
				if (!xvar) eqs.push_front(new eq(0, x, y));
				else if (!yvar) eqs.push_front(new eq(0, y, x));
				else eqs.push_front(new eq(x, y));
			} else	// if y appears and x doesn't, push x to y's row
				// (we can assume we may do that since check() succeeded)
				push(x, xvar, *k);
		} else if (!k) // relying on check(), if x appears and y doesn't, 
			push(y, yvar, *n);// push y to x's row
		else if (n != k) // if x,y appear at different rows, we can safely merge
			merge(itn, itk); // these two rows (again relying on check())
		return true;
	}
	void clear() { eqs = list<eq*>(); }
private:	
	void push(const res* x, bool var, eq& e) { var ? e.push_back(x) : e.push_front(x), e.push_front(0); }
	void merge(list<eq*>::iterator& n, list<eq*>::iterator& k) {
		(*n)->free() ? (*k)->splice(eqs.erase(n)) : (*n)->splice(eqs.erase(k));
	}
	bool check(bool xvar, bool yvar, eq *n, eq *k) {
		// if x never appears, require that y never appears, or
		// that x is a variable (hence unbounded since it never appears),
		// or that y's row is unbounded (so even nonvar x can be appended
		// to it and bind all vars in it)
		if (!n) return !k || xvar || k->free();
		// if k never appears but x appears, require that
		// either y is a var or that y's row is unbounded
		if (!k) return yvar || n->free();
		// if x,y both appear, require that x,y to either appear at the same
		// row, or that one of the rows they appear at is unbounded.
		return n == k || n->front() || k->free();
	}
//	bool apply(const vm& v) { }
};

struct din_t {
	wchar_t ch;
	bool f = false;
	wchar_t peek() 	{ return f ? ch : ((f = true), (wcin >> ch), ch); }// due to wcin.peek() not working
	wchar_t get() 	{ return f ? ((f = false), ch) : ((wcin >> ch), ch); }
	bool good() 	{ return wcin.good(); }
	void skip() 	{ while (good() && iswspace(peek())) get(); }
	wstring getline(){ wstring s; f = false, std::getline(wcin, s); return s; }
	din_t() 	{ wcin >> std::noskipws; }

	wstring& trim(wstring& s) {
		static wstring::iterator i = s.begin();
		while (iswspace(*i)) s.erase(i), i = s.begin();
		size_t n = s.size();
		if (n) {
			while (iswspace(s[--n]));
			s = s.substr(0, ++n);
		}
		return s;
	}
	wstring edelims = L")}.";
	const wchar_t* till() { 
		skip();
		wstringstream ws;
		while 	(good() &&
			edelims.find(peek()) == wstring::npos && 
			!iswspace(peek()))
			ws << get();
		const wchar_t *r = wcsdup(ws.str().c_str());
		return wcslen(r) ? r : 0;
	}
	const res* readlist() {
		get();
		vector<const res*> items;
		while (skip(), (good() && peek() != L')'))
			items.push_back(readany()), skip();
		return get(), skip(), mkres(items);
	}
	const res* readany() {
		if (skip(), !good()) return 0;
		if (peek() == L'(') return readlist();
		const wchar_t *s = till();
		return s ? mkres(s) : 0;
	}
	const triple* readtriple() {
		const res *s, *p, *o;
		if (skip(), !(s = readany()) || !(p = readany()) || !(o = readany())) return 0;
		const triple* r = mktriple(s, p, o);
		if (skip(), peek() == L'.') get(), skip();
		return r;
	}
	void readdoc() { // TODO: support fin notation too. also support prefixes and remarks.
		const triple *t;
		while (good()) {
			triples body;
			if (skip(), peek() == L'{') {
				get();
				while (good() && peek() != L'}') body.push_back(readtriple());
				get(), skip(), get(), get(), skip(), get(), skip(); // "} => {";
				if (peek() == L'}') rules.push_back(rule(0, body)), skip();
				else while (good() && peek() != L'}') rules.push_back(rule(readtriple(), body));
				get();
			} else if ((t = readtriple()))
				rules.push_back(rule(t));
			if (skip(), peek() == L'.')
				get(), skip();
}	}	} din;

// serializers
wostream& operator<<(wostream& os, const rule& r) {
	os << L'{';
	for (const triple* t : r.body) os << *t << ' ';
	return os << L"} => { ", (r.head ? os << *r.head : os << L""), os << " }.";
}
wostream& operator<<(wostream& os, const triple& t) {
	return os << *t.r[0] << ' ' << *t.r[1] << ' ' << *t.r[2] << L'.';
}
wostream& operator<<(wostream& os, const res& r) {
	if (!islist(r)) return os << r.val;
	os << L'(';
	const res **a = r.args;
	while (*a) os << (**a++) << L' ';
	return os << L')'; 
}
wostream& operator<<(wostream& os, const vm& r) {
	for (auto e : r.eqs) {
		for (auto x : *e) os << *x << '=';
		os << ';';
	}
	return os;
}
void print() { dout << "rules: " << rules.size() << endl; for (auto r : rules) dout << r << endl; } 

const size_t max_frames = 1e+6;
const size_t max_gnd = 1e+5;
struct frame {
	int h, b; // head and body ids
	frame* prev;
	vm trail;
};

list<frame> proof;
list<frame*> gnd;

map<int/*head*/,map<int/*body*/, map<int/*head*/, vm>>> intermediate;
map<int/*head*/,map<int/*body*/, std::function<void()>>> program;

// var cannot be bound to something that refers to it
bool occurs_check(const res *x, const res *y) {
	if (x == y) return true;
	if (islist(*y))
		for (const res **r = y->args; *r; ++r)
			if (occurs_check(x, *r))
				return true;
	return false;
}

bool prepare(const res *x, const res *y, vm& c) {
	if (x == y) return true;
	bool xvar = isvar(*x), yvar = isvar(*y);
	if (xvar && occurs_check(x, y)) return false;
	if (yvar && occurs_check(y, x)) return false;
	if (!xvar && !yvar) {
		if (!islist(*x) || !islist(*y))
			return false;
		const res **rx, **ry;
		for (rx = x->args, ry = y->args; *rx && *ry; ++rx, ++ry)
			if (!*rx != !*ry || !prepare(*rx, *ry, c))
				return false;
	}
	return c.apply(x, xvar, y, yvar);
}

// calculate conditions given two triples
bool prepare(const triple *s, const triple *d, vm& c) {
	if (!d) return false;
	dout << "preparing " << *s << " and " << *d;
	FOR(n, 3)
		if (!prepare(s->r[n], d->r[n], c))
			return dout << " failed." << endl, c.clear(), false;
	dout << " passed with conds: " << c << endl;
	return true;
}

// calculate conditions for kb
void prepare() {
	vm c;
	FOR(n, rules.size())
		FOR(k, rules[n].body.size())
			FOR(m, rules.size())
				if (prepare(rules[n].body[k], rules[m].head, c))
					intermediate[n][k][m] = c, c.clear();
}
/*
int *bsizes;

// return a function that pushes new frames according to the static
// conditions and the dynamic frame
function<void()> compile(conds& _c) {
	map<int, function<bool()>> r;
	// we break the body's function to many functions
	// per head, in order to efficiently update the compiled
	// code when one rule changes
	for (conds::const_iterator x = _c.begin(); x != _c.end(); ++x) {
		int h = x->first;
		const condset& c = x->second;
		r[h] = [c, h]() {
			const res *u, *v;
			for (auto y : get<0>(c)) {
				u = y.first, v = y.second;
				if (!first->trail.apply(u, isvar(*u), v, isvar(*v), true))
					return false;
			}
			for (auto y : get<1>(c)) {
				u = y.first, v = y.second;
				if (!first->trail.apply(u, isvar(*u), v, isvar(*v), true))
					return false;
			}
			for (auto y : get<2>(c)) {
				u = y.first, v = y.second;
				if (!first->trail.apply(u, isvar(*u), v, isvar(*v), true))
					return false;
			}
			last->h = h, last->b = 0, last->prev = first, ++last;
			return true;
	};	}
	return [r]() {
		// TODO: EP. hence need to compile a func for every head to unify with itself.
		for (auto x : r) {
			++last;
			if (!x.second()) --last;
		}
		if (first->b == bsizes[first->h]) {
			if (!first->prev)
				*gnd++ = first;
			else // TODO: unify in the opposite direction here (as in euler)
				++(*++last = *first).b;
		}
	};
}

// compile whole kb
void compile() {
	prepare();
	bsizes = new int[rules.size()];
	FOR(n, rules.size())
		if (!(bsizes[n] = rules[n].body.size()))
			program[n][0] = [](){return true;};// fact
		else FOR(k, bsizes[n])
			program[n][k] = compile(intermediate[n][k]);
}

void run() {
	last = first;
	FOR(n, rules.size())
		if (!rules[n].head) // push queries
			(++last)->h = n, last->b = 0, last->prev = 0;
	do {
		// update env, safely assuming it's a valig merge (otherwise
		// this frame wouldn't be created). this has to be compiled too,
		// applying static resources according to body-head pair.
		// we also compile a function per body that pushes the frames
		// by calling check() for statically-coded resources.
		// conversly we can compile one func with one bool param (check/apply)
		// and a goto. the check/apply calls and params are identical, though
		// being called on different locations in the proof flow.
		if (first->prev) first->v.apply(first->prev->v, false, true);
		// execute the func that is indexed by the current frame's head&body id
		program[first->h][first->b]();
		// TODO: run a proof trace collector thread here
		// as well as garbage collector.
	} while (++first <= last);
}

int main() { readdoc(), print(), compile(), run(); }
*/
