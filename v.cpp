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
void skip() { while (iswspace(wcin.peek())) wcin.get(); }
bool isdelim(wchar_t ch, wstring& d = edelims) {
	for (auto x : d) if (ch == x) return true;
	return false;
}
wchar_t* till(wstring& d = edelims) {
	skip();
	wstringstream ws;
	wchar_t ch;
	while (!isdelim(wcin.peek())) { wcin >> ch, ws << ch; }
	wchar_t *r = wcsdup(ws.str().c_str());
	wcout << "till: " << r << endl;
	return r;
}

wostream& operator<<(wostream& os, const struct resource& r);
struct resource {
	wchar_t *value;
	union {
		resource* sub;	// substitution, in case of var. not used in compile time
		resource** args;// list items. last item must be null
	};
	resource(bool in = false) : value(in ? till() : 0), sub(0) { wcout << "new resource: " << *this << endl; }
	resource(vector<resource*> _args) : value(new wchar_t[2]), args(new resource*[_args.size() + 1]) {
	       *value= L'.', value[1] = 0;
	       int n = 0;
	       for (auto x : _args) args[n++] = x;
	       args[n] = 0;
	}
};
wostream& operator<<(wostream& os, const resource& r) {
	if (*r.value == L'.') {
		os << L'(';
		resource **a = r.args;
		while (*a)
			os << (**a) << L' ';
		os << L')'; 
	} else {
		os << r.value;
		if (*r.value == L'?' && r.sub) os << L'=' << *r.sub;
	}
	return os;
}

struct triple {
	resource *r[3]; // spo
	triple(resource *s, resource *p, resource *o) {
		r[0] = s, r[1] = p, r[2] = o; }
	triple(const triple& t) : triple(t.r[0], t.r[1], t.r[2]) {}
};
wostream& operator<<(wostream& os, const triple& t) {
	os << *t.r[0] << ' ' << *t.r[1] << ' ' << *t.r[2] << L'.';
}

struct rule {
	triple head;
	vector<triple> body;
//	rule() {}
	rule(triple h, vector<triple> b) : head(h), body(b) {}
};
vector<rule> rules;
wostream& operator<<(wostream& os, const rule& r) {
	os << L'{';
	for (const triple& t : r.body)
		os << t << ' ';
	os << L"} => { " << r.head << " }.";
}

void print() {
	for (auto r : rules) wcout << r << endl;
}

resource* readany();

resource* readlist() {
	wcin.get(), skip();
	vector<resource*> items;
	while (wcin.peek() != L')')
		items.push_back(readany()), skip();
	return new resource(items);
}

resource* readany() {
	skip();
	if (wcin.peek() == L'(') return readlist();
	return new resource(true);
}

triple readtriple() {
	skip();
	triple r(readany(), readany(), readany());
	skip();
	if (wcin.peek() == L'.') wcin.get(), skip();
	return r;
}

void readrule() {
	skip();
	vector<triple> body;
	if (wcin && wcin.peek() == L'{') {
		while (wcin.peek() != L'}')
			body.push_back(readtriple()), skip();
		wcin.get(), skip(), wcin.get(), wcin.get(), skip(), wcin.get(); // "} => {";
		while (wcin && wcin.peek() != L'}')
			rules.push_back(rule(readtriple(), body));
	} else rules.push_back(rule(readtriple(), body));
	skip();
}

void readdoc() { while (wcin.good()) skip(), readrule(), skip(); }

vector<resource> res;
typedef map<resource*, resource*> subs;


const size_t max_frames = 1e+6;
struct frame {
	int h,b;
	frame* prev;
	subs trail;
} *first, *last = new frame[max_frames];

typedef map<int/*head*/, subs> conds;
// compiler's intermediate representation language. the information
// in the following variable contains everything the emitter has to know
map<pair<int/*head*/,int/*body*/>, conds> intermediate;
map<pair<int/*head*/,int/*body*/>, std::function<bool()>> program; // compiled functions

bool occurs_check(resource *x, resource *y) {
	if (*x->value != L'?') return (*y->value != L'?') ? false : occurs_check(y, x);
	if (*y->value != L'.') return false;
	for (resource **r = y->args; *r; ++r)
		if (occurs_check(x, *r))
			return true;
	return false;
}
bool prepare(resource *s, resource *d, subs& c) {
	if (*s->value != L'?' && *d->value != L'?') {
		// FIXME
//		if (s->type != d->type && s != d) return c.clear(), false;
		if (*s->value != L'.') return true;
		resource **rs, **rd;
		for (rs = s->args, rd = d->args; *rs && *rd;)
			if (!*rs != !*rd || !prepare(*rs++, *rd++, c))
				return false;
		return true;
	}
	if (occurs_check(s, d)) return false;
	return c[s] = d, true;
}
bool prepare(triple *s, triple *d, subs& c) {
	for (int n = 0; n < 3; ++n)
		if (!prepare(s->r[n], d->r[n], c))
			return false;
	return true;
}
void prepare() {
	subs c;
	for (int n = 0; n < rules.size(); ++n)
		for (int k = 0; k < rules[n].body.size(); ++k)
			for (int m = 0; m < rules.size(); ++m)
				if (prepare(&rules[n].body[k], &rules[m].head, c))
					intermediate[make_pair(n, k)][m] = c, c.clear();
};
bool unify(resource *s, resource *d, subs& trail) { // in runtime
}
function<bool()> compile(conds& c) {
	return [c]() {
		for (conds::const_iterator x = c.begin(); x != c.end(); ++x) {
			for (pair<resource*, resource*> y : x->second)
				if (!unify(y.first, y.second, last->trail))
					continue;
			last->h = x->first, last->b = 0, last->prev = first, ++last;
		}
		return false;
	};
};
void compile() {
	prepare();
	for (int n = 0; n < rules.size(); ++n)
		if (!rules[n].body.size())
			program[make_pair(n, 0)] = [](){return true;};
		else for (int k = 0; k < rules[n].body.size(); ++k) {
			auto i = make_pair(n, k);
			program[i] = compile(intermediate[i]);
		}
}
void run(int q /* query's index in rules */) {
	first = last, first->h = q, first->b = 0, first->prev = 0;
	do {
		program[make_pair(first->h, first->b)]();
	} while (++first <= last);
}
int main() {
//	wcin << skipws;
	readdoc(), print();
	int q;// = read(), parse();
	compile(), run(q);
}


