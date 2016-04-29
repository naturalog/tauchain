#include "n3driver.h"
#include <sstream>

wostream &dout = std::wcout;

#define THROW(x, y) {                                                          \
    stringstream s;                                                            \
    s << x << ' ' << y;                                                        \
    throw runtime_error(s.str()); }

wchar_t din_t::peek() { return f ? ch : ((f = true), (is >> ch), ch); }
wchar_t din_t::get() { return f ? ((f = false), ch) : ((is >> ch), ch); }
wchar_t din_t::get(wchar_t c) {
	wchar_t r = get();
	if (c != r) THROW(L"expected: ", c);
	return r;
}
bool din_t::good() { return is.good(); }
void din_t::skip() { while (good() && iswspace(peek())) get(); }
wstring din_t::getline() {
	wstring s;
	f = false, std::getline(is, s);
	return s;
}
din_t::din_t(wistream &is) : is(is) { is >> std::noskipws; } 
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

const term* din_t::readlist(n2vm& vm) {
	get();
	vector<const term*> items;
	while (skip(), (good() && peek() != L')'))
		items.push_back(readany(vm)), skip();
	return get(), skip(), vm.add_term(term(items));
}

const term* din_t::readany(n2vm& vm) {
	if (skip(), !good()) return 0;
	if (peek() == L'(') return readlist(vm);
	wstring s = till();
	if (s == L"fin" && peek() == L'.') return get(), done = true, (term*)0;
	return s.size() ? vm.add_term(s.c_str()) : 0;
}

const term* din_t::readtriple(n2vm& vm) {
	const term *s, *p, *o;
	if (skip(), !(s = readany(vm)) || !(p = readany(vm)) || !(o = readany(vm)))
		return 0;
	vector<const term*> v;
	v.push_back(s);
	v.push_back(p);
	v.push_back(o);
	if (skip(), peek() == L'.') get(), skip();
	return vm.add_term(term(v));
}
// TODO: support prefixes
void din_t::readdoc(bool query, n2vm& vm, vector<rule> &rules) { 
	const term *t;
	done = false;
	while (good() && !done) {
		vector<const term*> body;
		switch (skip(), peek()) {
		case L'{':
			if (query) THROW("can't handle {} in query", "");
			get();
			while (good() && peek() != L'}')
				body.push_back(readtriple(vm));
			get(), skip(), get(L'='), get(L'>'), skip(), get(L'{'), skip();
			if (peek() == L'}')
				rules.push_back(rule(0, body)), skip();
			else
				while (good() && peek() != L'}')
					rules.push_back(rule(readtriple(vm), body));
			get();
			break;
		case L'#':
		case L'@':
			getline();
			break;
		default:
			if ((t = readtriple(vm)))
				rules.push_back(query ? rule(0, t) : rule(t));
			else if (!done)
				THROW("parse error: triple expected", "");
		}
		if (done) return;
		if (skip(), peek() == L'.') get(), skip();
	}
}

wostream &operator<<(wostream &os, const term &r) {
	os << (std::wstring)r;
	return os;
}

//void din_t::print() {
//
//}
