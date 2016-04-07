#include <stdbool.h>
#include <stdlib.h>

typedef int word;
typedef struct parrint parrint;
struct arr  { int s; word *a; };
struct diff { int i; word v; parrint* t;};

struct parrint {
	bool isdiff;
	union {
		struct arr a;
		struct diff d;
	};
};

parrint mkarr(int *a, word s) {
	assert(a && s);
	parrint r;
	r.isdiff = false; r.a.a = a; r.a.s = s;
	return r;
}

parrint* alloc_arr(int *a, word s) {
	parrint *r = malloc(sizeof(parrint));
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
	parrint *res = calloc(1, sizeof(parrint));
	*res = mkarr(n.a, n.s);
	n.a[i] = v;
	*t = mkdiff(i, old, res);
	return res;
}
