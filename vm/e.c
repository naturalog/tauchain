#include "e.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

var *vars;
uri *uris;
term *terms;
uint nterms, nvars, nuirs, *reps, *ranks;
undo *un = 0;

void push_change(uint x, uint y, bool rank) {
	undo *u = un;
	un = malloc(sizeof(undo));
	un->rank = rank, un->x = x, 
	un->y = y, un->n = u;
}

termid update(termid x, termid y) {
	push_change(x, y, true);
	return reps[x] = y;
}

void revert(undo *u) {
	if (!u) {
		if (!un) return;
		u = un, un = 0;
	}
	(u->rank ? ranks : reps)[u->x] = u->y;
	revert(u->n), free(u);
}

void commit(undo *u) {
	if (!u) {
		if (!un) return;
		u = un, un = 0;
	}
	commit(u->n), free(u);
}

termid rep(termid t) {
	return	reps[t] == t	?
		t		:
		update(t,rep(reps[t]));
}

void inc_rank(termid x) {
	push_change(x, ranks[x]++, true);
}

#define merge_cons(x, y, s) \
	merge(x.r, y.r, s) && \
	merge(x.l, y.l, s)

#define merge_vars(x, y) (\
	ranks[x] > ranks[y]	?  update(y, x)	: \
	ranks[x] < ranks[y]	?  update(x, y)	: \
	update(y, x)		,  inc_rank(x))

bool merge(termid _x, termid _y, void *scope) {
	termid rx = rep(_x), ry = rep(_y);
	if (rx == ry) return true;
	term x = terms[rx], y = terms[ry];
	assert(memcmp(&x, &y, sizeof(term)));

	return	x.isvar && y.isvar	?
		merge_vars(rx, ry), true:
		!x.leaf && !y.leaf	?
		merge_cons(x, y, scope)	:
		x.isvar			?
		update(rx, ry)		:
		!y.isvar		?
		false			:
		update(ry, rx), true	;
}

