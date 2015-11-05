// Conchon&Filliatre persistent data structures from "A Persistent Union-Find
// Data Structure" published on "Proceeding ML '07 Proceedings of the 2007
// workshop on Workshop on ML" pp. 37-46 
// https://www.lri.fr/~filliatr/ftp/publis/puf-wml07.pdf
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "cfpds.h"

struct aux  { parr *rep; int r;		};

parr  mkarr(int *a, int s);
parr* alloc_arr(int *a, int s);
parr  mkdiff(int i, int v, parr* t);
void  reroot(parr *t);
int   get(parr *t, int i);
parr* set(parr *t, int i, int v);
aux   find_aux(parr *f, int i);

uf* create(int n) {
	uf *r = malloc(sizeof(uf));
	int *t = malloc(n * sizeof(int)), k = n;
	while (k--) t[k] = k;
	r->rep = alloc_arr(t, n);
	r->rank = alloc_arr(calloc(n, sizeof(int)), n);
	return r;
}

aux find_aux(parr *f, int i) {
	aux fr;
	int fi = get(f, i);
	if (fi == i)
		return fr.rep = f, fr.r = i, fr;
	fr = find_aux(f, fi), *f = *set(f, i, fr.r);
	return fr;
}

int find(uf* h, int x) {
	aux fcx = find_aux(h->rep, x);
	h->rep = fcx.rep;
	return fcx.r;
}

uf* unio(uf *h, int x, int y) {
	int cx = find(h, x), cy = find(h, y);
	if (cx == cy) return h;
	uf *r = malloc(sizeof(uf));
	r->rank = h->rank;
	int rx = get(h->rank, cx), ry = get(h->rank, cy);
	if (rx > ry) 
		r->rep = set(h->rep, cy, cx);
	else if (rx < ry) 
		r->rep = set(h->rep, cx, cy);
	else
		r->rank = set(h->rank, cx, rx + 1),
		r->rep = set(h->rep, cy, cx);
	return r;
}

parr mkarr(int *a, int s) {
	assert(a && s);
	parr r;
	r.isdiff = false;
	r.d.a.a = a;
	r.d.a.s = s;
	return r;
}

parr* alloc_arr(int *a, int s) {
	parr *r = malloc(sizeof(parr));
	return r->isdiff = false, r->d.a.a = a, r->d.a.s = s, r;
}

parr mkdiff(int i, int v, parr* t) {
	parr r;
	r.isdiff = true;
	r.d.d.i = i;
	r.d.d.v = v;
	r.d.d.t = t;
	return r;
}

void reroot(parr *t) {
	if (!t->isdiff) return;
	int i = t->d.d.i, v = t->d.d.v;
	parr *tt = t->d.d.t;

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

int main() {
// c&f's original ocaml unit test
	uf *t = create(10);
	assert(find(t, 0) != find(t, 1));
	t = unio(t, 0, 1);
	assert(find(t, 0) == find(t, 1));
	assert(find(t, 0) != find(t, 2));
	t = unio(t, 2, 3);
	t = unio(t, 0, 3);
	assert(find(t, 1) == find(t, 2));
	t = unio(t, 4, 4);
	assert(find(t, 4) != find(t, 3));

	return 0;
}
