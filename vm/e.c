#include "e.h"

var *vars;
uri *uris;
term *terms;
uint nterms, nvars, nuirs, *reps, *ranks;
undo *un = 0;

void push_change(uint x, uint y, bool rank) {
	undo *u = un;
	*(un = malloc(sizeof(undo))) = { rank, x, y, u };
}

termid update(termid x, termid y, bool rank) {
	push_change(x, y, rank);
	return (rank ? ranks : reps)[x] = y;
}

void revert(undo *u = 0) {
	if (!u) {
		if (!un) return;
		u = un;
		un = 0;
	}
	(u->rank ? ranks : reps)[u->x] = u->y;
	revert(u->n), free(u);
}

void commit(undo *u = 0) {
	if (!u) {
		if (!un) return;
		u = un;
		un = 0;
	}
	commit(u->n), free(u);
}

termid rep(termid t) {
	return reps[t] == t ? t : update(t, rep(reps[t]));
}

void inc_rank(termid x) {
	push_change(x, ranks[x]++, true);
}

bool merge(termid _x, termid _y, void *scope) {
	term x = terms[rep(x)], y = terms[rep(y)];
	if (x.leaf) {
		if (y.leaf)
			return	merge(x.r, y.r, scope)
				&& merge(x.l, y.l, scope);
		else if (!u.isvar) return false;

	}
	if (!unify(x, y, scope)) return false;
	if (ranks[x] > ranks[y]) update(y, x, false);
	else if (ranks[x] < ranks[y]) update(x, y, false);
	else update(y, x, false), inc_rank(x);
	return true;
}
