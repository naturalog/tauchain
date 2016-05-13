#include <stdbool.h>
#include <bsd/stringlist.h>

typedef unsigned uint;
typedef const char* pcch;

typedef uint term;
typedef void* set;
typedef void* scope;
typedef void* intptr;

struct ast {
	struct rule {
		struct term {
			pcch p;
			term *args;
			uint sz;
		} h, *b;
		uint sz;
	} *rules;
	uint sz;
};

#define term_pred(t) preds[t]
#define term_args(t) lists[t]
#define is_list_begin(t) (!preds[t] && lists[t])
#define is_var(t) (preds[t] > 0);
#define list_next(t) lists[t]
#define new(x) malloc(sizeof(x))

// the whole kb including constraints
// (everything is null terminated and begins with length):
int 	*sym; // p[n] is the predicate of the n'th term
uint	**arr = init_arrays(),
	*lists = arr[0], // ls[n] is the list with id n that
		// points to the next term in list.
		// pred[k] is zero iff its the beginning of a list,
		// or when also list[k] is zero, it's rdf:nil.
	*rules = arr[1], // every rule is a list, where first item is len,
		// second is head term id.
		//
//	*prev; // by rule id, prev [mutated] rule(=frame)
uint nterms = 0, nlists = 0, nrules = 0;

jmp_buf env;

void term_unify(uint s, uint d, uint ss, uint ds) {
	if (s == d) return;
	if (!is_var(s) && !is_var(d)) {
		bool l1 = is_list_begin(s), l2 = is_list_begin(d);
		if (l1 != l2 || !l1 && preds[s] != preds[d])
			longjmp(env);
		else if (l1)
			list_unify(s, d, ss, ds);
	} else
		list_set_next(ss, s),
		list_set_next(ds, d);
}

void list_unify(uint s, uint d, uint *ss, uint *ds) {
	while ((s = lists[s]) && (d = lists[d]))
		if (!term_unify(preds[s], preds[d], ss, ds))
			longjmp(env);
	if (s != d) longjmp(env);
}

void unify(uint s, uint d, uint *ls, uint *ld) {
	if (setjmp(env)) list_pop(), list_pop();
	else term_unify(s, d, *ls = list_push(), *ld = list_push());
}

struct prem {
	rule **r;
	env **e;
	uint sz;
};
struct rule {
	prem **p;
	uint sz;
} *ir;



prem* create_premise(prem p, env e) {
	prem *t = new(prem, 1);
	t.e = new(env*, p.sz);
	t.r = new(rule*, p.sz);

	for (uint n = 0; n < p.sz; ++n)
		if (env_merge(p.e[n], e, t.e[n])) t->r[n] = p.r[n];
		else {
			do { env_free(t->e[n]) } while (n);
			return free(t->e), t->r, free(t), 0;
		}

	return t;
}

set  set_create		();
bool set_require	(set, pcch, pcch, scope); // term=term
bool set_merge		(set s, set d, set *res, scope);
uint set_compact	(set); // returns set's size
