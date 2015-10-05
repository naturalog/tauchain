#include <map>
#include <vector>
#include <utility>
#include <functional>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
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
	bool isdel;
	while 	(din.good() &&
		(isdel = (edelims.find(din.peek()) == string::npos)) && 
		!iswspace(din.peek()))
		ws << din.get();
	const wchar_t *r = wcsdup(ws.str().c_str());
//	if (isdel) din.get();
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

typedef tuple<subs, subs, subs> condset;
typedef map<int/*head*/, condset> conds;
// the following tuple is cs, cd, csd
// where cs are conds where s is var and d is nonvar
// cd where d is var and s is nonvar
// csd where both s,d are vars
map<int/*head*/,map<int/*body*/, conds>> intermediate;
map<int/*head*/,map<int/*body*/, std::function<void()>>> program;

void print_interm() {
	for (auto x : intermediate) {
		for (auto y : x.second) {
			dout << "Conditions in order for " << *rules[x.first].body[y.first] << ":" << endl;
			for (auto z : y.second)
				dout << "\tto unify with " << *rules[z.first].head << ": " << endl << "\t\t" << get<0>(z.second) << endl << "\t\t" << get<1>(z.second) << endl << "\t\t" << get<2>(z.second) << endl;
}	}	}

bool occurs_check(const res *x, const res *y) {
	if (!isvar(*x)) return isvar(*y) ? occurs_check(y, x) : false;
	if (x == y) return true;
	if (islist(*y)) for (const res **r = y->args; *r; ++r)
		if (occurs_check(x, *r)) return true;
	return false;
}

// calculate conditions given two resources
bool prepare(const res *s, const res *d, subs& cs, subs& cd, subs& csd) {
	dout << "preparing " << *s << " and " << *d << endl;
	if (!isvar(*s) && !isvar(*d)) {
		if (s != d) return dout << " failed." << endl, cs.clear(), cd.clear(), csd.clear(), false;
		if (!islist(*s)) return true;
		const res **rs, **rd;
		for (rs = s->args, rd = d->args; *rs && *rd;)
			if (!*rs != !*rd || !prepare(*rs++, *rd++, cs, cd, csd))
				return false;
		return dout << " passed." << endl, true;
	}
	if (occurs_check(s, d)) return false;
	if (isvar(*s)) {
		if (isvar(*d)) csd[s] = d;
		else cs[s] = d;
	} else cd[s] = d;
	return (dout << " passed " << endl, true);
}

// calculate conditions given two triples
bool prepare(const triple *s, const triple *d, subs& cs, subs& cd, subs& csd) {
	if (!d) return false;
	FOR(n, 3)
		if (!prepare(s->r[n], d->r[n], cs, cd, csd))
			return false;
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
			// so need to keep trail as the subs to be done, then
			// when reaching there, trail will hold subs to be reverted.
			for (auto y : get<0>(c))
				if ((u = evalvar(y.first)))
					if (u != y.second)
						return false;
			for (auto y : get<1>(c))
				if ((u = evalvar(y.second))) {
					if (y.first != u)
						return false;
				} else
					last->trail[y.second] = y.second->sub
					, const_cast<res*>(y.second)->sub = y.first;
			for (auto y : get<2>(c))
				if ((u = evalvar(y.first))) {
					if ((v = evalvar(y.first))) {
						if (u != v)
							return false;
					} else
						last->trail[y.second] = y.second->sub
						, const_cast<res*>(y.second)->sub = u;
				}
			last->h = h, last->b = 0, last->prev = first, ++last;
			return true;
	};	}
	return [r]() {
		// TODO: EP
		for (auto x : r) {
			++last;
			if (!x.second())
				--last;
		}
		// TODO: prepare the sizes in fast access table
		if (first->b == (int)rules[first->h].body.size()) {
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
	FOR(n, rules.size())
		FOR(k, rules[n].body.size())
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

int main() { readdoc(), print(), compile(), print_interm(), run(); }
