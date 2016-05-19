#include "ast.h"
#include <sstream>
#include <cstdio>
ostream &dout = std::cout;

typedef ast::term term;
typedef ast::rule rule;

ast *st;
char ch, r;
bool f, done, eof;
size_t sz;

#define THROW(x, y) { \
    std::stringstream s;  \
    s << x << ' ' << y; \
    dout << s.str(); throw runtime_error(s.str()); }
#define mkterm(x) new term(x)

void peek() { eof = (r = f ? ch : ((f = true), ch = getchar())) == EOF; }
void get() { eof = (r = f ? ((f = false), ch) : (ch = getchar())) == EOF; }
void get(char c) { get(); if (c != r) THROW("expected: ", c); }
bool good() { return !feof(stdin) && ch != EOF && !eof; }
void skip() { while (good() && (peek(), isspace(r))) get(); }
void getline() {
	char *s = 0;
	sz = 0, f = false, ::getline(&s, &sz, stdin), free(s), ch = ' ';
}
string &trim(string &s) {
	static string::iterator i = s.begin();
	while (isspace(*i)) s.erase(i), i = s.begin();
	size_t n = s.size();
	if (n) {
		while (isspace(s[--n])) ;
		s = s.substr(0, ++n);
	}
	return s;
}

string edelims = ")}.";
term* readany();

term* mktriple(term* s, term* p, term* o) {
	term *t[] = {s, p, o};
	return new term(t, 3);
}

string till() {
	static char buf[4096];
	static size_t pos;
	skip(), pos = 0;
	while (good() && edelims.find((peek(), r)) == string::npos && (peek(), !isspace(r)) && pos < 4096)
		get(), buf[pos++] = r;
	if (pos == 4096) perror("Max buffer limit reached (till())");
	buf[pos] = 0;
	string s;
	if (buf && strlen(buf)) s = buf;
	return s;
}

term* readlist() {
	vector<term*> items;
	get();
	while (skip(), (good() && (peek(), (r != ')'))))
		items.push_back(readany()), skip();
	return get(), skip(), mkterm(items);
}

term* readany() {
	if (skip(), !good()) return 0;
	peek();
	if (r == '(') return readlist();
	string s = till();
	if (s == "fin" && (peek(), r == '.')) return get(), done = true, (term*)0;
	return s.size() ? mkterm(s.c_str()) : 0;
}

const term *readterm() {
	term *s, *p, *o;
	if (skip(), !(s = readany()) || !(p = readany()) || !(o = readany()))
		return 0;
	const term *t = mktriple(s, p, o);
	if (skip(), (peek(), r == '.')) get(), skip();
	return t;
}

void readdoc(bool query, ast *_st) { // TODO: support prefixes
	const term *t;
	f = eof = done = false;
	st = _st;
	while (good() && !done) {
		body_t body;
		switch (skip(), peek(), r) {
		case '{':
			if (query) THROW("can't handle {} in query", "");
			get();
			while (good() && (peek(), r != '}'))
				body.push_back(new rule::premise(readterm()));
			get(), skip(), get('='), get('>'), skip(), get('{'), skip();
			if ((peek(), r == '}'))
				st->rules.push_back(new rule(0, body)), skip();
			else while (good() && (peek(), r != '}'))
				st->rules.push_back(new rule(readterm(), body));
			get(); break;
		case '#': getline(); break;
		default:
			if ((t = readterm()))
				st->rules.push_back(query ? new rule(0, t) : new rule(t));
			else if (!done)
				THROW("parse error: term expected", "");
		}
		if (done) return;
		if (skip(), (peek(), r == '.')) get(), skip();
	}
}

ostream &rule::premise::operator>>(ostream &os) const { return (*t) >> os; }
ostream &rule::operator>>(ostream &os) const {
	os << '{';
	for (auto t : body) *t >> os << ' ';
	return os << "} => { ", (head ? *head >> os : os << ""), os << " }.";
}
