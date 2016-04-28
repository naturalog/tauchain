#include <iostream>
#include "n2vm.h"
#include <set>
#include "n3driver.h"

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

term::term(const wchar_t* p) : isvar(*p == '?'), p(wcsdup(p)), args(0), sz(0) {
//	TRACE(dout << "new term: " << p << endl);
}

term::term(const vector<term*> &_args) 
	: isvar(false), p(0), args(new term*[_args.size() + 1])
	, sz(_args.size()) {
	int n = 0;
	for (auto x : _args) args[n++] = x;
	args[n] = 0;
}

term::~term() {
	if (p) free(p);
}

term* n2vm::add_term(const wchar_t* p, const vector<term*>& args) {
	struct term_cmp {
		int operator()(const term *_x, const term *_y) const {
			if (_x == _y) return 0;
			const term &x = *_x, &y = *_y;
			if (!x.p) return y.p ? 1 : 0;
			if (!y.p) return -1;
			if (x.p == y.p) {
				if (x.sz != y.sz) return x.sz > y.sz ? 1 : -1;
				return memcmp(x.args, y.args, sizeof(term*) * x.sz);
			}
			return wcscmp(x.p, y.p);
		}
	};
	static std::set<term*, term_cmp> terms;
	term *t = args.empty() ? new term(p) : new term(args);
	terms.emplace(t);
	return t;
}


using namespace std;

term::operator wstring() const {
	wstringstream ss;
	if (p) ss << p;
	else {
		ss << '(';
		auto a = args;
		while (a && *a) ss << (wstring)(**a++) << ' ';
		ss << ')';
	}
	return ss.str();
}

rule::operator wstring() const {
	wstringstream ss;
	ss << "{ ";
	if (h) ss << (wstring)(*h);
	ss << "} <= {";
	for (unsigned n = 0; n < sz; ++n)
		ss << (wstring)*b[n] << ' ';
	ss << "}.";
	return ss.str();
}

#define tocstr(x) ( x ? ((wstring)(*(x))).c_str() : L"(null)" )

bool n2vm::add_constraint(hrule r, hprem p, hrule h, const term *x, const term *y) {
	if (x == y) return true;
	if (!x || !y) return false;
	return add_constraint(*(*kb[r])[p], h, *x, *y);
}

bool n2vm::add_constraint(iprem &p, hrule h, const term &tx, const term &ty) {
	if (!isvar(tx)) {
		if (isvar(ty)) return add_constraint(p, h, ty, tx);
		if (islist(tx)) {
			if (!islist(ty)) return false;
			return add_lists(p, h, tx, ty);
		}
		return tx.p == ty.p;
	}
	return add_constraint(p, h, tx.p, ty);
}

bool n2vm::add_constraint(iprem &p, hrule h, const wchar_t *x, const term& y) {
	sub &s = p[h];
	sub::const_iterator it = s.find(x);
	if (it != s.end()) {
		if (!isvar(*it->second))
			return add_constraint(p, h, *it->second, y);
		const term* z = it->second;
		s[x] = &y;
		return add_constraint(p, h, *z, y);
	}
	return s[x] = &y, true;
}

bool n2vm::add_lists(iprem &p, hrule h, const term &x, const term &y) {
	auto sz = x.sz;
	if (y.sz != sz) return false;
	auto ix = x.args, iy = y.args;
	for (; *ix;  ++ix, ++iy)
		if (!add_constraint(p, h, **ix, **iy))
			return false;
	return true;
}

bool n2vm::mutate(iprem &dn, const sub &e, auto m, const sub &env) {
	for (auto c : e)
		if (!add_constraint( dn, m.first, c.first, *c.second))
			return false;
	for (auto c : env)
		if (!add_constraint( dn, m.first, c.first, *c.second))
			return false;
	return true;
}

hrule n2vm::mutate(hrule r, const sub &env) {
	irule &d = *new irule, &s = *kb[r];
	auto sz = s.size();
	d.resize(sz);
	for (unsigned n = 0; n < sz; ++n) {
		iprem &dn = *(d[n] = new iprem);
		for (const auto &m : *s[n])
			if (!mutate(dn, m.second, m, env))
				dn.erase(m.first);
		if (dn.empty()) return -1;
	}
	kb.push_back(&d);
	// TODO: remove matched constraints from varmap
	return kb.size();
}

void n2vm::getvarmap(const term& t, varmap& v) {
	if (isvar(t)) v.push_front(t.p);
	for (unsigned n = 0; n < t.sz; ++n) getvarmap(*t.args[n], v);
}

void n2vm::add_rules(rule *rs, unsigned sz) {
	orig = rs;
	origsz = sz;
	kb.resize(sz);
	for (unsigned r = 0; r < sz; ++r) {
		(kb[r] = new irule)->resize(rs[r].sz);
		if (!rs[r].h) query = r;
		for (unsigned p = 0; p < rs[r].sz; ++p) {
			(*kb[r])[p] = new iprem;
			for (unsigned h = 0; h < sz; ++h)
				add_constraint(r, p, h, rs[r].b[p], rs[h].h);
		}
		if (rs[r].h) getvarmap(*rs[r].h, vars[r]);
	}
}

bool n2vm::tick() {
	TRACE(dout<<"tick"<<endl);
	if (!last) last = curr = new frame(query, 0, 0);
	if (!curr) return false;
	hrule r;
	for (auto m : *(*kb[curr->r])[curr->p]) {
		TRACE(printkb());
		if (-1 != (r = mutate(m.first, m.second))) continue;
		last = (last->next = new frame(r, 0, curr));
	}
	curr = curr->next;
	return true;
}
	
void n2vm::printkb() {
	for (unsigned n = 0; n < kb.size(); ++n) {
		dout << "Rule " << n << ':' << (n < origsz ? (wstring)orig[n] : wstring()) << endl;
		const auto &r = *kb[n];
		for (unsigned k = 0; k < r.size(); ++k) {
			dout << "\tPrem " << k << ':' << endl;
			for (auto &m : *r[k]) {
				dout << "\t\tHead " << m.first << ':' << endl;
				for (auto &x : m.second)
					dout << "\t\t\t" << x.first << ' ' << (wstring)*x.second << endl;
			}
		}
	}
}
