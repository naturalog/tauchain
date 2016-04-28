#include "n2vm.h"

bool n2vm::add_constraint(hrule r, hprem p, hrule h, hterm x, hterm y) {
	return add_constraint(kb[r][p], h, x, y);
}

bool n2vm::tick() {
	if (!curr) return false;
	resolve(curr);
	curr = curr->next;
	return true;
}

bool n2vm::add_constraint(auto &p, hrule h, hterm x, hterm y) {
	if (!isvar(x)) {
		if (isvar(y)) return add_constraint(p, h, y, x);
		if (islist(x)) {
			if (!islist(y)) return false;
			return add_lists(p, h, list(x), list(y));
		}
	}
	return add_var_constraint(p, h, x, y);
}

bool n2vm::add_var_constraint(auto &p, hrule h, hterm x, hterm y) {
	auto &s = p[h];
	auto it = s.find(x);
	if (it != s.end())
		return add_constraint(p, h, it->second, y);
	return s[x] = y, true;
}

bool n2vm::add_lists(auto &p, hrule h, hlist x, hlist y) {
	hterm a, b;
	while ((a = next(x)) && (b = next(y)))
		if (!add_constraint(p, h, a, b))
			return false;
}

hrule n2vm::mutate(hrule r, auto env) {
	auto &d = kb[kb.size()], &s = kb[r];
	auto sz = s.size();
	d.resize(sz);
	for (auto n = 0; n < sz; ++n) {
		auto &dn = d[n];
		for (const auto &m : s[n]) {
			const auto &e = m.second;
			for (auto c : e)
				if (!add_constraint(
					dn,
					m.first,
					c.first,
					c.second))
					return -1;
			for (auto c : env)
				if (!add_constraint(
					dn,
					m.first,
					c.first,
					c.second))
					return -1;
		}
	}
	return sz;
}

bool n2vm::resolve(frame *f) {
	hrule r;
	for (auto m : kb[f->r][f->p]) {
		if (-1 != (r = mutate(m.first, m.second))) continue;
		last = (last->next = new frame(r, 0, f));
	}
}
