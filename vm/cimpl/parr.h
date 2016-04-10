#include <stdbool.h>

#define PARR_DECL(alpha, name) \
	typedef struct name name; \
	struct arr##alpha  { int s; alpha *a; }; \
	struct diff##alpha { int i; alpha v; name* t;}; \
	struct name { \
		bool isdiff; \
		union { \
			struct arr##alpha a; \
			struct diff##alpha d; \
		}; \
	}

#define PARR_IMPL(alpha, name) \
	name mkarr(int *a, alpha s) { \
		assert(a && s); \
		name r; \
		r.isdiff = false; r.a.a = a; r.a.s = s; \
		return r; \
	} \
	name* alloc_arr(int *a, alpha s) { \
		name *r = malloc(sizeof(name)); \
		r->isdiff = false; r->a.a = a; r->a.s = s; \
		return r; \
	} \
	name mkdiff(int i, alpha v, parr* t) { \
		name r; \
		r.isdiff = true; r.d.i = i; \
		r.d.v = v;	 r.d.t = t; \
		return r; \
	} \
	void reroot(name *t) { \
		if (!t->isdiff) return; \
		int i = t->d.i; \
		alpha v = t->d.v; \
		name *tt = t->d.t; \
		reroot(tt); \
		assert(!tt->isdiff); \
		alpha vv = tt->a.a[i]; \
		tt->a.a[i] = v; \
		*t = *tt; \
		*tt = mkdiff(i, vv, t); \
		assert(!t->isdiff); \
		assert(tt->isdiff); \
	} \
	alpha get(name *t, int i) { \
		if (t->isdiff) reroot(t); \
		return t->a.a[i]; \
	} \
	name* set(name *t, int i, int v) { \
		if (t->isdiff) reroot(t); \
		struct arr##alpha n = t->a; \
		int old = n.a[i]; \
		if (old == v) return t; \
		name *res = calloc(1, sizeof(parr)); \
		*res = mkarr(n.a, n.s); \
		n.a[i] = v; \
		*t = mkdiff(i, old, res); \
		return res; \
	}
