#include "n3driver.h"
#include <sstream>

wostream &dout = std::wcout;

vector<term*> terms;
vector<rule> rules;
//map<int, wstring> dict;
din_t din;

#define THROW(x, y)                                                            \
  {                                                                            \
    stringstream s;                                                            \
    s << x << ' ' << y;                                                        \
    throw runtime_error(s.str());                                              \
  }

rule mkrule(term *h, const vector<term*> &b) {
	rule r;
	r.h = h;
	if (!(r.sz = b.size())) return r;
	r.b = new term*[r.sz];
	for (unsigned n = 0; n < r.sz; ++n) r.b[n] = b[n];
	return r;
}

rule mkrule(term *h, term *t) {
	vector<term*> v;
	v.push_back(t);
	return mkrule(h, v);
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
	return pos ? buf : wstring();
}

term* din_t::readlist() {
	get();
	vector<term*> items;
	while (skip(), (good() && peek() != L')'))
		items.push_back(readany()), skip();
	return get(), skip(), new term(items);
}

term* din_t::readany() {
	if (skip(), !good()) return 0;
	if (peek() == L'(') return readlist();
	wstring s = till();
	if (s == L"fin" && peek() == L'.') return get(), done = true, (term*)0;
	return s.size() ? new term(s.c_str()) : 0;
}

term* din_t::readtriple() {
	term *s, *p, *o;
	if (skip(), !(s = readany()) || !(p = readany()) || !(o = readany()))
		return 0;
	vector<term*> v;
	v.push_back(s);
	v.push_back(p);
	v.push_back(o);
	if (skip(), peek() == L'.') get(), skip();
	return new term(v);
}

void din_t::readdoc(bool query) { // TODO: support prefixes
	term *t;
	done = false;
	while (good() && !done) {
		vector<term*> body;
		switch (skip(), peek()) {
		case L'{':
			if (query) THROW("can't handle {} in query", "");
			get();
			while (good() && peek() != L'}')
				body.push_back(readtriple());
			get(), skip(), get(L'='), get(L'>'), skip(), get(L'{'), skip();
			if (peek() == L'}')
				rules.push_back(mkrule(0, body)), skip();
			else
				while (good() && peek() != L'}')
					rules.push_back(mkrule(readtriple(), body));
			get();
			break;
		case L'#':
		case L'@':
			getline();
			break;
		default:
			if ((t = readtriple()))
				rules.push_back(query ? mkrule(0, t) : mkrule(t));
			else if (!done)
				THROW("parse error: triple expected", "");
		}
		if (done) return;
		if (skip(), peek() == L'.') get(), skip();
	}
}

wostream &operator<<(wostream &os, const rule &r) {
	os << L'{';
	for (unsigned n = 0; n < r.sz; ++n) os << *r.b[n] << ' ';
	return os << L"} => { ", (r.h ? os << *r.h : os << L""), os << " }.";
}
wostream &operator<<(wostream &os, const term &r) {
	os << (std::wstring)r;
	return os;
}
