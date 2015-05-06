#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>

typedef unsigned int uint;
typedef char bool;
const bool true = 1;
const bool false = 0;

struct predicate 	{ int 			p;	struct predicate 	*s, *o;					};
struct rule 		{ struct predicate* 	p; 	struct predlist*	body;					};
struct subst 		{ int 			p;	struct predicate* 	pr; 	struct subst* 		next;	};
struct evidence		{ int 			p;	struct evidence_item* 	item;	struct evidence* 	next;	};
struct ruleset		{ struct rule*	 	r; 					struct ruleset*		next;	};
struct ground 		{ struct rule* 		r;	struct subst* 		s; 	struct ground* 		next;	};
struct queue		{ struct proof* 	p;	struct queue* 		prev;	struct queue* 		next;	};
struct evidence_item	{ struct predicate* 	p;	struct ground* 		g;	struct evidence_item* 	next;	};
struct predlist 	{ struct predicate* 	p; 					struct predlist* 	next; 	};
struct dict 		{ char*  		s;	int 			n;	struct dict*		next;	};
struct proof 		{ struct rule* 		rul;	struct predlist*	last;	struct proof* 		prev; struct subst* s; struct ground* g;	};

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
const uint max_dicts = 1024; 		uint ndicts = 0; 		struct dict* dicts;

void initmem();
void pushg(struct ground* g, struct rule* r, struct subst* s); // push rule and subst pair into ground
void pushe(struct evidence* e, int p, struct evidence_item* ei); // push evidence item into evidence
struct subst* clone(struct subst* s); // deep-copy a subst
void pushq(struct queue* _q, struct proof* p); // push a proof to the back of a proofs queue
void unshift(struct queue** _q, struct proof* p); // push a proof to the front of the queue
void setsub(struct subst** s, int p, struct predicate* pr); // pushes a substitution
struct predicate* evaluate(struct predicate* p, struct subst* s); // evaluates a predicate given a subst
bool unify(struct predicate* s, struct subst* ssub, struct predicate* d, struct subst** dsub, bool f); // unification
void prove(struct predlist* goal, struct ruleset* cases); // backchaining
bool equals(struct predicate* x, struct predicate* y); // deep-compare predicates
void pushp(struct predlist** l, struct predicate* p); // push a predicate to a predlist
void pushr(struct ruleset** _rs, struct predicate* s, struct predicate* o); // push subject and object of s=>o into a ruleset
void printpred(struct predicate* p, struct dict* d); // print a predicate
struct predicate* pred(int s, int p, int o, struct dict* d); // create an s/p/o predicate
void prints(struct subst* s, struct dict* d); // print a subst
void printl(struct predlist* l, struct dict* d); // print a predlist
void printg(struct ground* g, struct dict* d); // print a ground struct
void printe(struct evidence* e, struct dict* d); // print evidence
void printei(struct evidence_item* ei, struct dict* d); // print single evidence step
void printr(struct rule* r, struct dict* d);  // print rule
void printps(struct predicate* p, struct subst* s, struct dict* d); // print a predicate with a subst
void printp(struct proof* p, struct dict* d); // print a proof element
void printrs(struct ruleset* rs, struct dict* d); // print a ruleset
void printq(struct queue* q, struct dict* d); // print a proof queue
struct ruleset* findruleset(struct ruleset* rs, int p); // find in a ruleset according to predicate
struct ruleset* find_or_create_rs_p(struct ruleset* rs, struct predicate* p); // finds a predicate in a ruleset or creates a new rule with that predicate if not exists
int pushw(struct dict** _d, const char* s); // push a string into a dictionary and get its int id
const char* dgetw(struct dict* d, int n); // decrypt string from dictionary given id
int readp(struct dict* d, struct ruleset** _r); // read a predicate from user and stores it in a ruleset
void readr(struct ruleset** _r); // read a rule from the user and stores it in a ruleset
void menu(struct dict* d);
void trim(char *s);

const char main_menu[] = 
"Tau deductive reasoner, part of Tau-Chain\n\
Based on Euler (EYE) reasoner\n\
http://idni.org\n\
=========================================\n\n\
Syntax: <command> args>\n\
Type '?' for help.\n";
const char help[] =
"Syntax: <command> args>\n\
Type '?' for help.\n\
Commands:\n\n\
---------\n\n\
Printing commands:\n\
Syntax: <command> [id] where kb selects kb to list the desired values, and id of the desired value.\n\
If id is not specified, all values from the specified kind will belisted.\n\
Exceptions to this syntax is pa that prints everything in memory.\n\n\
pp [id]\tprints predicates.\n\
pr [id]\tprints rules.\n\
ps [id]\tprints substitutions.\n\
pg [id]\tprints grounds.\n\
pq [id]\tprints queues.\n\
ppr [id]\tprints proofs.\n\
pd [id]\tprints dicts.\n\
pl [id]\tprints predicate lists.\n\
pll [id]\tprints rule lists.\n\
pe [id]\tprints evidence.\n\
pa \tprint everything.\n\n\
\nInsertion commands:\n\n\
All commands that end with 's' will cause repeated insertion until a line equals the word 'done' is inserted.\n\
ip[s] [kb]\tinsert predicate[s].\n\
ir[s] [kb]\tinsert rule[s].\n\
\nProof commands:\n\n\
cq\tcreate a proof queue and print its id.\n\
nq\tprocess the next frame in the queue.\n";

const char prompt[] = "tau >> ";

#define syn_err { printf("Syntax error."); continue; }

bool read_num(const char* line, int* x) {
	*x = 0;
	while (isdigit(*line)) {
		*x *= 10;
		*x += *line++ - '0';
	}
	return !*line;	
}

bool read_two_nums(const char* line, int* x, int* y) {
	*x = *y = 0;
	while (isdigit(*line)) {
		*x *= 10;
		*x += *line++ - '0';
	}
	if (!isspace(*line))
		return false;
	while (isdigit(*line)) {
		*y *= 10;
		*y += *line++ - '0';
	}
	return !*line;	
}

void printcmd(struct dict* d, const char* line) {
	int id;
	uint n = 0;
	do 
	if (read_num(line, &id)) {
		switch (*line) {
		case 'p':
			if (isspace(line + 1)) {
				printpred(&predicates[id], d);
				puts("\n");
			} else {
				if (*line == 'r') {
					printp(&proofs[id], d);
					puts("\n");
				}
				else
					syn_err;
			}
			break;
		case 'r':
			printr(&rules[id], d);
			puts("\n");
			break;
		case 's':
			prints(&substs[id], d);
			puts("\n");
			break;
		case 'g':
			printg(&grounds[id], d);
			puts("\n");
			break;
		case 'q':
			printq(&queues[id], d);
			puts("\n");
			break;
		case 'e':
			printe(&evidences[id], d);
			puts("\n");
			break;
		case 'l':
			if (isspace(line + 1)) {
				printl(&predlists[id], d);
				puts("\n");
			} else {
				if (*line == 'l') {
					printrs(&rulesets[id], d);
					puts("\n");
				}
				else
					syn_err;
			}
			break;
		}
	}
	else {
		switch (*line) {
		case 'p':
			if (isspace(line + 1))
				for (; n < npredicates; ++n) {
					printf("%d\t", n);
					printpred(&predicates[n], d);
					puts("\n");
				}
			else {
				if (*line == 'r')
					for (; n < nproofs; ++n) {
						printf("%d\t", n);
						printp(&proofs[n], d);
						puts("\n");
					}
				else
					syn_err;
			}
			break;
		case 'r':
			for (; n < nrules; ++n) {
				printf("%d\t", n);
				printr(&rules[n], d);
				puts("\n");
			}
			break;
		case 's':
			for (; n < nsubsts; ++n) {
				printf("%d\t", n);
				prints(&substs[n], d);
				puts("\n");
			}
			break;
		case 'g':
			for (; n < ngrounds; ++n) {
				printf("%d\t", n);
				printg(&grounds[n], d);
				puts("\n");
			}
		case 'q':
			for (; n < nqueues; ++n) {
				printf("%d\t", n);
				printq(&queues[n], d);
				puts("\n");
			}
			break;
		case 'e':
			for (; n < nevidences; ++n) {
				printf("%d\t", n);
				printe(&evidences[n], d);
				puts("\n");
			}
			break;
		case 'l':
			if (isspace(line + 1))
				for (; n < npredlists; ++n) {
					printf("%d\t", n);
					printl(&predlists[n], d);
					puts("\n");
				}
			else {
				if (*line == 'l')
					for (; n < nrulesets; ++n) {
						printf("%d\t", n);
						printrs(&rulesets[n], d);
						puts("\n");
					}
				else
					syn_err;
			}
			break;
		}
	}
	while(0);
}

void menu(struct dict* d) {
	char* line;
	puts(main_menu);
	rl_bind_key('\t', rl_complete);
	while ((line = readline(prompt))) {
		add_history(line);
		trim(line);
		if (!strcmp(line, "?")) {
			printf(help);
			continue;
		}
		if (!line[0] || !line[1]) 
			syn_err;
		if (line[0] == 'p') {
			++line;
			if (isspace(++line))
				syn_err;
			continue;
		}

		free(line);
	}


	struct ruleset *kb = 0, *query = 0;
	struct ruleset** curr = &kb;
	char ch;
	while (1) {
	bool validinput = false;
	while (!validinput) {
			if (curr == &query)
				printf("Press p for entering a new predicate, r for entering a new rule, or q to begin reasoning when done.");
			else
				printf("Press p for entering a new predicate, r for entering a new rule, or q to input a query when ready.");
				fflush(stdout);
			ch = getchar();
			switch (ch) {
			case 'p': 
				validinput = true;
				printf("\n");
				readp(d, curr);
				break;
			case 'r': 
				validinput = true;
				printf("\n");
				readr(curr); 
				break;
			case 'q': 
				validinput = true;
				printf("\n");
				if (curr != &query)
					curr = &query;
				else
					prove(query, kb);
				break;
			default:
				break;
			}
		}
	}
}


#define str_input_len 255

struct dict* gdict;

void initmem() {
	memset(predicates 	= malloc(sizeof(struct predicate) * max_predicates), 		0, sizeof(struct predicate) * max_predicates);
	memset(predlists 	= malloc(sizeof(struct predlist) * max_predlists), 		0, sizeof(struct predlist) * max_predlists);
	memset(rules 		= malloc(sizeof(struct rule) * max_rules), 			0, sizeof(struct rule) * max_rules);
	memset(substs 		= malloc(sizeof(struct subst) * max_substs), 			0, sizeof(struct subst) * max_substs);
	memset(rulesets 	= malloc(sizeof(struct ruleset) * max_rulesets), 		0, sizeof(struct ruleset) * max_rulesets);
	memset(grounds 		= malloc(sizeof(struct ground) * max_grounds), 			0, sizeof(struct ground) * max_grounds);
	memset(proofs 		= malloc(sizeof(struct proof) * max_proofs), 			0, sizeof(struct proof) * max_proofs);
	memset(dicts 		= malloc(sizeof(struct dict) * max_dicts), 			0, sizeof(struct dict) * max_dicts);
	memset(queues 		= malloc(sizeof(struct queue) * max_queues), 			0, sizeof(struct queue) * max_queues);
	memset(evidence_items 	= malloc(sizeof(struct evidence_item) * max_evidence_items), 	0, sizeof(struct evidence_item) * max_evidence_items);
	memset(evidences 	= malloc(sizeof(struct evidence) * max_evidences), 		0, sizeof(struct evidence) * max_evidences);
}

void trim(char *s) {
	char *end;
	uint len = strlen(s), n = 0;
	while (isspace ( *s ) ) 
		memcpy(s, s + 1, len--);
	while (isspace ( s[len - 1] ) )
		--len;
	s[len] = 0;
}


struct subst* findsubst(struct subst* l, int m) { 
	while (l) 
		if (l->p == m) 
			return l; 
	return 0; 
}

struct ruleset* findruleset(struct ruleset* rs, int p) {
	for (; rs; rs = rs->next)
		if (rs->r && rs->r->p && rs->r->p->p == p)
			break;
	return rs;
}

struct ruleset* find_or_create_rs_p(struct ruleset* rs, struct predicate* p) {
	struct ruleset* prev = 0;
	while (rs) {
		if (rs->r && rs->r->p && equals(rs->r->p, p))
			return rs;
		prev = rs;
		rs = rs->next;
	}
	prev->next = &rulesets[nrulesets++];
	prev->next->r = &rules[nrules++];
	prev->next->r->p = p;
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

struct subst* clone(struct subst* s) {
	if (!s) return 0;
	struct subst* r = &substs[nsubsts++];
	r->p = s->p;
	r->pr = s->pr;
	r->next = clone(s->next);
	return s;
}

void pushq(struct queue* _q, struct proof* p) {
	if (!p)
		printf("Error: pushq called with null proof\n");
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
	if (!p)
		printf("Error: unshift called with null proof\n");
	q->p = p;
	if (!*_q) {
		*_q = q;
		q->next = q->prev = 0;
		return;
	}
	while ((*_q)->next)
		_q = &(*_q)->next;
	(*_q)->prev = q;
	q->prev = 0;
	q->next = *_q;
	*_q = q;
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

void printps(struct predicate* p, struct subst* s, struct dict* d) {
	printpred(p, d);
	printf(" [ ");
	prints(s, d);
	printf(" ] ");
}

bool unify(struct predicate* s, struct subst* ssub, struct predicate* d, struct subst** dsub, bool f) {
	printf("unify called with ");
	printps(s, ssub, gdict);
	printf(" and ");
	printps(d, *dsub, gdict);
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
			unify(ep->rul->p, ep->s, p->rul->p, &p->s, false))
			break;
	if (ep) {
		printf("Euler path detected\n");
		return true;
	}
	return false;
}

void prove(struct predlist* goal, struct ruleset* cases) {
	struct queue *qu = &queues[nqueues++];
	struct rule* rg = &rules[nrules++];
	rg->p = 0;
	rg->body = goal;
	qu->p = &proofs[nproofs++];
	qu->prev = qu->next = 0;
	qu->p->s = 0;
	qu->p->g = 0;
	qu->p->rul = rg;
	qu->p->last = rg->body;
	struct evidence* e = &evidences[nevidences++];
	do {
		struct proof* p = qu->p;
		qu->prev = 0;
		qu = qu->next;
		printf("popped frame...\n");
		printp(p, gdict);
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
			*r = *p->prev;
			r->g = g;
			r->s = clone(p->prev->s);
			unify(p->rul->p, p->s, r->last->p, &r->s, true);
			r->last = r->last->next;
			pushq(qu, r);
			continue;
		}
		struct predicate* t = p->last->p;
		printf("fetched predicate: ");
		printpred(t, gdict);
		printf("\n");
		struct ruleset* rs = cases;
		printf("cases:\n");
		printrs(rs, gdict);
		while ((rs = findruleset(rs, t->p))) {
			struct subst* s = 0;
			if (unify(t, p->s, rs->r->p, &s, true)) {
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
				unshift(&qu, r);
				printf("pushed frame...\n");
				printp(r, gdict);
				printf("queue:\n");
				printq(qu, gdict);
			} else
				printf("unification failed\n");
			rs = rs->next;
		}
	} while (qu);
	printe(e, gdict);
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
	rs = find_or_create_rs_p(rs, o);
	pushp(&rs->r->body, s);
}

void printpred(struct predicate* p, struct dict* d) {
	const char* s;
	if (!p)
		return;
	printpred(p->s, d);
	if (!d)
		printf(" %d ", p->p);
	else {
		s = dgetw(d, p->p);
		if (s)
			puts(s);
	}
	printpred(p->o, d);
}

void printp(struct proof* p, struct dict* d) {
	if (!p)
		return;
	printf("rule: ");
	printr(p->rul, d);
	printf("\nremaining: ");
	printl(p->last, d);
	printf("\nprev: %lu\nsubst: ", (p - proofs)/sizeof(struct proof));
	prints(p->s, d);
	printf("\nground:");
	printg(p->g, d);
	printf("\n");
}

void printrs(struct ruleset* rs, struct dict* d) {
	if (!rs)
		return;
	printr(rs->r, d);
	printf("\n");
	printrs(rs->next, d);
}

void printq(struct queue* q, struct dict* d) {
	if (!q)
		return;
	printp(q->p, d);
	printq(q->next, d);
}

struct predicate* pred(int s, int p, int o, struct dict* d) {
	struct predicate* ps = &predicates[npredicates++];
	struct predicate* pp = &predicates[npredicates++];
	struct predicate* po = &predicates[npredicates++];
	pp->p = p;
	pp->s = ps;
	pp->o = po;
	ps->s = ps->o = po->s = po->o = 0;
	ps->p = s;
	po->p = o;
	printpred(pp, d);
	printf(".\n");
	return pp;
}

void prints(struct subst* s, struct dict* d) {
	if (!s)
		return;
	printf("%d / ", s->p);
	printpred(s->pr, d);
	if (s->next)
		printf(", ");
	prints(s->next, d);
}

void printl(struct predlist* l, struct dict* d) {
	if (!l)
		return;
	printpred(l->p, d);
	printf(", ");
	printl(l->next, d);
}

void printr(struct rule* r, struct dict* d) {
	if (!r)
		return;
	printl(r->body, d);
	printf(" => ");
	printpred(r->p, d);
}

void printg(struct ground* g, struct dict* d) {
	if (!g)
		return;
	printf("ground: ");
	printr(g->r, d);
	printf(" ");
	prints(g->s, d);
	printf("\n");
	printg(g->next, d);
}

void printei(struct evidence_item* ei, struct dict* d) {
	if (!ei)
		return;
	printf("evidence_item:\n");
	printpred(ei->p, d);
	printf("\n");
	printg(ei->g, d);
	printf("\n");
	printei(ei->next, d);
}

void printe(struct evidence* e, struct dict* d) {
	if (!e)
		return;
	printf("%d:\n", e->p);
	printei(e->item, d);
	printe(e->next, d);
}

int pushw(struct dict** _d, const char* s) {
	uint c = 1;
	if (!*_d) {
		*_d = &dicts[ndicts++];
		(*_d)->next = 0;
		(*_d)->s = malloc(strlen(s) + 1);
		strcpy((*_d)->s, s);
		return (*_d)->n = c;
	}
	struct dict* d = *_d;
	while (d->next) {
		++c;
		d = d->next;
	}
	d->next = &dicts[ndicts++];
	d->s = malloc(strlen(s) + 1);
	strcpy(d->s, s);
	return d->n = c;
}

const char* dgetw(struct dict* d, int n) {
	if (!d)
		return 0;
	if (d->n == n)
		return d->s;
	return dgetw(d->next, n);
}

int _readp(struct dict* d, struct ruleset** _r, char* s, char* p, char* o, int* ns, int *np, int *no) {
	char ch;
	uint pos = 0;
	while ((ch = getchar()) != ' ') {
		if (ch == '\n' && !pos)
			continue;
		else if (pos)
			return -1;
		s[pos++] = ch;
	}
	s[pos] = 0;
	pos = 0;
	while ((ch = getchar()) != ' ') {
		if (ch == '\n')
			return -1;
		p[pos++] = ch;
	}
	p[pos] = 0;
	pos = 0;
	while (((ch = getchar()) != ' ') && (ch != '\n'))
		o[pos++] = ch;
	o[pos] = 0;
	*ns = pushw(&d, s);
	*np = pushw(&d, p);
	*no = pushw(&d, o);

	if (!*_r) {
		*_r = &rulesets[nrulesets++];
		(*_r)->next = 0;
	}

	struct ruleset* r = *_r;
	while (r->next)
		r = r->next;
	r->next = &rulesets[nrulesets++];
	r->r = &rules[nrules++];
	r->r->body = 0;
	r->r->p = pred(*ns, *np, *no, d);

	return (r->r->p - predicates)/sizeof(struct predicate);
}

int readp(struct dict* d, struct ruleset** _r) {
	char s[str_input_len], p[str_input_len], o[str_input_len];
	int ns, np, no;
	printf("Enter a predicate as 's p o':");
	int pid = _readp(d, _r, s, p, o, &ns, &np, &no);
	if (pid != -1)
		printf("predicate read: %s %s %s ( %d %d %d ) Predicate id: %d\n ", s, p, o, ns, np, no, pid);
	else
		printf("Cancelled.\n");
	return pid;
}

void readr(struct ruleset** r) {
	char s[16], o[16], ch;
	uint pos = 0;
	int ns, no;
	printf("Enter two numbers separated by spaces ('s o') such that predicate s implies predicate o: ");
	while ((ch = getchar()) != ' ')
		s[pos++] = ch;
	pos = 0;
	while ((ch = getchar()) != '\n')
		o[pos++] = ch;
	ns = atoi(s);
	no = atoi(o);
	pushr(r, &predicates[ns], &predicates[no]);
}

int main(int argc, char* argv[]) {
	initmem();
	gdict = &dicts[ndicts++];
	struct predlist* goal = &predlists[npredlists++];
	struct ruleset* cases = 0;

	if (argc == 1) {
		menu(gdict);
		return 0;
	}

	const int socrates = 1, a = 2, human = 3, man = 4, mortal = 5, _x = -6, _y = -7, _z = -8;
	printf("socrates a man:\t"); 	struct predicate* p1 = pred(socrates, a, man, gdict);
	printf("_x a human:\t"); 	struct predicate* p2 = pred(_x, a, human, gdict);
	printf("_x a man:\t"); 		struct predicate* p3 = pred(_x, a, man, gdict);
	printf("_y a man:\t"); 		struct predicate* p4 = pred(_y, a, human, gdict);
	printf("_y a mortal:\t"); 	struct predicate* p5 = pred(_y, a, mortal, gdict);
	printf("_z a mortal:\t"); 	struct predicate* p6 = pred(_z, a, mortal, gdict);

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
