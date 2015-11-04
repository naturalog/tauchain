// Conchon&Filliatre persistent data structures from "A Persistent Union-Find
// Data Structure" published on "Proceeding ML '07 Proceedings of the 2007
// workshop on Workshop on ML" pp. 37-46 
// https://www.lri.fr/~filliatr/ftp/publis/puf-wml07.pdf
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

struct arr {
	int *a, s;
};
typedef struct arr arr;
struct diff {
	int i, v;
	struct parr* _t;
};
typedef struct diff diff;
union data {
	arr a;
	diff d;
};
typedef union data data;
struct parr {
	bool isdiff;
	data d;
};
typedef struct parr parr;

parr mkarr(int *a, int s) {
	assert(a && s);
	parr r;
	r.isdiff = false;
	r.d.a.a = a;
	r.d.a.s = s;
	return r;
}
parr mkdiff(int i, int v, parr* t) {
	parr r;
	r.isdiff = true;
	r.d.d.i = i;
	r.d.d.v = v;
	r.d.d._t = t;
	return r;
}

void reroot(parr *t) {
	if (!t->isdiff) return;
	int i = t->d.d.i, v = t->d.d.v;
	parr *tt = t->d.d._t;

	reroot(tt);

	assert(!tt->isdiff);
	int vv = tt->d.a.a[i];
	tt->d.a.a[i] = v;
	*t = *tt;
	*tt = mkdiff(i, vv, t);

	assert(!t->isdiff);
	assert(tt->isdiff);
}

int get(parr *t, int i) {
	if (t->isdiff) reroot(t);
	assert(i < t->d.a.s);
	return t->d.a.a[i];
}

parr* set(parr *t, int i, int v) {
	if (t->isdiff) reroot(t);
	arr n = t->d.a;
	int old = n.a[i];
	if (old == v) return t;
	parr *res = calloc(1, sizeof(parr));
	*res = mkarr(n.a, n.s);
	n.a[i] = v;
	*t = mkdiff(i, old, res);
	return res;
}

void print(parr *t, int n) {
	int i = 0;
	for (; i < n; ++i)
		printf("%d, ", get(t, i)), fflush(stdout);
	puts("");
}

void main() {
	const int s = 10;
	parr *p = calloc(1, sizeof(parr));
	*p = mkarr(calloc(s, sizeof(int)), s);
	print(p, 10);
	p = set(p, 0, 3), print(p, 10);
	p = set(p, 1, 1), print(p, 10);
	p = set(p, 2, 7), print(p, 10);
	p = set(p, 1, 5), print(p, 10);
	p = set(p, 2, 2), print(p, 10);
	p = set(p, 3, -3), print(p, 10);
	p = set(p, 4, 9), print(p, 10);
}
