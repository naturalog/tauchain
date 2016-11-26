#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <err.h>

typedef struct tree tree;
struct tree {
	union {
		const char* t;
		tree *rep;
	};
	tree *r, *l;
	bool isrep;
	unsigned level;
};
void printtree(tree* t);

tree* find(tree* t) {
	if (t->isrep) return t;
	if (t->rep) return find(t->rep);
	return t;
}

bool rep(tree *x, tree *y) {
	x = find(x), y = find(y);
	if (x->isrep) {
		if (y->isrep) return !strcmp(x->t, y->t);
		y->rep = x;
		return true;
	}
	if (y->isrep) {
		if (x->isrep) return !strcmp(x->t, y->t);
		x->rep = y;
		return true;
	}
	if (x->level < y->level) x->rep = y;
	else y->rep = x;
	return true;
}

// binary tree only
tree* readtree(short d) {
	char *w = malloc(4096), *r, *ww = w, c;
	unsigned short p = 0;
	tree *t = malloc(sizeof(tree));
	t->r = t->l = t->rep = 0, t->level = d, t->isrep = false;
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
			if (*w == '\\') t->l->isrep = true;
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
		while (isspace(w[--p])) w[p] = -2;
		t->t = strdup(w);
//		if (var && !strcmp(t->t, var->t)) t->rep = var;
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
	t = find(t);
	printf("%s", t->t);
	if (t->r) putchar('('), printtree(t->l), putchar(','), printtree(t->r), putchar(')');
	fflush(stdin);
}

int main() {
	tree *t = readtree(0);
	printtree(t), puts("");
//	compute(t);
	print_computed(t), puts("");
	return 0;
}
