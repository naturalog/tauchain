//gcc prover.c -g -Wall
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

typedef unsigned int uint;
typedef char bool;
const bool true = 1;
const bool false = 0;

struct predicate 	{ int 			p;	struct predicate 	*s, *o;					};
struct subst 		{ int 			p;	struct predicate* 	pr; 	struct subst* 		next;	};
struct evidence		{ int 			p;	struct evidence_item* 	item;	struct evidence* 	next;	};
struct ruleset		{ struct rule*	 	r; 	struct ruleset*		next;					};
struct ground 		{ struct rule* 		r;	struct subst* 		s; 	struct ground* 		next;	};
struct queue		{ struct proof* 	p;	struct queue* 		next;	struct queue* 		prev;	};
struct evidence_item	{ struct predicate* 	p;	struct ground* 		g;	struct evidence_item* 	next;	};
struct predlist 	{ struct predicate* 	p; 	struct predlist* 	next; 					};
struct rule 		{ struct predicate* 	head; 	struct predlist*	body;					};
struct proof 		{ struct rule* 		rul;	struct predlist*	last;	struct proof* 		prev; struct subst* s; struct ground* g;	};

const uint K1 = 1024;
const uint max_predicates = 1024;	uint npredicates = 0; 		struct predicate* predicates;
const uint max_predlists = 1024;	uint npredlists = 0; 		struct predlist* predlists;
const uint max_rules = 1024;		uint nrules = 0; 		struct rule* rules;
const uint max_substs = 1024; 		uint nsubsts = 0; 		struct subst* substs;
const uint max_rulesets = 1024; 	uint nrulesets = 0; 		struct ruleset* rulesets;
const uint max_grounds = 1024; 		uint ngrounds = 0; 		struct ground* grounds;
const uint max_proofs = 1024; 		uint nproofs = 0; 		struct proof* proofs;
const uint max_queues = 1024; 		uint nqueues = 0; 		struct queue* queues;
const uint max_evidence_items = 1024; 	uint nevidence_items = 0; 	struct evidence_item* evidence_items;
const uint max_evidences = 1024; 	uint nevidences = 0; 		struct evidence* evidences;

void initmem();
uint size(struct predlist* p); 
void pushg(struct ground* g, struct rule* r, struct subst* s);
void pushe(struct evidence* e, int p, struct evidence_item* ei);
bool empty(struct queue* q); 
struct subst* clone(struct subst* s);
void pushq(struct queue* _q, struct proof* p);
void unshift(struct queue** _q, struct proof* p);
void push_back(struct ground** _g, struct rule* r, struct subst* s);
void setsub(struct subst** s, int p, struct predicate* pr);
struct predicate* evaluate(struct predicate* p, struct subst* s);
bool unify(struct predicate* s, struct subst* ssub, struct predicate* d, struct subst** dsub, bool f);
void prove(struct predlist* goal, struct ruleset* cases);
struct predicate* mkpred(int s, int p, int o);
bool equals(struct predicate* x, struct predicate* y);
void pushp(struct predlist** l, struct predicate* p);
void pushr(struct ruleset** _rs, struct predicate* s, struct predicate* o);
void printpred(struct predicate* p);
struct predicate* pred(int s, int p, int o);
void prints(struct subst* s);
void printl(struct predlist* l);
void printg(struct ground* g);
void printe(struct evidence* e);
void printei(struct evidence_item* ei);
void printr(struct rule* r);
void printps(struct predicate* p, struct subst* s);
void printp(struct proof* p);
void printrs(struct ruleset* rs);
void printq(struct queue* q);
struct ruleset* findruleset(struct ruleset* rs, int p);
struct ruleset* find_or_create_rs_head(struct ruleset* rs, struct predicate* p);

void initmem() {
	bzero(predicates = malloc(sizeof(struct predicate) * max_predicates), sizeof(struct predicate) * max_predicates);
	bzero(predlists = malloc(sizeof(struct predlist) * max_predlists), sizeof(struct predlist) * max_predlists);
	bzero(rules = malloc(sizeof(struct rule) * max_rules), sizeof(struct rule) * max_rules);
	bzero(substs = malloc(sizeof(struct subst) * max_substs), sizeof(struct subst) * max_substs);
	bzero(rulesets = malloc(sizeof(struct ruleset) * max_rulesets), sizeof(struct ruleset) * max_rulesets);
	bzero(grounds = malloc(sizeof(struct ground) * max_grounds), sizeof(struct ground) * max_grounds);
	bzero(proofs = malloc(sizeof(struct proof) * max_proofs), sizeof(struct proof) * max_proofs);
	bzero(queues = malloc(sizeof(struct queue) * max_queues), sizeof(struct queue) * max_queues);
	bzero(evidence_items = malloc(sizeof(struct evidence_item) * max_evidence_items), sizeof(struct evidence_item) * max_evidence_items);
	bzero(evidences = malloc(sizeof(struct evidence) * max_evidences), sizeof(struct evidence) * max_evidences);
}

uint size(struct predlist* p) { 
	if (!p) 
		return 0; 
	uint r; 
	while (p) { 
		p = p->next; 
		++r; 
	} 
	return r; 
}

struct subst* findsubst(struct subst* l, int m) { 
	while (l) 
		if (l->p == m) 
			return l; 
	return 0; 
};

struct ruleset* findruleset(struct ruleset* rs, int p) {
	for (; rs; rs = rs->next)
		if (rs->r && rs->r->head && rs->r->head->p == p)
			break;
	return rs;
}

struct ruleset* find_or_create_rs_head(struct ruleset* rs, struct predicate* p) {
	struct ruleset* prev = 0;
	while (rs) {
		if (rs->r && rs->r->head && equals(rs->r->head, p))
			return rs;
		prev = rs;
		rs = rs->next;
	}
	prev->next = &rulesets[nrulesets++];
	prev->next->r = &rules[nrules++];
	prev->next->r->head = p;
	prev->next->r->body = 0;
	return prev->next;
}

struct ground* copyg(struct ground* g) {
	if (!g)
		return 0;
	struct ground* r = &grounds[ngrounds++];
	r->r = g->r;
	r->s = g->s;
	r->next = copyg(g->next);
	return r;
}

void pushg(struct ground* g, struct rule* r, struct subst* s) {
	while (g->next) 
		g = g->next;
	g->next = &grounds[ngrounds++];
	g->next->r = r;
	g->next->s = s;
}

void pushe(struct evidence* e, int p, struct evidence_item* ei) {
	while (e->next) 
		e = e->next;
	e->next = &evidences[nevidences++];
	e->next->p = p;
	e->next->item = ei;
	e->next->next = 0;
}

bool empty(struct queue* q) { 
	return !q || (!q->p && !q->next && !q->prev); 
}

struct subst* clone(struct subst* s) {
	if (!s) return 0;
	struct subst* r = &substs[nsubsts++];
	r->p = s->p;
	r->pr = s->pr;
	r->next = clone(s->next);
	return s;
}

void pushq(struct queue* _q, struct proof* p) {
	struct queue* q = &queues[nqueues++];
	q->p = p;
	while (_q->next)
		_q = _q->next;
	_q->next = q;
	q->prev = _q;
	q->next = 0;
}

void unshift(struct queue** _q, struct proof* p) {
	struct queue* q = &queues[nqueues++];
	q->p = p;
	if (!*_q) {
		*_q = &queues[nqueues++];
		(*_q)->p = 0;
		(*_q)->next = (*_q)->prev = 0;
	}
	while ((*_q)->next)
		_q = &(*_q)->next;
	(*_q)->prev = q;
	q->prev = 0;
	q->next = *_q;
	*_q = q;
}

void push_back(struct ground** _g, struct rule* r, struct subst* s) {
	struct ground* g = &grounds[ngrounds++];
	g->next = 0;
	g->r = r;
	g->s = s;
	if (!*_g)
		*_g = g;
	else {
		while ((*_g)->next)
			_g = &(*_g)->next;
		(*_g)->next = g;
	}
}

void setsub(struct subst** s, int p, struct predicate* pr) {
	struct subst* _s = &substs[nsubsts++];
	_s->p = p;
	_s->pr = pr;
	_s->next = 0;
	if (!*s) 
		*s = _s;
	else {
		while ((*s)->next) 
			s = &(*s)->next;
		(*s)->next = _s;
	}
}

struct predicate* evaluate(struct predicate* p, struct subst* s) {
	if (p->p < 0) {
		struct subst* _s = findsubst(s, p->p);
		if (!_s)
			return 0;
		return evaluate(_s->pr, s);
	}
	if (!p->s && !p->o)
		return p;
	struct predicate *a = evaluate(p->s, s), *r = &predicates[npredicates++];
	r->p = p->p;
	if (a)
		r->s = a;
	else {
		r->s = &predicates[npredicates++];
		r->s->p = p->s->p;
		r->s->s = r->s->o = 0;
	}
	a = evaluate(p->o, s);
	if (a)
		r->o = a;
	else {
		r->o = &predicates[npredicates++];
		r->o->p = p->o->p;
		r->o->s = r->o->o = 0;
	}
	return r;
}

void printps(struct predicate* p, struct subst* s) {
	printpred(p);
	printf(" [ ");
	prints(s);
	printf(" ] ");
}

bool unify(struct predicate* s, struct subst* ssub, struct predicate* d, struct subst** dsub, bool f) {
	printf("unify called with ");
	printps(s, ssub);
	printf(" and ");
	printps(d, *dsub);
	printf("\n");

	if (s->p < 0) {
		struct predicate* sval = evaluate(s, ssub);
		if (sval)
			return unify(sval, ssub, d, dsub, f);
		return true;
	}
	if (d->p < 0) {
		struct predicate* dval = evaluate(d, *dsub);
		if (dval)
			return unify(s, ssub, dval, dsub, f);
		else {
			if (f) 
				setsub(dsub, d->p, evaluate(s, ssub));
			return true;
		}
	}
	if (!(s->p == d->p && !s->s == !d->s && !s->o == !d->o))
		return false;
	bool r = true;
	if (s->s)
		r &= unify(s->s, ssub, d->s, dsub, f);
	if (s->o)
		r &= unify(s->o, ssub, d->o, dsub, f);
	return r;

}

bool euler_path(struct proof* p) {
	struct proof* ep = p;
	while ((ep = ep->prev))
		if (ep->rul == p->rul &&
			unify(ep->rul->head, ep->s, p->rul->head, &p->s, false))
			break;
	if (ep) {
		printf("Euler path detected\n");
		return true;
	}
	return false;
}

void prove(struct predlist* goal, struct ruleset* cases) {
	struct queue *q = &queues[nqueues++];
	q->p = &proofs[nproofs++];
	q->prev = q->next = 0;
	q->p->s = 0;
	q->p->g = 0;
	struct rule* rg = &rules[nrules++];
	rg->head = 0;
	rg->body = goal;
	q->p->rul = rg;
	q->p->last = rg->body;
	struct evidence* e = &evidences[nevidences++];
	do {
		struct proof* p = q->p;
		q->prev = 0;
		q = q->next;
		printf("popped frame...\n");
		printp(p);
		if (!p->last) {
			if (!p->prev) {
				struct predlist* r;
				for (r = p->rul->body; r; r = r->next) {
					struct predicate* t = evaluate(r->p, p->s);
					struct evidence_item* ei = &evidence_items[nevidence_items++];
					ei->p = t;
					ei->g = p->g;
					pushe(e, t->p, ei);
				}
				continue;
			}
			struct ground* g = &grounds[ngrounds++];
			if (p->rul->body)
				pushg(g, p->rul, p->s);
			struct proof* r = &proofs[nproofs++];
			*r = *p;
			r->g = g;
			r->s = clone(r->s);
			unify(p->rul->head, p->s, p->last->p, &r->s, true);
			r->last = p->last->next;
			pushq(q, r);
			continue;
		}
		struct predicate* t = p->last->p;
		printf("fetched predicate: ");
		printpred(t);
		printf("\n");
		struct ruleset* rs = cases;
		printf("cases:\n");
		printrs(rs);
		while ((rs = findruleset(rs, t->p))) {
			struct subst* s = 0;
			if (unify(t, p->s, rs->r->head, &s, true)) {
				printf("unification succeeded\n");
				if (euler_path(p))
					continue;
				struct ground* g = &grounds[ngrounds++];
				struct proof* r = &proofs[nproofs++];
				r->s = s;
				r->rul = rs->r;
				r->last = p->last->next;
				r->prev = p;
				r->s = 0;
				r->g = g = copyg(p->g);
				if (!rs->r->body)
					pushg( g, rs->r, 0 );
				unshift(&q, r);
				printf("pushed frame...\n");
				printp(r);
				printf("queue:\n");
				printq(q);
			} else
				printf("unification failed\n");
			rs = rs->next;
		}
	} while (q);
	printe(e);
}

struct predicate* mkpred(int s, int p, int o) {
	struct predicate* pr = &predicates[npredicates++];
	pr->p = p;
	pr->s = &predicates[npredicates++];
	pr->s->p = s;
	pr->o->p = o;
	pr->s->o = pr->s->s = pr->o->s = pr->o->o = 0;
	return pr;
}

bool equals(struct predicate* x, struct predicate* y) {
	return (!x == !y) && x && equals(x->s, y->s) && equals(x->o, y->o);
}

void pushp(struct predlist** l, struct predicate* p) {
	if (!*l) {
		*l = &predlists[npredlists++];
		(*l)->p = 0;
		(*l)->next = 0;
	}
	while ((*l)->next && !equals(p, (*l)->p))
		l = &(*l)->next;
	if ((*l)->next)
		return;
	(*l)->next = &predlists[npredlists++];
	(*l)->next->p = p;
	(*l)->next->next = 0;
}

void pushr(struct ruleset** _rs, struct predicate* s, struct predicate* o) {
	struct ruleset* rs;
	if (!*_rs) {
		*_rs = &rulesets[nrulesets++];
		(*_rs)->r = 0;
		(*_rs)->next = 0;
	}
	rs = *_rs;
	rs = find_or_create_rs_head(rs, o);
	pushp(&rs->r->body, s);
}

void printpred(struct predicate* p) {
	if (!p)
		return;
	printpred(p->s);
	printf(" %d ", p->p);
	printpred(p->o);
}

void printp(struct proof* p) {
	if (!p)
		return;
	printf("rule: ");
	printr(p->rul);
	printf("\nremaining: ");
	printl(p->last);
	printf("\nprev: %lu\nsubst: ", (p - proofs)/sizeof(struct proof));
	prints(p->s);
	printf("\nground:");
	printg(p->g);
	printf("\n");
}

void printrs(struct ruleset* rs) {
	if (!rs)
		return;
	printr(rs->r);
	printf("\n");
	printrs(rs->next);
}

void printq(struct queue* q) {
	if (!q)
		return;
	printp(q->p);
	printq(q->next);
}

struct predicate* pred(int s, int p, int o) {
	struct predicate* ps = &predicates[npredicates++];
	struct predicate* pp = &predicates[npredicates++];
	struct predicate* po = &predicates[npredicates++];
	pp->p = p;
	pp->s = ps;
	pp->o = po;
	ps->s = ps->o = po->s = po->o = 0;
	ps->p = s;
	po->p = o;
	printpred(pp);
	printf(".\n");
	return pp;
}

void prints(struct subst* s) {
	if (!s)
		return;
	printf("%d / ", s->p);
	printpred(s->pr);
	if (s->next)
		printf(", ");
	prints(s->next);
}

void printl(struct predlist* l) {
	if (!l)
		return;
	printpred(l->p);
	printf(", ");
	printl(l->next);
}

void printr(struct rule* r) {
	if (!r)
		return;
	printl(r->body);
	printf(" => ");
	printpred(r->head);
}

void printg(struct ground* g) {
	if (!g)
		return;
	printf("ground: ");
	printr(g->r);
	printf(" ");
	prints(g->s);
	printf("\n");
	printg(g->next);
}

void printei(struct evidence_item* ei) {
	if (!ei)
		return;
	printf("evidence_item:\n");
	printpred(ei->p);
	printf("\n");
	printg(ei->g);
	printf("\n");
	printei(ei->next);
}

void printe(struct evidence* e) {
	if (!e)
		return;
	printf("%d:\n", e->p);
	printei(e->item);
	printe(e->next);
}

int main(int argc, char* argv[]) {
	initmem();
	struct predlist* goal = &predlists[npredlists++];
	struct ruleset* cases = 0;

	const int socrates = 1, a = 2, human = 3, man = 4, mortal = 5, _x = -6, _y = -7, _z = -8;
	printf("socrates a man:\t"); 	struct predicate* p1 = pred(socrates, a, man);
	printf("_x a human:\t"); 	struct predicate* p2 = pred(_x, a, human);
	printf("_x a man:\t"); 		struct predicate* p3 = pred(_x, a, man);
	printf("_y a man:\t"); 		struct predicate* p4 = pred(_y, a, human);
	printf("_y a mortal:\t"); 	struct predicate* p5 = pred(_y, a, mortal);
	printf("_z a mortal:\t"); 	struct predicate* p6 = pred(_z, a, mortal);

	pushr(&cases, 0, p1);
	pushr(&cases, 0, p2);
	pushr(&cases, 0, p3);
	pushr(&cases, 0, p4);
	pushr(&cases, 0, p5);

	pushr(&cases, p3, p2);
	pushr(&cases, p4, p5);

	goal->p = p6;
	goal->next = 0;

	prove(goal, cases);
	return 0;
}
