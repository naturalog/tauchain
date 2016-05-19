#include "ast.h"
#include <iostream>
int ccmp::operator()(const crd& x, const crd& y) {
	auto ix = x.c.begin(), ex = x.c.end();
	auto iy = y.c.begin(), ey = y.c.end();
//	if (x.size() != y.size()) return x.size() > y.size() ? -1 : 1;
	while (ix != ex && iy != ey)
		if (*ix != *iy) return *ix < *iy ? 1 : -1;
		else ++ix, ++iy;
	//if (ix != ex) return -1;
	//if (iy != ey) return 1;
	//if (x.str && y.str && *x.str != '?' && *y.str != '?' && x.str != y.str)
	//	return strcmp(x.str, y.str);
	//if (!x.str != !y.str) return x.str ? -1 : 1;
	return 0;
}

ostream& operator<<(ostream& os, const crd& c) {
	for (int i : c.c) os << i << ':';
	if (c.str) os << '(' << c.str << ')';
	return os << endl;
}

ostream& operator<<(ostream& os, const word& w) {
	for (auto c : w) os << c; return os << endl;
}

void term::crds(word &kb, crd c, int k) const {
	if (k != -1) c.c.push_back(k);
	if (!p) for (uint n = 0; n < sz; ++n) args[n]->crds(kb, c, n);
	else { c.str = p; kb.emplace(c);/* std::cout << p << std::endl << c;*/ }
}

word rule::crds(int rl) {
	word r;
	if (head) head->crds(r), r = push_front(r, -1);
	for (uint n = 0; n < body.size(); ++n) {
		word k;
		body[n]->t->crds(k, crd(0), n+1);
		r.insert(k.begin(), k.end());
	}
	return push_front(r, rl);
}

//#define res_type(x) !(x) ? lst : *(x) == '?' ? var : lit

term::term(pcch v) : p(ustr(v)), /*t(res_type(v)), */sz(0) {
}

term::term(term** _args, uint sz)
	: p(0), args(new term*[sz + 1])
	, /*t(lst), */sz(sz) {
	for (uint n = 0; n < sz; ++n) args[n] = _args[n];
	args[sz] = 0;
}

term::~term() {
	if (lst) delete[] args;
}

premise::premise(const term *t) : t(t) {}

rule::rule(const term *h, const body_t &b) :
	head(h), body(b) { assert(h); }

template<typename T>
inline vector<T> singleton(T x) { vector<T> r; r.push_back(x); return x; }

rule::rule(const term *h, const term *b) : head(h), body(singleton(new premise(b))) {
	assert(!h);
}

rule::rule(const rule &r) : head(r.head), body(r.body) {}

ostream &term::operator>>(ostream &os) const {
	if (p) return os << p;
	os << '(';
	auto a = args;
	while (*a) **a++ >> os << ' ';
	return os << ')';
}

term* mktriple(term* s, term* p, term* o) {
	term *t[] = {s, p, o};
	return new term(t, 3);
}
