#include "n3driver.h"
#include <sstream>
#include <cstdio>
ostream &dout = std::cout;

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

term::term(pcch v) : p(ustr(v)), var(v && *v == '?'), lst(!v), sz(0) {
}

term::term(term** _args, uint sz)
	: p(0), args(new term*[sz + 1])
	, var(false), lst(true), sz(sz) {
	for (uint n = 0; n < sz; ++n) args[n] = _args[n];
	args[sz] = 0;
}

term::~term() {
	if (lst) delete[] args;
}

premise::premise(const term *t) : t(t) {}

rule::rule(const term *h, const body_t &b) :
	head(h), body(b) { assert(h); }

rule::rule(const term *h, const term *b) : head(h) {
	assert(!h);
	body.push_back(premise(b));
}

rule::rule(const rule &r) : head(r.head), body(r.body) {}

#define mkterm(x) new term(x)

char din_t::peek() {
	char r = f ? ch : ((f = true), ch = getchar());
	eof = r == EOF;
	return r;
}
char din_t::get() {
	char r = f ? ((f = false), ch) : /*((wcin >> ch), ch)*/(ch = getchar());
	eof = r == EOF;
	return r;
}
char din_t::get(char c) {
	char r = get();
	if (c != r) THROW("expected: ", c);
	return r;
}
bool din_t::good() { return !feof(stdin) && ch != EOF && !eof; }
void din_t::skip() { while (good() && isspace(peek())) get(); }
void din_t::getline() {
	size_t sz = 0;
	char *s = 0;
	f = false;
	::getline(&s, &sz, stdin);
	free(s);
	ch = ' ';
}
din_t::din_t() : eof(false) {/* wcin >> std::noskips;*/ } 
string &din_t::trim(string &s) {
	static string::iterator i = s.begin();
	while (isspace(*i)) s.erase(i), i = s.begin();
	size_t n = s.size();
	if (n) {
		while (isspace(s[--n])) ;
		s = s.substr(0, ++n);
	}
	return s;
}

string din_t::edelims = ")}.";

string din_t::till() {
	skip();
	static char buf[4096];
	static size_t pos;
	pos = 0;
	while (good() && edelims.find(peek()) == string::npos && !isspace(peek()) && pos < 4096)
		buf[pos++] = get();
	if (pos == 4096) perror("Max buffer limit reached (din_t::till())");
	buf[pos] = 0;
	string s;
	if (buf && strlen(buf)) s = buf;
	return s;
}

term* din_t::readlist() {
	get();
	vector<term*> items;
	while (skip(), (good() && peek() != ')'))
		items.push_back(readany()), skip();
	return get(), skip(), mkterm(items);
}

term* din_t::readany() {
	if (skip(), !good()) return 0;
	if (peek() == '(') return readlist();
	string s = till();
	if (s == "fin" && peek() == '.') return get(), done = true, (term*)0;
	return s.size() ? mkterm(s.c_str()) : 0;
}

const term *din_t::readterm() {
	term *s, *p, *o;
	if (skip(), !(s = readany()) || !(p = readany()) || !(o = readany()))
		return 0;
	std::cout<<*s<<*p<<*o<<std::endl;
	const term *r = mktriple(s, p, o);
	if (skip(), peek() == '.') get(), skip();
	std::cout << *r << endl;
	return r;
}

void din_t::readdoc(bool query) { // TODO: support prefixes
	const term *t;
	done = false;
	while (good() && !done) {
		body_t body;
		switch (skip(), peek()) {
		case '{':
			if (query) THROW("can't handle {} in query", "");
			get();
			while (good() && peek() != '}')
				body.push_back(premise(readterm()));
			get(), skip(), get('='), get('>'), skip(), get('{'), skip();
			if (peek() == '}')
				rules.push_back(rule(0, body)), skip();
			else
				while (good() && peek() != '}')
					rules.push_back(rule(readterm(), body));
			get();
			break;
		case '#':
			getline();
			break;
		default:
			if ((t = readterm()))
				rules.push_back(query ? rule(0, t) : rule(t));
			else if (!done)
				THROW("parse error: term expected", "");
		}
		if (done) return;
		if (skip(), peek() == '.') get(), skip();
	}
}

// output
ostream &operator<<(ostream &os, const premise &p) { return os << *p.t; }
ostream &operator<<(ostream &os, const rule &r) {
	os << '{';
	for (auto t : r.body) os << t << ' ';
	return os << "} => { ", (r.head ? os << *r.head : os << ""), os << " }.";
}
//ostream &operator<<(ostream &os, const term &t) {
//	return os << *terms[abs(t.r[0])] << ' ' << *terms[abs(t.r[1])] << ' '
//	       << *terms[abs(t.r[2])] << '.';
//}
ostream &operator<<(ostream &os, const term &r) {
	if (!r.lst)
		return r.p ? os << r.p : os;
	os << '(';
	auto a = r.args;
	while (*a) os << **a++ << ' ';
	return os << ')';
}

term* mktriple(term* s, term* p, term* o) {
	term *t[] = {s, p, o};
	return new term(t, 3);
}
}
