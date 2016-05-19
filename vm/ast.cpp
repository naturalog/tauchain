#include "ast.h"

int ccmp::operator()(const crd& x, const crd& y) {
	auto ix = x.begin(), ex = x.end();
	auto iy = y.begin(), ey = y.end();
	while (ix != ex && iy != ey)
		if (*ix != *iy) return *ix < *iy ? 1 : -1;
		else ++ix, ++iy;
	return 0;
}

ostream& operator<<(ostream& os, const crd& c) {
	for (int i : c) os << i << ':';
	if (c.str) os << '(' << c.str << ')';
	return os << endl;
}

ostream& operator<<(ostream& os, const word& w) {
	for (auto c : w) os << c; return os << endl;
}

void term::crds(word &l, word &v, crd c, int k) const {
	if (k != -1) c.push_front(k);
	if (t == lit) { c.var = false; c.str = p; l.emplace(c); return; }
	if (t == var) { c.var = true; c.str = p; v.emplace(c); return; }
	for (uint n = 0; n < sz; ++n) args[n]->crds(l, v, c, n);
}

void rule::crds(word &ll, word &vv, int rl) {
	word l, v;
	if (head) head->crds(l, v), l = push_front(l, -1), v = push_front(v, -1);
	for (uint n = 0; n < body.size(); ++n) {
		word _l, _v;
		body[n]->t->crds(_l, _v, crd(0), n+1);
		l.insert(_l.begin(), _l.end()), v.insert(_v.begin(), _v.end());
	}
	l = push_front(l, rl), v = push_front(v, rl);
	ll.insert(l.begin(), l.end()), vv.insert(v.begin(), v.end());
}

#define res_type(x) !(x) ? lst : *(x) == '?' ? var : lit

term::term(pcch v) : p(ustr(v)), t(res_type(v)), sz(0) {
}

term::term(term** _args, uint sz)
	: p(0), args(new term*[sz + 1])
	, t(lst), sz(sz) {
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
	if (t != lst)
		return p ? os << p : os;
	os << '(';
	auto a = args;
	while (*a) **a++ >> os << ' ';
	return os << ')';
}

term* mktriple(term* s, term* p, term* o) {
	term *t[] = {s, p, o};
	return new term(t, 3);
}
