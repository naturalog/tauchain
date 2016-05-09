#include "strings.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <bsd/sys/tree.h>

typedef struct data data;
struct data {
	RB_ENTRY(data) n;
	pcch s;
};

int cmp(const data *x, const data *y) { return strcmp(x->s, y->s); }

RB_HEAD(tree, data) head = RB_INITIALIZER(&head);
RB_PROTOTYPE(tree, data, n, cmp);
RB_GENERATE(tree, data, n, cmp);

pcch ustr(pcch s) {
	data *d = malloc(sizeof(data));
	d->s = s;
	RB_INSERT(tree, &head, d);
}

int main() {
	ustr("hello");
	ustr("world");
	ustr("world");
	data *d;
	RB_FOREACH(d, tree, &head) puts(d->s);
	return 0;
}
