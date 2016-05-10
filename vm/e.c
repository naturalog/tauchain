#include "e.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "list.h"

typedef struct list list;
typedef struct undo_record undo_record;
typedef struct list list;
typedef unsigned uint;
typedef uint termid;
typedef pch char*;
typedef pcch const char*;

pcch *vars, *lits;
uint *ranks, *reps, *terms; // terms: lists are always odd, rest are even
int *reps;
uint nvars, nlits, nconds, nlists;

struct undo_record {
	bool rank;
	uint x, y;
};

list *undo = list_init(undo);

void save(uint x, uint y, bool rank) {
	undo_record *u = malloc(sizeof(undo_record)),
	u->rank = rank, u->x = x, 
	u->y = y, u->n = u;
	list_add(&undo, u);
}

#define update(x, y) save(x, y, true), reps[x] = y

void revert() {
	undo_record *u = list_pop(&undo);
	if (!u) return;
	(u->rank ? ranks : reps)[u->x] = u->y;
	revert(), free(u);
}

void commit() {
	list_clear(&undo);
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
		y.isvar			&& update(ry, rx), true	;
}

