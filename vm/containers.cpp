#include <assert.h>
#include "containers.h"

parrint mkarr(int *a, word s) {
	assert(a && s);
	parrint r;
	r.isdiff = false; r.a.a = a; r.a.s = s;
	return r;
}

parrint* alloc_arr(int *a, word s) {
	parrint *r = (parrint*)malloc(sizeof(parrint));
	r->isdiff = false; r->a.a = a; r->a.s = s;
	return r;
}

parrint mkdiff(int i, word v, parrint* t) {
	parrint r;
	r.isdiff = true; r.d.i = i;
	r.d.v = v;	 r.d.t = t;
	return r;
}

void reroot(parrint *t) {
	if (!t->isdiff) return;
	int i = t->d.i;
	word v = t->d.v;
	parrint *tt = t->d.t;
	reroot(tt);
	assert(!tt->isdiff);
	word vv = tt->a.a[i];
	tt->a.a[i] = v;
	*t = *tt;
	*tt = mkdiff(i, vv, t);
	assert(!t->isdiff);
	assert(tt->isdiff);
}

word get(parrint *t, int i) {
	if (t->isdiff) reroot(t);
	return t->a.a[i];
}

parrint* set(parrint *t, int i, int v) {
	if (t->isdiff) reroot(t);
	struct arr n = t->a;
	int old = n.a[i];
	if (old == v) return t;
	parrint *res = (parrint*)calloc(1, sizeof(parrint));
	*res = mkarr(n.a, n.s);
	n.a[i] = v;
	*t = mkdiff(i, old, res);
	return res;
}

// Conchon&Filliatre persistent data structures from "A Persistent Union-Find
// Data Structure" published on "Proceeding ML '07 Proceedings of the 2007
// workshop on Workshop on ML" pp. 37-46 
// https://www.lri.fr/~filliatr/ftp/publis/puf-wml07.pdf

struct aux { parrint *rep; int r; };

parrint  mkarr(int *a, int s);
parrint* alloc_arr(int *a, int s);
parrint  mkdiff(int i, int v, parrint* t);
void  reroot(parrint *t);
int   get(parrint *t, int i);
parrint* set(parrint *t, int i, int v);
aux   find_aux(parrint *f, int i);

uf* create(int n) {
	void *buf = malloc(sizeof(uf) + 2 * sizeof(int) * n);
	uf *r = (uf*)malloc(sizeof(uf));
	int k = n, *t = (int*)malloc(n * sizeof(int));
	while (k--) t[k] = k;
	r->rep = alloc_arr(t, n);
	r->rank = alloc_arr((int*)calloc(n, sizeof(int)), n);
	return r;
}

aux find_aux(parrint *f, int i) {
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
	uf *r = (uf*)malloc(sizeof(uf));
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

void uftest() {
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
}

uf* uf::create(int n) { return ::create(n); }
int uf::find(int x) { return ::find(this, x); }
uf* uf::unio(int x, int y) { return ::unio(this, x, y); }

//}
