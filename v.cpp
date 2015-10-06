#include <map>
#include <vector>
#include <utility>
#include <functional>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
//#include <list>
using namespace std;

wstring edelims = L")}.";
#define isvar(x) ((x).type == L'?')
#define islist(x) ((x).type == L'.')
wostream& dout = wcout;
#define FOR(x, y) for (int x = 0; x < (int)y; ++x)

struct din_t { // due to wcin.peek() not working
	wchar_t ch;
	bool f = false;
	wchar_t peek() { return f ? ch : ((f = true), (wcin >> ch), ch); }
	wchar_t get() { return f ? ((f = false), ch) : ((wcin >> ch), ch); }
	bool good() { return wcin.good(); }
	void skip() { while (good() && iswspace(peek())) get(); }
	wstring getline() { wstring s; f = false, std::getline(wcin, s); return s; }
	din_t() { wcin >> noskipws; }
} din;

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

const wchar_t* till() { // read till delim or space
	din.skip();
	wstringstream ws;
	while 	(din.good() &&
		edelims.find(din.peek()) == string::npos && 
		!iswspace(din.peek()))
		ws << din.get();
	const wchar_t *r = wcsdup(ws.str().c_str());
	return wcslen(r) ? r : 0;
}

struct res { // iri, var, or list
	wchar_t type; // the only member used in runtime
	union {
		const res** args; // list items, last item must be null.
		const wchar_t *val;
	};
	res(const wchar_t *v) : type(*v), val(wcsdup(v)) { }
	res(vector<res*> _args)
		: type(L'.'), args(new const res*[_args.size() + 1]) {
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
	const triples body;
	rule(const triple* h, const triples& b = triples()) : head(h), body(b) {}
};
vector<rule> rules;

// following list isn't very robust but is minimal
// and efficient due to making some assumptions like
// list cannot be empty
template<typename T>
class list {
	struct impl {
		T t;
		impl *next;
		impl(const T& _t) : t(_t), next(0) {}
		impl(const T& _t, impl* n) : t(_t), next(n) {}
		~impl() { if (next) delete next; }
	} *first = 0, *last = 0;
	bool dontdel = false;
public:
	list(const T& t) : first(new impl(t)), last(first) {}
	void push_back(const T& t) { last = last->next = new impl(t); }
	void push_front(const T& t) { first = new impl(t, first); }
	T& back() { return last->t; }
	T& front() { return first->t; }
	void appendelete(list<T>* l) { l.dontdel = true, last->next = l->first, delete l; }
	~list() { if (!dontdel) delete first; }
};

bool occurs_check(const res *x, const res *y);

class vm {
	typedef list<const res*> eq; 	// "row", representing that all listed
					// resources have to be equal: x=?y=?z=...=?t
	list<eq> eqs;
public:
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
	bool apply(const res* x, bool xvar, const res* y, bool yvar, bool checkOnly = false, bool dontEvenCheck = false) {
		// TODO: the condition at the following line must be checked
		// externally ahead of time and not rechecked here.
		if (x == y || occurs_check(x, y)) return false;
		eq *n = 0, *k = 0;
		// TODO: optimize. many times the caller already
		// has (or can have) n or even n&k in hand.
		for (list<eq>::iterator e = eqs.begin(), e != eqs.end() && (!n || !k), ++e)
			for (const res* r : *e)
				if 	(r == x){ n = *e; break; }
				else if (r == y){ k = *e; break; }
		if (dontEvenCheck || !check(xvar, yvar, n, k)) return false;
		if (checkOnly) return true;
		// at the following logic we take
		// into account that check() returned
		// true so we avoid duplicate checks.
		if (!n) { // x never appears
			if (!k) { // y never appears
				eq e;
				if (!xvar)	push_nonvar(x, e), e.push_back(y);
				else if (!yvar) push_nonvar(y, e), e.push_back(x); 
				else		e.push_front(x),   e.push_front(y);
				eqs.push_back(e);
			} else	// if y appears and x doesn't, push x to y's row
				// (we can assume we may do that since check() succeeded)
				push(x, xvar, *k);
		} else if (!k) // relying on check(), if x appears and y doesn't, 
			push(y, yvar, *n);// push y to x's row
		else if (n != k) // if x,y appear at different rows, we can safely merge
			merge(n, k); // these two rows (again relying on check())
		return true;
	}
	void clear() { eqs.clear(); }
private:	
	void push(const res* x, bool var, eq& e) { var ? e.push_back(x) : push_nonvar(x, e); }
	void push_nonvar(const res* x, eq& e) { e.push_front(x), e.push_front(0); }
	void merge(eq* n, eq* k) {
		if (!n->front()) k->splice(k->begin(), *n);
		else min(n, k)->splice(min(n, k)->begin(), *max(n, k));
	}
	bool check(bool xvar, bool yvar, eq *n, eq *k) {
		// if x never appears, require that y never appears, or
		// that x is a variable (hence unbounded since it never appears),
		// or that y's row is unbounded (so even nonvar x can be appended
		// to it and bind all vars in it)
		if (!n) return !k || xvar || k->front();
		// if k never appears but x appears, require that
		// either y is a var or that y's row is unbounded
		if (!k) return yvar || n->front();
		// if x,y both appear, require that x,y to either appear at the same
		// row, or that one of the rows they appear at is unbounded.
		return n == k || n->front() || k->front();
	}
	bool apply(const vm& v) {

	}
};

wostream& operator<<(wostream&, const res&);
wostream& operator<<(wostream&, const triple&t);
wostream& operator<<(wostream&, const vm&);
wostream& operator<<(wostream&, const rule&);

// uniquely create resources and triples
res* mkres(const wchar_t* v) { 
	static map<wstring, res*> r; 
	static map<wstring, res*>::iterator i; 
	return ((i = r.find(wstring(v))) == r.end()) ? (r[v] = new res(v)) : i->second; 
}

res* mkres(const vector<res*>& v) { 
	static map<vector<res*>, res*> r; 
	static map<vector<res*>, res*>::iterator i; 
	return ((i = r.find(v)) == r.end()) ? (r[v] = new res(v)) : i->second; 
}

triple* mktriple(res *s, res *p, res *o) {
	static map<res*, map<res*, map<res*, triple*>>> spo;
	static map<res*, map<res*, map<res*, triple*>>>::iterator i;
	static map<res*, map<res*, triple*>>::iterator j;
	static map<res*, triple*>::iterator k;
	static triple* r;
	i = spo.find(s);
	if (i == spo.end()) r = spo[s][p][o] = new triple(s, p, o);
	else {
		j = i->second.find(p);
		if (j == i->second.end()) r = i->second[p][o] = new triple(s, p, o);
		else {
			k = j->second.find(o);
			if (k == j->second.end()) r = j->second[o] = new triple(s, p, o);
			else r = k->second;
	}	}
	dout << "new triple: " << *r << endl;
	return r;
}

// serializers
wostream& operator<<(wostream& os, const rule& r) {
	os << L'{';
	for (const triple* t : r.body) os << *t << ' ';
	os << L"} => { ", (r.head ? os << *r.head : os << L""), os << " }.";
	return os;
}
wostream& operator<<(wostream& os, const triple& t) {
	os << *t.r[0] << ' ' << *t.r[1] << ' ' << *t.r[2] << L'.';
	return os;
}
wostream& operator<<(wostream& os, const res& r) {
	if (islist(r)) {
		os << L'(';
		const res **a = r.args;
		while (*a) os << (**a++) << L' ';
		os << L')'; 
	} else os << r.val;
	return os;
}
wostream& operator<<(wostream& os, const vm& r) {
	for (auto e : r.eqs) {
		for (auto x : e) os << *x << '=';
		os << ';';
	}
	return os;
}
void print() { dout << "rules: " << rules.size() << endl; for (auto r : rules) dout << r << endl; } 

// parsers
res* readany();

res* readlist() {
	din.get(), din.skip();
	vector<res*> items;
	while (din.good() && din.peek() != L')') items.push_back(readany()), din.skip();
	din.get(), din.skip();
	return mkres(items);
}

res* readany() {
	din.skip();
	if (!din.good()) return 0;
	if (din.peek() == L'(') return readlist();
	const wchar_t *s = till();
	return s ? mkres(s) : 0;
}

triple* readtriple() {
	din.skip();
	res *s, *p, *o;
	if (!(s = readany()) || !(p = readany()) || !(o = readany())) return 0;
	triple* r = mktriple(s, p, o);
	din.skip();
	if (din.peek() == L'.') din.get(), din.skip();
	return r;
}

// TODO: support fin notation too. also support prefixes and remarks.
void readdoc() {
	while (din.good()) {
		din.skip();
		triples body;
		if (din.peek() == L'{') {
			din.get();
			while (din.good() && din.peek() != L'}') body.push_back(readtriple());
			din.get(), din.skip(), din.get(),
			din.get(), din.skip(), din.get(), din.skip(); // "} => {";
			if (din.peek() == L'}') rules.push_back(rule(0, body)), din.skip();
			else while (din.good() && din.peek() != L'}') rules.push_back(rule(readtriple(), body));
			din.get();
		} else {
			triple *t = readtriple();
			if (t) rules.push_back(rule(t));
		}
		din.skip();
		if (din.peek() == L'.') din.get(), din.skip();
}	}

const size_t max_frames = 1e+6;
const size_t max_gnd = 1e+5;
struct frame {
	int h, b; // head and body ids
	frame* prev;
	vm trail;
} *first = new frame[max_frames], *last, **gnd = new frame*[max_gnd];

typedef tuple<vm, vm, vm> condset;
typedef map<int/*head*/, condset> conds;
// the following tuple is cs, cd, csd
// where cs are conds where s is var and d is nonvar
// cd where d is var and s is nonvar
// csd where both s,d are vars
map<int/*head*/,map<int/*body*/, conds>> intermediate;
map<int/*head*/,map<int/*body*/, std::function<void()>>> program;

// var cannot be bound to something that refers to it
bool occurs_check(const res *x, const res *y) {
	if (!isvar(*x)) return isvar(*y) ? occurs_check(y, x) : false;
	if (x == y)	return true;
	if (islist(*y)) for (const res **r = y->args; *r; ++r)
		if (occurs_check(x, *r)) return true;
	return false;
}

// since they already match in compile time.  TODO: verify subs conflicts.
bool prepare(const res *s, const res *d, vm& cs, vm& cd, vm& csd) {
	bool r = true;
	if (!isvar(*s) && !isvar(*d)) {
		if (s != d) r = false;
		else if (islist(*s)) {
			const res **rs, **rd;
			for (rs = s->args, rd = d->args; *rs && *rd;)
				if (!*rs != !*rd || !prepare(*rs++, *rd++, cs, cd, csd))
					r = false;
		}
	}
	if (r && (r = !occurs_check(s, d))) {
		if (isvar(*s)) {
			if (isvar(*d)) r = csd.apply(s, true, d, true);
			else r = cs.apply(s, true, d, false);
		} else r = cd.apply(s, false, d, true);
	}
	return r;
}

// calculate conditions given two triples
bool prepare(const triple *s, const triple *d, vm& cs, vm& cd, vm& csd) {
	if (!d) return false;
	dout << "preparing " << *s << " and " << *d;
	FOR(n, 3)
		if (!prepare(s->r[n], d->r[n], cs, cd, csd))
			return dout << " failed." << endl, cs.clear(), cd.clear(), csd.clear(), false;
	dout << " passed with conds: " << cs << ' ' << cd << ' ' << csd << endl;
	return true;
}

// calculate conditions for kb
void prepare() {
	vm cs, cd, csd;
	FOR(n, rules.size())
		FOR(k, rules[n].body.size())
			FOR(m, rules.size())
				if (prepare(rules[n].body[k], rules[m].head, cs, cd, csd))
					intermediate[n][k][m] = make_tuple(cs, cd, csd)
					, cs.clear()
					, cd.clear()
					, csd.clear();
}

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
	do { // execute the func that is indexed by the current frame's head&body id
		program[first->h][first->b]();
		// TODO: run a proof trace collector thread here
		// as well as garbage collector.
	} while (++first <= last);
}

int main() { readdoc(), print(), compile(), run(); }
