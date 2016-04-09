#include "ir.h"
#include "api.h"

using namespace ir;

kb_t *kb;

rule* rule::mutate(puf &c) {
	rule &res = *new rule;
	match *t;
	int rep;
	for (vector<match*> *p : *this) { // per body item
		auto &v = *new vector<match*>;
		for (match *mt : *p) { // per matching head
			match &m = *new match(mt->rl, mt->conds);
			for (int x : mt->vars)
				if (!m.conds->unio(x, rep = c.find(x))) {
					delete &m;
					goto stop;
				} else if (rep > 0) // rep is var
					m.vars.insert(x);
			v.push_back(&m);
		stop:	;
		}
		// body item can't ever be satisfied
		if (v.empty()) return delete &res, (rule*)0;
		res.push_back(&v);
	}
	return mutations.push_back(&res), res.prev = this, &res;
}

rule::~rule() {
	for (auto m : mutations) delete m;
	for (auto x : *this) {
		for (auto y : *x) delete y;
		delete x;
	}
}
