#include "e.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

var *vars;
uri *uris;
term *terms;
uint nterms, nvars, nuirs, *reps, *ranks;
undo *un = 0;

void save(uint x, uint y, bool rank) {
	undo *u = un;
	un = malloc(sizeof(undo)),
	un->rank = rank, un->x = x, 
	un->y = y, un->n = u;
}

#define update(x, y) save(x, y, true), reps[x] = y

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
	return	reps[t] == t ? t : update(t,rep(reps[t]));
}

bool merge(termid _x, termid _y, void *scop) {
	termid rx = rep(_x), ry = rep(_y);
	if (rx == ry) return true;
	term x = terms[rx], y = terms[ry];
	assert(memcmp(&x, &y, sizeof(term)));

	return	x.isvar && y.isvar	?  
		ranks[rx] > ranks[ry]	? update(ry, rx)	:
		ranks[rx] < ranks[ry]	? update(rx, ry)	:
		update(ry, rx)		, save(rx,ranks[rx]++,1):
		!x.leaf && !y.leaf	?
		merge(x.r, y.r, scop)	&& merge(x.l, y.l, scop):
		x.isvar			?  update(rx, ry)	:
		!y.isvar		?  false		:
		update(ry, rx), true	;
}

