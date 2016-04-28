#include "n3driver.h"
#include <sstream>

wostream &dout = std::wcout;

namespace n3driver {

vector<term*> terms;
vector<rule> rules;
map<const term *, int> dict;
din_t din;

#define THROW(x, y)                                                            \
  {                                                                            \
    stringstream s;                                                            \
    s << x << ' ' << y;                                                        \
    throw runtime_error(s.str());                                              \
  }

term::term(const wchar_t *v) : type(*v), val(wcsdup(v)) {
	dout << val << endl;
}

term::term(vector<int> _args) : type(L'.'), args(new int[_args.size() + 1]) {
	int n = 0;
	for (auto x : _args) args[n++] = x;
	args[n] = 0;
}

term::~term() {
	if (args && type == L'.') delete[] args;
	if (val) free((wchar_t *)val);
}

triple::triple(int s, int p, int o) {
	r[0] = s, r[1] = p, r[2] = o;
}

triple::triple(const triple &t) : triple(t.r[0], t.r[1], t.r[2]) {}

premise::premise(const triple *t) : t(t) {}

rule::rule(const triple *h, const body_t &b) :
	head(h), body(b) { assert(h); }

rule::rule(const triple *h, const triple *b) : head(h) {
	assert(!h);
	body.push_back(premise(b));
}

rule::rule(const rule &r) : head(r.head), body(r.body) {}

int mkterm(const wstring &v) {
	static std::map<wstring, int> r;
	auto i = r.find(v);
	if (i != r.end()) return i->second;
	int id = terms.size();
	terms.push_back(new term(v.c_str()));
	dict.set(terms[id - 1], id);
	if (v[0] == L'?') id = -id;
	r.emplace(v, id);
	return id;
}

int mkterm(const vector<int> &v) {
	struct cmp {
		bool operator()(const vector<int> &x, const vector<int> &y) const {
			int s1 = x.size(), s2 = y.size();
			if (s1 != s2) return s1 < s2;
			return memcmp(&x[0], &y[0], sizeof(int) * s1) < 0;
		}
	};
	static std::map<vector<int>, int, cmp> r;
	auto i = r.find(v);
	if (i != r.end()) return i->second;
	int id = terms.size();
	terms.push_back(new term(v));
	r.emplace(v, id);
	dict.set(terms[id - 1], id);
	return -id;
}

const triple *mktriple(int s, int p, int o) {
	static map<tuple<int, int, int>, const triple *> spo;
	auto t = make_tuple(s, p, o);
	auto i = spo.find(t);
	return i ? i->second : spo.set(t, new triple(s, p, o));
}

wchar_t din_t::peek() { return f ? ch : ((f = true), (wcin >> ch), ch); }
wchar_t din_t::get() { return f ? ((f = false), ch) : ((wcin >> ch), ch); }
wchar_t din_t::get(wchar_t c) {
	wchar_t r = get();
	if (c != r) THROW(L"expected: ", c);
	return r;
}
bool din_t::good() { return wcin.good(); }
void din_t::skip() { while (good() && iswspace(peek())) get(); }
wstring din_t::getline() {
	wstring s;
	f = false, std::getline(wcin, s);
	return s;
}
din_t::din_t() { wcin >> std::noskipws; } 
wstring &din_t::trim(wstring &s) {
	static wstring::iterator i = s.begin();
	while (iswspace(*i)) s.erase(i), i = s.begin();
	size_t n = s.size();
	if (n) {
		while (iswspace(s[--n])) ;
		s = s.substr(0, ++n);
	}
	return s;
}

wstring din_t::edelims = L")}.";

wstring din_t::till() {
	skip();
	static wchar_t buf[4096];
	static size_t pos;
	pos = 0;
	while (good() && edelims.find(peek()) == wstring::npos && !iswspace(peek()))
		buf[pos++] = get();
	buf[pos] = 0;
	return wcslen(buf) ? buf : 0;
}

int din_t::readlist() {
	get();
	vector<int> items;
	while (skip(), (good() && peek() != L')'))
		items.push_back(readany()), skip();
	return get(), skip(), mkterm(items);
}

int din_t::readany() {
	if (skip(), !good()) return 0;
	if (peek() == L'(') return readlist();
	wstring s = till();
	if (s == L"fin" && peek() == L'.') return get(), done = true, 0;
	return s.size() ? mkterm(s) : 0;
}

const triple *din_t::readtriple() {
	int s, p, o;
	if (skip(), !(s = readany()) || !(p = readany()) || !(o = readany()))
		return 0;
	const triple *r = mktriple(s, p, o);
	if (skip(), peek() == L'.') get(), skip();
	return r;
}

void din_t::readdoc(bool query) { // TODO: support prefixes
	const triple *t;
	done = false;
	while (good() && !done) {
		body_t body;
		switch (skip(), peek()) {
		case L'{':
			if (query) THROW("can't handle {} in query", "");
			get();
			while (good() && peek() != L'}')
				body.push_back(premise(readtriple()));
			get(), skip(), get(L'='), get(L'>'), skip(), get(L'{'), skip();
			if (peek() == L'}')
				rules.push_back(rule(0, body)), skip();
			else
				while (good() && peek() != L'}')
					rules.push_back(rule(readtriple(), body));
			get();
			break;
		case L'#':
			getline();
			break;
		default:
			if ((t = readtriple()))
				rules.push_back(query ? rule(0, t) : rule(t));
			else if (!done)
				THROW("parse error: triple expected", "");
		}
		if (done) return;
		if (skip(), peek() == L'.') get(), skip();
	}
}

// output
wostream &operator<<(wostream &os, const premise &p) { return os << *p.t; }
wostream &operator<<(wostream &os, const rule &r) {
	os << L'{';
	for (auto t : r.body) os << t << ' ';
	return os << L"} => { ", (r.head ? os << *r.head : os << L""), os << " }.";
}
wostream &operator<<(wostream &os, const triple &t) {
	return os << *terms[abs(t.r[0])] << ' ' << *terms[abs(t.r[1])] << ' '
	       << *terms[abs(t.r[2])] << L'.';
}
wostream &operator<<(wostream &os, const term &r) {
	if (r.type != L'.') return r.val ? os << r.val : os;
	os << L'(';
	const int *a = r.args;
	while (*a) os << terms[*a++] << L' ';
	return os << L')';
}
}
