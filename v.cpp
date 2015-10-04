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
#define isvar(x) (*(x).val == L'?')
wostream& dout = wcout;

struct din_t { // due to wcin.peek() not working
	wchar_t ch;
	bool f = false;
	wchar_t peek() {
		if (f)
			return ch;
		return (f = true), (wcin >> ch), ch;
	}
	wchar_t get() {
		if (f) 
			return (f = false), ch;
		return (wcin >> ch), ch;
	}
	bool good() { return wcin.good(); }
	void skip() {
		while (good() && iswspace(peek()))
			get();
	}
	wstring getline() {
		wstring s;
		f = false, std::getline(wcin, s);
		return s;
	}
	din_t() { wcin >> noskipws; }
} din;

bool isdelim(wchar_t ch, wstring d = edelims) {
	for (auto x : d) if (ch == x) return true;
	return false;
}

wchar_t* till(/*wstring& d = edelims*/) {
	din.skip();
	wstringstream ws;
	while (!isdelim(din.peek()) && !iswspace(din.peek()))
		ws << din.get();
	wchar_t *r = wcsdup(ws.str().c_str());
	return din.get(), (wcslen(r) ? r : 0);
}

wostream& operator<<(wostream& os, const struct res& r);

struct res {
	wchar_t *val;
	union {
		res* sub;	// substitution, in case of var. not used in compile time
		res** args;// list items. last item must be null
	};
	res(wchar_t *v) : val(v ? wcsdup(v) : 0), sub(0) { }
	res(vector<res*> _args)
		: val(new wchar_t[2]), args(new res*[_args.size() + 1]) {
		*val= L'.', val[1] = 0;
		int n = 0;
		for (auto x : _args) args[n++] = x;
		args[n] = 0;
	}
	~res() { if (args && *val == L'.') delete[] args; if (val) free(val); }
};

#define _mkres(T, K) \
res* mkres(T v) { \
	static map<K, res*> r; \
	static map<K, res*>::iterator i; \
	return ((i = r.find(K(v))) == r.end()) ? (r[v] = new res(v)) : i->second; \
}
_mkres(wchar_t*, wstring)
_mkres(vector<res*>, vector<res*>)

wostream& operator<<(wostream& os, const res& r) {
	if (*r.val == L'.') {
		os << L'(';
		res **a = r.args;
		while (*a) os << (**a) << L' ';
		os << L')'; 
	} else {
		os << r.val;
		if (isvar(r) && r.sub) os << L'=' << *r.sub;
	}
	return os;
}

struct triple {
	res *r[3]; // spo
	triple(res *s, res *p, res *o) {
		r[0] = s, r[1] = p, r[2] = o; }
	triple(const triple& t) : triple(t.r[0], t.r[1], t.r[2]) {}
};

triple* mktriple(res *s, res *p, res *o) {
	static map<res*, map<res*, map<res*, triple*>>> spo;
	static map<res*, map<res*, map<res*, triple*>>>::iterator i;
	static map<res*, map<res*, triple*>>::iterator j;
	static map<res*, triple*>::iterator k;
	i = spo.find(s);
	if (i == spo.end()) return spo[s][p][o] = new triple(s, p, o);
	j = i->second.find(p);
	if (j == i->second.end()) return i->second[p][o] = new triple(s, p, o);
	k = j->second.find(o);
	if (k == j->second.end()) return j->second[o] = new triple(s, p, o);
	return k->second;
}

wostream& operator<<(wostream& os, const triple& t) {
	os << *t.r[0] << ' ' << *t.r[1] << ' ' << *t.r[2] << L'.';
	return os;
}

typedef vector<triple*> triples;

struct rule {
	triple* head;
	triples body;
	rule(triple* h, const triples& b = triples()) : head(h), body(b) {}
};
vector<rule> rules;

wostream& operator<<(wostream& os, const rule& r) {
	os << L'{';
	for (const triple* t : r.body) os << *t << ' ';
	os << L"} => { "; 
	r.head ? os << *r.head : os << L"";
       	os << " }.";
	return os;
}

void print() { for (auto r : rules) wcout << r << endl; } 
res* readany();

res* readlist() {
	din.get(), din.skip();
	vector<res*> items;
	while (din.peek() != L')') items.push_back(readany()), din.skip();
	din.get(), din.skip();
	return mkres(items);
}

res* readany() {
	din.skip();
	if (din.peek() == L'(') return readlist();
	wchar_t *s = till();
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

void readrule() {
	if (din.peek() == L'#') din.getline();
	triples body;
	if (din.peek() == L'{') {
		din.get();
		while (din.peek() != L'}') body.push_back(readtriple());
		din.get(), din.skip(), din.get(),
		din.get(), din.skip(), din.get(), din.skip(); // "} => {";
		if (din.peek() == L'}') rules.push_back(rule(0, body)), din.skip();
		else {
			while (din.peek() != L'}') rules.push_back(rule(readtriple(), body));
			din.get();
		}
	} else {
		triple *t = readtriple();
		if (t) rules.push_back(rule(t));
	}
	din.skip();
	if (din.peek() == L'.') din.get();
}

void readdoc() { while (din.good()) din.skip(), readrule(), din.skip(); }

typedef map<res*, res*> subs;

const size_t max_frames = 1e+6;
struct frame {
	int h,b;
	frame* prev;
	subs trail;
} *first = new frame[max_frames], *last;

typedef map<int/*head*/, subs> conds;
// compiler's intermediate representation language. the information
// in the following variable contains everything the emitter has to know
map<int/*head*/,map<int/*body*/, conds>> intermediate;
map<int/*head*/,map<int/*body*/, std::function<bool()>>> program; // compiled functions

bool occurs_check(res *x, res *y) {
	if (!isvar(*x)) return isvar(*y) ? occurs_check(y, x) : false;
	if (x == y) return true;
	for (res **r = y->args; *r; ++r)
		if (occurs_check(x, *r))
			return true;
	return false;
}

bool prepare(res *s, res *d, subs& c) {
	dout << "preparing " << *s << " and " << *d << endl;
	if (!isvar(*s) && !isvar(*d)) {
		if (s != d) return dout << " failed." << endl, c.clear(), false;
		if (*s->val != L'.') return true;
		res **rs, **rd;
		for (rs = s->args, rd = d->args; *rs && *rd;)
			if (!*rs != !*rd || !prepare(*rs++, *rd++, c))
				return false;
		return dout << " passed." << endl, true;
	}
	return occurs_check(s, d) ? false : (dout << " passed " << endl, c[s] = d, true);
}

bool prepare(triple *s, triple *d, subs& c) {
	if (!d) return false;
	for (int n = 0; n < 3; ++n)
		if (!prepare(s->r[n], d->r[n], c))
			return false;
	return true;
}

void prepare() {
	subs c;
	for (int n = 0; n < (int)rules.size(); ++n)
		for (int k = 0; k < (int)rules[n].body.size(); ++k)
			for (int m = 0; m < (int)rules.size(); ++m)
				if (prepare(rules[n].body[k], rules[m].head, c))
					intermediate[n][k][m] = c, c.clear();
}

bool unify(res *s, res *d, subs& trail) { // in runtime
}

function<bool()> compile(conds& c) {
	return [c]() {
		for (conds::const_iterator x = c.begin(); x != c.end(); ++x) {
			for (pair<res*, res*> y : x->second)
				if (!unify(y.first, y.second, last->trail))
					continue;
			last->h = x->first, last->b = 0, last->prev = first, ++last;
		}
		return false;
	};
}

void compile() {
	prepare();
	for (int n = 0; n < (int)rules.size(); ++n)
		if (!rules[n].body.size())
			program[n][0] = [](){return true;}; // fact
		else for (int k = 0; k < (int)rules[n].body.size(); ++k)
			program[n][k] = compile(intermediate[n][k]);
}

void run() {
	last = first;
	for (int n = 0; n < (int)rules.size(); ++n)
		if (!rules[n].head) // push queries
			(++last)->h = n, last->b = 0, last->prev = 0;
	do { program[first->h][first->b](); } while (++first <= last);
}

int main() {
	readdoc(), print(), compile(), run();
}


