#include "ir.h"

using namespace ir;

kb_t *kb = 0;
frame* frame::last = 0;
/*
mutation rule::mutate(unsigned b) {
	uf &c = *mutator->conds;
	rule &res = *new rule;
	match *t;
	int rep;
	premise *p = (*this)[b];
	auto &v = *new premise;
	for (match *mt : *p) { // per matching head
		match &m = *new match(mt->rl, mt->conds);
		for (int x : mt->vars) // iterate registered vars only
			// uf into every match
			if (!m.conds->unio(x, rep = c.find(x))) {
				delete &m;
				goto stop;
			} else if (rep > 0) // rep is var
				m.vars.insert(x);
		v.push_back(&m);
	stop:	;
	}
	// body item can't ever be satisfied
	if (v.empty()) return delete &res, (mutation){(rule*)0, mutator, b};
	res.push_back(&v);
	return mutations.push_back(&res), res.prev = this, (mutation){ &res, mutator, b };
}

rule::~rule() {
	for (auto m : mutations) delete m;
	for (auto x : *this) {
		for (auto y : *x) delete y;
		delete x;
	}
}*/
