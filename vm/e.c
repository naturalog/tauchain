#include "e.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "list.h"

// defined in strings.c
pcch ustr(pcch s);

typedef struct list list;
typedef struct undo_record undo_record;
typedef struct list list;
typedef unsigned uint;
typedef uint termid;
typedef pch char*;
typedef pcch const char*;

uint *ranks, *reps, *terms; // terms: lists are always odd, rest are even
int *reps;
uint nconds, nlists;

set set_create()	{ return malloc(1); }

bool set_require	(set, pcch, pcch, scope) {

}
bool set_merge		(set s, set d, set *res, scope);
uint set_compact	(set); // returns set's size

pcch init() { return ustr(""); }
pcch str_base = init();

uint term_create_lit(pcch s) { return ustr(s) - str_base; }
uint (pcch s) { return ustr(s) - str_base; }

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
#define commit() list_clear(&undo);

void revert() {
	undo_record *u = list_pop(&undo);
	if (!u) return;
	(u->rank ? ranks : reps)[u->x] = u->y;
	revert(), free(u);
}


