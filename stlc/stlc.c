#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <err.h>

typedef struct term term;
typedef struct type type;
typedef struct tree tree;
typedef struct var var;
typedef struct sym sym;
typedef struct app app;
typedef struct abst abst;
typedef struct rec rec;

struct type {
	bool isfunc;
	union {
		char o;
		const type *c, *d;
	};
};
struct var {
	const char* name;
	const type* t;
};
struct sym {
	const char* name;
	const type* t;
};
struct app {
	const term *M, *N;
};
struct abst {
	const var *x;
	const term *M;
};
struct rec {
	const var *x;
	const term *M;
};
struct term {
	char t;
	union {
		var x;
		sym c;
		app a;
		abst l;
		rec y;
	};
};

term* mkvar(const char* name, const type* tt) {
	term *t = malloc(sizeof(term));
	return t->t = 'x', t->x.name = name, t->x.t = tt, t;
}
term* mksym(const char* name, const type* tt) {
	term *t = malloc(sizeof(term));
	return t->t = 'c', t->c.name = name, t->c.t = tt, t;
}
term* mkapp(const term* M, const term* N) {
	term *t = malloc(sizeof(term));
	return t->t = '@', t->a.M = M, t->a.N = N, t;
}
term* mkabst(const var* x, const term* M) {
	term *t = malloc(sizeof(term));
	return t->t = '\\', t->l.x = x, t->l.M = M, t;
}
term* mkrec(const var* x, const term* M) {
	term *t = malloc(sizeof(term));
	return t->t = 'Y', t->y.x = x, t->y.M = M, t;
}
const type* mktype(const type *c, const type *d) {
	static type* base = 0;
	if (!base) {
		base = malloc(sizeof(type));
		base->isfunc = false;
	}
	if (!d) return c ? c : base;
	type *res = malloc(sizeof(type));
	res->isfunc = true, res->c = c, res->d = d;
	return res;
}
const type* type_infer(const term* t) {
	switch (t->t) {
	case '@': return mktype(type_infer(t->a.M), type_infer(t->a.N));
	case '\\':return mktype(type_infer(t->l.M), 0);
	case 'Y':return mktype(type_infer(t->y.M), 0);
	case 'x':return t->x.t;
	case 'c':return t->c.t;
	}
}
struct tree {
	const char* t;
	tree *r, *l, *rep;
};
void printtree(tree* t);

tree* uf_find(tree *t) {
	tree *r = t->rep;
	return t->rep = r ? uf_find(r) : t;
}

void uf_setrep(tree *r, tree *t) {
	if (!t->rep) t->rep = r;
	else uf_setrep(r, t->rep);
}

void uf_union(tree *x, tree *y) {
	if (!islower(*x->t)) uf_setrep(x, y);
	else uf_setrep(y, x);
}

void equiv(tree *x, tree *y) {
	if (!x) return;
	if (!strcmp(x->t, y->t)) uf_union(x, y);
	equiv(x->r, y);
	equiv(x->l, y);
}

void compute(tree *t) {
	if (*t->t == '\\') equiv(t->r, t->l);
	else if (*t->t == '@') uf_union(t->l->l, t->r), 
	compute(t->l);
	compute(t->r);
}

tree* readtree(short d) {
	char *w = malloc(4096), *r, *ww = w, c;
	unsigned short p = 0;
	tree *t = malloc(sizeof(tree));
	t->r = t->l = 0;
	bool f = true;

	while (f)
		switch (c = getchar()) {
		case ')':
			ungetc(')', stdin);
			if (--d < 0) err(1, "unbalanced parenthesis");
			f = false; break;
		case ',':
			ungetc(',', stdin);
			f = false; break;
		case EOF: f = false; break;
		case '(':
			t->l = readtree(d + 1);
			if (getchar() != ',') err(1, "comma expected");
			t->r = readtree(d + 1);
			if (getchar() != ')') err(1, "closing parenthesis expected");
			f = false; break;
		default : if (!isspace(c)) w[p++] = c;
		}
	if (p) {
		while (isspace(*w) && p) ++w, --p;
		if (!p) return 0;
		w[p] = 0;
		while (isspace(w[--p])) w[p] = 0;
		t->t = strdup(w);
	} else {
		free(t);
		t = 0;
	}
	free(ww);
	return t;
}
void printtree(tree* t) {
	if (!t) return;
	printf("%s", t->t);
	if (t->r) putchar('('), printtree(t->l), putchar(','), printtree(t->r), putchar(')');
	fflush(stdin);
}

int main() {
	tree *t;
//	while (!feof(stdin))
		 printtree(readtree(0)), puts("");
	return 0;
}
