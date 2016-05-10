#include <stdlib.h>

struct list_item {
	void *x;
	list_item *next;
};

struct list {
	list_item *h;
	unsigned sz;
} *lists;

typedef struct list list;
typedef struct list_item list_item;

#define list_init(l) l = malloc(sizeof(list)), l->sz = 0, l->h = 0;

#define list_item_add(l, x) \
	list_item *y##__LINE__ = malloc(sizeof(list_item)); \
	y##__LINE__->x = x, \
	y##__LINE__->next = *l, \
	*l = y##__LINE__
#define list_add(l, x) list_item_add(&l->h, x), ++l->sz;

inline void* list_pop(list *l) {
	if (!l->h) return 0;
	void *x = l->h->x;
	return l->h = l->h->next, --l->sz, free(l->h), x;
}

inline void list_clear(list *l) {
	list_item *x = l->h, *y;
	if (!x) return;
	while (x) y = x->next, free(x), x = y;
	l->sz = 0, l->h = 0;
}

inline void list_clear_free(list *l) {
	list_item *x = l->h, *y;
	if (!x) return;
	while (x) {
		if (x->x) free(x->x);
		y = x->next, free(x), x = y;
	}
	l->sz = 0, l->h = 0;
}
