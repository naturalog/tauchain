#include <iostream>
#include "n2vm.h"
#include <set>
#include "n3driver.h"

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

n2vm::n2vm(wistream& is, bool fin) : kb(*new kb_t) {
	din_t d(is);
	vector<rule> *r = new vector<rule>;
	d.readdoc(false, *this, *r);
	if (fin) d.readdoc(true, *this, *r);
//	TRACE(d.print());
	add_rules(&(*r)[0], r->size());
}

term::term(const wchar_t* p) : isvar(*p == '?'), p(wcsdup(p)), args(0), sz(0) {
//	TRACE(dout << "new term: " << p << endl);
}

term::term(const vector<const term*> &_args) 
	: isvar(false), p(0), args(vec_to_nt_arr(_args)) , sz(_args.size()) {
}

term::~term() {
	if (p && *p) { *p = 0; free(p); p = 0; }
}

rule::rule(const term *h, const vector<const term*> &b)
	: h(h), b(vec_to_nt_arr(b)), sz(b.size()) {
}

rule::rule(const term *h, const term *t) : h(h), b(vec_to_nt_arr(singleton_vector(t))), sz(1) {}
//	vector<term*> v;
//	v.push_back(t);
//	return mkrule(h, v);
//}

const term* n2vm::add_term(const term& t) {
	struct term_cmp {
		int operator()(const term* x, const term* y) const {
			return x->cmp(*y);
		}
	};
	static std::set<const term*, term_cmp> terms;
	auto it = terms.find(&t);
	term *tt;
	if (it != terms.end()) return *it;
	terms.insert(tt = new term(t));
	return tt;
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
	if (h) ss << (wstring)(*h);
	if (sz) ss << L" <= ";
	for (unsigned n = 0; n < sz; ++n) ss << (wstring)*b[n] << L' ';
	return ss << L'.', ss.str();
}

#define tocstr(x) ( x ? ((wstring)(*(x))).c_str() : L"(null)" )

bool n2vm::add_constraint(iprem &p, hrule h, const term *x, const term *y) {
	if (x == y) return true;
	if (!x || !y) return false;
	return add_constraint(p, h, *x, *y);
}

bool n2vm::add_constraint(iprem &p, hrule h, const term &x, const term &y) {
	if (!isvar(x)) {
		if (isvar(y)) return add_constraint(p, h, y, x);
		if (islist(x)) {
			if (!islist(y)) return false;
			return add_lists(p, h, x, y);
		}
		return x.p == y.p;
	}
	sub &s = p[h];
	sub::const_iterator it = s.find(&x);
	if (it != s.end()) {
		if (!isvar(*it->second))
			return add_constraint(p, h, *it->second, y);
		const term* z = it->second;
		s[&x] = &y;
		return add_constraint(p, h, *z, y);
	}
	return s[&x] = &y, true;
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
		if (!add_constraint(dn, m.first, c.first, c.second))
			return false;
	for (auto c : env)
		if (!add_constraint(dn, m.first, c.first, c.second))
			return false;
	return true;
}

hrule n2vm::mutate(hrule r, const sub &env) {
	irule &d = *new irule, &s = *kb[r];
	for (auto sn : s) {
		iprem &dn = *new iprem;
		for (const auto &m : *sn)
			if (!mutate(dn, m.second, m, env))
				dn.erase(m.first);
		if (dn.empty()) return -1;
		d.push_back(&dn);
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
	for (unsigned r = 0; r < sz; ++r) {
		irule &rl = *new irule;
		if (!rs[r].h) query = r;
		for (unsigned p = 0; p < rs[r].sz; ++p) {
			iprem &prem = *new iprem;
			for (unsigned h = 0; h < sz; ++h)
				add_constraint(prem, h, rs[r].b[p], rs[h].h);
			rl.push_back(&prem);
		}
		if (rs[r].h) getvarmap(*rs[r].h, vars[r]);
		kb.push_back(&rl);
	}
}

bool n2vm::tick() {
	TRACE(dout<<"tick"<<endl);
	if (!last) last = curr = new frame(query, kb[query]->begin());
	if (!curr) return false;
	hrule r;
	for (auto m : **curr->p) {
		TRACE(printkb());
		if (-1 != (r = mutate(m.first, m.second))) continue;
		last = (last->next = new frame(r, kb[r]->begin(), curr));
	}
	curr = curr->next;
	return true;
}
	
void n2vm::printkb() {
	for (unsigned n = 0; n < kb.size(); ++n) {
		dout << "Rule " << n << ':';
		dout << (n < origsz ? (wstring)orig[n] : wstring()) << endl;
		const auto &r = kb[n];
		int k = 0;
		for (auto prem : *r) {
			dout << "\tPrem " << k++ << ':' << endl;
			for (auto &m : *prem) {
				dout << "\t\tHead " << m.first << ':' << endl;
				for (auto &x : m.second)
					dout << "\t\t\t" << *x.first << ' ' << (wstring)*x.second << endl;
			}
		}
	}
}

// test cases
// 1. one fact, one body:
// 	1.1 one var, verify one cond
// 	1.2 one var, verify fail
//	1.3 one var twice, verify one cond
// 	1.4 one var twice, verify fail
// 	1.5 two vars, verify two conds
// 	1.6 two vars, verify two fail
// 2. two facts, two rules, one body each
// 3. two facts, two rules, two bodies each
// 4. lists

void test1() {
	std::wstringstream in;
	in << "a b c.			# rule 0" << endl;
	in << "{ ?x b c } => { a b ?x}. # rule 1" << endl;
	in << "{ ?t ?p ?t } => { }.	# rule 2" << endl;
	n2vm vm(in, false);
	vm.printkb();
}

int main() {
	test1();
	return 0;
	n2vm vm(wcin);
	int s = 0;
	while (vm.tick()) dout << "iter: " << s++ << endl;
	return 0;
}
