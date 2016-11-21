#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <err.h>

typedef struct tree tree;
struct tree {
	const char* t;
	tree *r, *l, *rep;
};
void printtree(tree* t);

tree* uf_find(tree *t) {
	tree *r = t->rep;
	if (r) return t->rep = uf_find(r);
	return t;
}

void uf_setrep(tree *r, tree *t) {
	if (!t->rep) t->rep = r;
	else uf_setrep(r, t->rep);
}

void uf_union(tree *x, tree *y) {
	x = uf_find(x), y = uf_find(y);
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
	if (!t) return;
	if (*t->t == '\\') equiv(t->r, t->l);
	else if (*t->t == '@') uf_union(t->l->l, t->r);
	compute(t->l);
	compute(t->r);
}

tree* readtree(short d) {
	char *w = malloc(4096), *r, *ww = w, c;
	unsigned short p = 0;
	tree *t = malloc(sizeof(tree));
	t->r = t->l = t->rep = 0;
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
//	t = uf_find(t);
	printf("%s", t->t);
	if (t->r) putchar('('), printtree(t->l), putchar(','), printtree(t->r), putchar(')');
	fflush(stdin);
}
void print_computed(tree* t) {
	if (!t) return;
	switch (*t->t) {
	case '\\': print_computed(t->r); return;
	case '@': print_computed(t->l); return;
	default: ;
	}
	t = uf_find(t);
	printf("%s", t->t);
	if (t->r) putchar('('), printtree(t->l), putchar(','), printtree(t->r), putchar(')');
	fflush(stdin);
}

int main() {
	tree *t = readtree(0);
	printtree(t), puts("");
	compute(t);
	print_computed(t), puts("");
	return 0;
}
