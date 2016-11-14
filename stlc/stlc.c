#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
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
	tree *r, *l;
};
tree* readtree() {
	char c = getchar(), *w;
	int p = 0;
	tree *t = malloc(sizeof(tree));
	w = malloc(4096);
	while (isspace(c)) c = getchar();
	do {
		w[p++] = c;
		c = getchar();
	} while (c != '(' && c != ')' && !isspace(c) && c != ',');
	w[p] = 0;
	t->t = strdup(w);
	if (c == '(') t->r = readtree(), t->l = readtree();
	else t->r = t->l = 0;
	free(w);
	return t;
}
void printtree(tree* t) {
	if (!t) return;
	printf("%s", t->t);
	if (!t->r) return;
	printf("(");
	printtree(t->r), printtree(t->l);
	printf(")");
}

void main() {
	tree *t = readtree();
	printtree(t);
}
