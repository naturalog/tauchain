#include <iostream>
#include "n2vm.h"

bool n2vm::add_constraint(hrule r, hprem p, hrule h, const term *x, const term *y) {
	if (x == y) return true;
	if (!x || !y) return false;
	return add_constraint(kb[r][p], h, *x, *y);
}

bool n2vm::add_constraint(auto &p, hrule h, const term &tx, const term &ty) {
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

bool n2vm::add_constraint(auto &p, hrule h, int x, const term& y) {
	auto &s = p[h];
	auto it = s.find(x);
	if (it != s.end()) {
		if (!isvar(it->second))
			return add_constraint(p, h, *it->second, y);
		auto z = it->second;
		s[x] = &y;
		return add_constraint(p, h, *z, y);
	}
	return s[x] = &y, true;
}

bool n2vm::add_lists(auto &p, hrule h, const term &x, const term &y) {
	auto sz = x.args.size();
	if (y.args.size() != sz) return false;
	auto ix = x.args.begin(), ex = x.args.end(), iy = y.args.begin();
	for (; ix != ex;  ++ix, ++iy)
		if (!add_constraint(p, h, **ix, **iy))
			return false;
	return true;
}

hrule n2vm::mutate(hrule r, auto env) {
	auto kbs = kb.size();
	kb.resize(kbs + 1);
	auto &d = kb[kbs], &s = kb[r];
	auto sz = s.size();
	bool fail = false;
	d.resize(sz);
	for (unsigned n = 0; n < sz; ++n) {
		auto &dn = d[n];
		for (const auto &m : s[n]) {
			const auto &e = m.second;
			for (auto c : e)
				if (!add_constraint(
					dn,
					m.first,
					c.first,
					*c.second)) {
						fail = true;
						break;
					}
			if (!fail) for (auto c : env)
				if (!add_constraint(
					dn,
					m.first,
					c.first,
					*c.second)) {
						fail = true;
						break;
					}
			if (fail) {
				fail = false;
				dn.erase(m.first);
				continue;
			}
		}
	}
	kb.push_back(d);
	return kbs;
}

term* n2vm::add_term(int p, const vector<const term*>& args) {
	struct term_cmp {
		int operator()(const term *_x, const term *_y) const {
			static term_cmp tc;
			if (_x == _y) return 0;
			const term &x = *_x, &y = *_y;
			if (x.p > y.p) return 1;
			if (x.p < y.p) return -1;
			int r;
			auto ix = x.args.begin(), ex = x.args.end(), iy = y.args.begin();
			for (; ix != ex;  ++ix, ++iy)
				if ((r = tc(*ix, *iy)))
					return r;
			return 0;
		}
	};
	static set<term*, term_cmp> terms;
	term *t = new term(p, args);
	terms.emplace(t);
	return t;
}

void n2vm::add_rules(rule *rs, unsigned sz) {
	kb.resize(sz);
	for (unsigned r = 0; r < sz; ++r) {
		kb[r].resize(rs[r].sz);
		if (!rs[r].h) query = r;
		for (unsigned p = 0; p < rs[r].sz; ++p)
			for (unsigned h = 0; h < sz; ++h)
				add_constraint(r, p, h, rs[r].b[p], rs[h].h);
	}
}

bool n2vm::tick() {
	if (!last) last = curr = new frame(query, 0, 0);
	if (!curr) return false;
	hrule r;
	for (auto m : kb[curr->r][curr->p]) {
		if (-1 != (r = mutate(m.first, m.second))) continue;
		last = (last->next = new frame(r, 0, curr));
	}
	curr = curr->next;
	return true;
}

int main() {
	n2vm v;
	int r = 1;
	
	typedef const term cterm;

	cterm *s = v.add_term(r++);
	cterm *a = v.add_term(r++);
	cterm *m = v.add_term(r++);
	cterm *x = v.add_term(-r++);
	cterm *y = v.add_term(-r++);
	cterm *l = v.add_term(r++);

	cterm *sam = v.add_term(0, { s, a, m });
	cterm *xam = v.add_term(0, { x, a, m });
	cterm *yam = v.add_term(0, { y, a, m });
	cterm *xal = v.add_term(0, { x, a, l });

	rule *rs = new rule[3];
	rs[0].sz = 0;
	rs[0].h = sam;

	rs[1].sz = 1;
	rs[1].h = xal;
	rs[1].b = &xam;

	rs[2].sz = 1;
	rs[2].h = 0;
	rs[2].b = &yam;

	v.add_rules(rs, 3);

	while (v.tick()) cout << "t" << endl;

	return 0;
}
