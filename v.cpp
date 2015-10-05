#include <map>
#include <vector>
#include <utility>
#include <functional>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <unordered_set>
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
	wchar_t type;
	union {
		const res* sub; // substitution, in case of var. not used in compile time (and will const_cast then)
		// in runtime, only the info up till here is used
		// (in case the specific struct instance is used anyway)
		const res** args; // list items, last item must be null. not used in runtime
	};
	const wchar_t *val;
	res(const wchar_t *v) : type(*v), sub(0), val(wcsdup(v)) { }
	res(vector<res*> _args)
		: type(L'.'), args(new const res*[_args.size() + 1]), val(0) {
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

typedef map<const res*, const res*> subs; // for holding conditions in compile time

wostream& operator<<(wostream&, const res&);
wostream& operator<<(wostream&, const triple&t);
wostream& operator<<(wostream&, const subs&);
wostream& operator<<(wostream&, const struct rule&);

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
	} else {
		os << r.val;
		if (isvar(r) && r.sub) os << L'=' << *r.sub;
	}
	return os;
}
wostream& operator<<(wostream& os, const subs& r) {
	for (auto x : r) os << *x.first << '=' << *x.second << ';';
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
	subs trail;
} *first = new frame[max_frames], *last, **gnd = new frame*[max_gnd];

typedef map<const res*, unordered_set<const res*>> cond_sublist;
typedef tuple<cond_sublist, cond_sublist, cond_sublist> condset;
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

struct vm {
	typedef list<res*> eq; // "cols"
	list<eqs> eqs; // "rows"
	// function apply(x,y): try to put x,y at the same row and keep the state valid,
	// means: require them to be equal.
	// rules of state validation:
	// 1. all rows must contain at most one nonvar.
	// 2. nonvars must be the second element in a row, while
	// the first element in a row containing a nonvar must be null.
	// this is a bounded row. an unbounded row (that contains only
	// vars) must not begin with null.
	// 3. once element was put in a row, it cannot be moved.
	// 4. every element appears at most once in at most one row.
	//
	// note that the res* pointers (for vars or nonvars)
	// are ever dereferenced, serve only as a unique id.
	// all comparisons are done by comparing those addresses
	// therefore the function has to have three specialized versions (or 
	// conversly another argument) to tell if which x,y are vars. though
	// the table itself doesn't need to store this information.
	//
	// given we have a valid state and we want to update
	// another equality condition x=y, we have to execute
	// the following function, called apply(x,y):
	// 1. if x already appears in row n:
	// 	1.1 if y never appears
	// 		1.1.1 if y is a nonvar
	// 			1.1.1 if the row n is unbounded,
	// 			add y to the head of row n and return success.
	// 			1.1.2 fail.
	// 		1.1.2 if y is a var, append it to row n and return success.
	// 	1.2 if y appears at row k
	// 		1.2.1 if n==k, return success.
	// 		1.2.2 if rows n,k are both bounded, fail.
	// 		1.2.3 append all items of the unbounded row to the bounded one
	// 		and delete the old unbounded row. if both rows are unbounded,
	// 		append into row min(n,k). return success.
	// 2. if x never appears
	// 	2.1 if y never appears, put x,y in a new row and return success.
	// 	otherwise, let k by y's row.
	// 	2.2 if x is a var or row k is unbounded, append x to row k and return success.
	// 	2.3 fail
	bool apply(res* x, bool xvar, res* y, bool yvar) {
		if (x == y) return false;
		eq *n = 0, *k = 0; 
		for (eq& e : eqs) {
			for (res* r : e)
				if (r == x) {
					n = &e;
					break;
				} else if (r == y) {
					k = &e;
					break;
				}
			if (n && k) break;
		}
		if (!n) { // x never appears
			if (!k) { // y never appears
				eq e;
				if (!xvar)
					e.push_front(x), e.push_front(0),
					e.push_back(y), eqs.push_back(e);
				else if (!yvar)
					e.push_front(y), e.push_front(0),
					e.push_back(x), eqs.push_back(e);
				else e.push_front(x), e.push_front(y), eqs.push_back(e);
				return true;
			}
			if (xvar) return k->push_back(x), true;
			return k->front() ? k->push_front(x), k->push_front(0), true : false;
		}
		if (!k) {
			if (yvar) return n->push_back(y), true;
			return n->front() ? n->push_front(y), n->push_front(0), true : false;
		}
		if (n == k) return true;
		if (!n->front())
			return k->front() ? k->splice(k->begin(), *n), true : false;
		return min(n, k)->front() ? 
			min(n, k)->splice(min(n, k)->begin(), *max(n, k)), true : false;
	}
};

struct conds_t {
	struct { // head
		struct { // body
			struct head {
				int id;
				struct cond {
					// this represents a condition that s 
					// should be (made) equal to d, otherwise fail.
					res *s, *d; // at least one of them is var.
					cond *next; // conds are stored as a linked list.
					cond() : s(0), d(0), next(0) {}
					bool apply(res* _s, res* _d, bool checkOnly = false) {
						if (s == _s) {
							if (!isvar(d)) {
							       	if (!isvar(_d)} {if (d != _d) return false;}
								else if (!apply(_d, d)) return false;
							}	
						}
						if (checkOnly) return true;
						if (!apply(d, s, true)) return false;
						if (next) return next->apply(c);
						return next->s = _s, next->d = _d, next->next = 0, true;
					}
				} conds;
				head* next; 
				// heads are linked list too since we list only
				// possibly matching ones and might prune ones
				// during inference.
				head() : id(-1), conds(0), next(0);
			} heads;
		} *bodies;
	} *heads;
	void init() {
		heads = malloc(sizeof(*heads) * rules.size());
		for (size_t n = 0; n < rules.size(); ++n)
			heads[n] = malloc(sizeof(*heads[n]) * rules[n].body.size());
	}
};

struct cond_t {
	const res &s, &d;
	bool svar;
	cond_t(const res *_s, const res *_d, cond_t* next = 0) : s(*_s), d(*_d), svar(isvar(s)) {}
	cond_t *next;

	bool prepare(bool checkOnly = false) {
		if (svar) {
			if (!s.sub)
				return checkOnly ? true : 
		}
	};
	function<bool(bool)> compile() {
		return [](bool checkOnly) {
		};
	}
};

bool putcond(const res* s, const res* d, subs& c, bool checkOnly = false) {
	if (s == d) return true;
	subs::iterator it = c.find(s);
	if (it == c.end())
		return checkOnly ? true :
			putcond(d, s, c, true) ? (c.insert(make_pair(s, d)) , true) : false;
	if (!isvar(*it->second)) return false;
	return (checkOnly || putcond(d, s, c, true)) && putcond(it->first, d, c);
}

// calculate conditions given two resources, i.e. only what has to be checked in runtime,
// which practically means only resources matching a variable. example: matching
// (x y z) with (?t y z) will emit "x,?t" in cd. y,z aren't matched in runtime
// since they already match in compile time.  TODO: verify subs conflicts.
bool prepare(const res *s, const res *d, subs& cs, subs& cd, subs& csd) {
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
			if (isvar(*d)) r = putcond(s, d, csd);
			else r = putcond(s, d, cs);
		} else r = putcond(s, d, cd);
	}
	return r;
}

// calculate conditions given two triples
bool prepare(const triple *s, const triple *d, subs& cs, subs& cd, subs& csd) {
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
	subs cs, cd, csd;
	FOR(n, rules.size())
		FOR(k, rules[n].body.size())
			FOR(m, rules.size())
				if (prepare(rules[n].body[k], rules[m].head, cs, cd, csd))
					intermediate[n][k][m] = make_tuple(cs, cd, csd)
					, cs.clear()
					, cd.clear()
					, csd.clear();
}

// runtime variable chain resolving
const res* evaluate(const res* r) { return isvar(*r) ? r->sub ? evaluate(r->sub) : 0 : r; }
const res* evalvar(const res* r) { return r->sub ? evaluate(r->sub) : 0; }
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
			// TODO: revise. need to apply the subs only in entrance
			// i.e. in the func returned from this compile() func.
			// this practically means verifying that existing subs
			// don't contradict the conditions.
			// so need to keep trail as the subs to be done, then
			// when reaching there, trail will hold subs to be reverted.
			// TODO: maybe runtime subs can be encoded as jitted code
			// into existing funcs, efficiently (i.e. only to funcs
			// of terms concerning those vars)? this way all
			// conditions are compiled including runtime's,
			// ultimate memory consumption, and no need
			// the var chain recursion.
			for (auto y : get<0>(c))
				for (auto z : y.second)
					if ((u = evalvar(y.first)) != z && u)
						return false;
			for (auto y : get<1>(c))
				for (auto z : y.second)
					if ((u = evalvar(z)))
						{ if (y.first != u) return false; }
					else last->trail[y.second] = z->sub,
						const_cast<res*>(z)->sub = y.first;
			for (auto y : get<2>(c))
				for (auto z : y.second)
					if ((u = evalvar(y.first))) {
						if ((v = evalvar(y.first)))
							{ if (u != v) return false; }
						else last->trail[z] = z->sub,
							const_cast<res*>(z)->sub = u;
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
