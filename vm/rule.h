#include <stdbool.h>
#include <stdlib.h>

#define new(x, y) malloc(sizeof(x) * (y))
#define _struct(x) typedef struct x x; struct x

typedef const char* pcch;
typedef map<pcch, struct term*> env;

struct term { pcch p; term **args; uint sz; };

struct rule {
	const term *s, *d;
	const rule *t, *f;
	rule(term*, term*, rule*, rule*);
	const function<void(env e)> g;
};
rule(term *s, term *d, rule *t, rule *f) : s(s), d(d) , t(t), f(f)
	, g(
	(t == f || s == d) // trivially true cases
	? [](env e) { t->g(e); }
	: islist(s) && islist(d)
	? s->sz != d->sz
	? [](env e) { f->g(e); } // two list, size mismatch
	: [this](env e) { // two lists, same size
		uint sz = s->sz;
		const term *y = &s->args[sz], *z = &s->args[sz];
		for (;--sz; t = new rule(*--y, *--z, t, f));
	}
	: [this](env e) { // two nonlists
		const term *rs = isvar(s) ? e[s->p] : s;
		const term *rd = isvar(d) ? e[d->p] : d;
		if (rs != s || rd != d) (t = new rule(rs, rd, t, f))->g(e);
		else if (isvar(s)) t->g(e);
		else if (isvar(d)) e[d->p] = s, t->g(e);
		else (s->p == d->p ? t : f)->g(e);
	}) {}

struct dfa { // init state is 0, accept states are negative
	int transition(int state, uint symbol);
};

_struct(term) {
	int p;
	term **args;
	uint sz;
};

_struct(conds) {
	term *s, *d;
	conds *next, *supp;
	uint ref;
};

_struct(match) {
	rule *r;
	conds *c, **sv, **dv;
	uint ref;
};

_struct(prem) {
	match *m;
	uint sz, ref;
};

_struct(rule) {
	bool diff;
	uint ref;
	union {
		struct { prem *p; uint sz;  };
		struct { rule *r; conds *c; };
	};
};

void mut(rule *r, match *m, rule **_rb, rule **_rh) {
	rule *rb = *_rb = new(rule, 1), *rh = *_rh = new(rule, 1);
	rb->diff = rh->diff = true,
	rb->r = r, rh->r = m->r,
	rb->c = rh->c = m->c;
}

//// vm
conds *eq_add(conds *c = 0, term *s = 0, term *d = 0);

conds *eq_add(conds *c, term *s, term *d) {
	conds *r =  new conds(1);
	if (c) ++c->ref;
	r->next = c, r->supp = 0, r->ref = 1, r->s = s, r->d = d;
	return r;
}
