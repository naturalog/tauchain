#include <stdbool.h>
#include <stdlib.h>

#define new(x, y) malloc(sizeof(x) * (y))
typedef unsigned int uint;
typedef struct term term;
typedef struct rule rule;
typedef struct prem prem;
typedef struct match match;
typedef struct conds conds;

struct term {
	int p;
	term **args;
	uint sz;
};
struct conds {
	term *s, *d;
	conds *next;
};
struct match {
	rule *r;
	conds *c, **sv, **dv;
};
struct prem {
	match *m;
	uint sz;
};
struct rule {
	bool diff;
	union {
		struct { prem *p; uint sz;  };
		struct { rule *r; conds *c; };
	};
};

void mut(rule *r, match *m, rule **_rb, rule **_rh) {
	rule *rb = *_rb = new(rule, 1), *rh = *_rh = new(rule, 1);
	rb->diff = rh->diff = true,
	rb->r = r, rh->r = m->r,
	rb->c = rh->c = m->c;
}
