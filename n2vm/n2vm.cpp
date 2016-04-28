#include "n2vm.h"

bool n2vm::add_constraint(hrule r, hprem p, hrule h, term &x, term &y) {
	return add_constraint(kb[r][p], h, x, y);
}

bool n2vm::tick() {
	if (!curr) return false;
	resolve(curr);
	curr = curr->next;
	return true;
}

bool n2vm::add_constraint(auto &p, hrule h, const term &tx, const term &ty) {
	if (!isvar(tx)) {
		if (isvar(ty)) return add_constraint(p, h, ty, tx);
		if (islist(tx)) {
			if (!islist(ty)) return false;
			return add_lists(p, h, tx, ty);
		}
	}
	return add_var_constraint(p, h, tx, ty);
}

bool n2vm::add_var_constraint(auto &p, hrule h, const term &x, const term& y) {
	auto &s = p[h];
	auto it = s.find(x);
	if (it != s.end()) {
		if (!isvar(it->second))
			return add_constraint(p, h, it->second, y);
		auto z = it->second;
		s[x] = y;
		return add_constraint(p, h, z, y);
	}
	return s[x] = y, true;
}

bool n2vm::add_lists(auto &p, hrule h, const term &x, const term &y) {
	auto sz = x.args.size();
	if (y.args.size() != sz) return false;
	for (auto n = 0; n < sz; ++n)
		if (!add_constraint(p, h, *x.args[n], *y.args[n]))
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

