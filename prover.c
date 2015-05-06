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

struct term 		{ int 		p;	struct term 	*s, *o;						};
struct rule 		{ struct term* 	p; 	struct termset*	body;						};
struct subst 		{ int 		p;	struct term* 	pr; 		struct subst* 		next;	};
struct evidence		{ int 		p;	struct evidence_item* 	item;	struct evidence* 	next;	};
struct ground 		{ struct rule* 	r;	struct subst* 		s; 	struct ground* 		next;	};
struct queue		{ struct proof*	p;	struct queue* 		prev;	struct queue* 		next;	};
struct evidence_item	{ struct term* 	p;	struct ground* 		g;	struct evidence_item* 	next;	};
struct termset	 	{ struct term*	p; 					struct termset* 	next; 	};
struct ruleset		{ struct rule* 	r; 					struct ruleset*		next;	};
struct dict 		{ char*  	s;	int 			n;	struct dict*		next;	};
struct session		{ struct queue*	q;	struct ruleset *	rkb;	struct termset *kb, *goal;	struct dict*	d;	};
struct proof 		{ struct rule* 	rul;	struct termset*		last;	struct proof* 		prev; 	struct subst* 	s; 	struct ground* g;	};

const uint max_terms = 1024;		uint nterms = 0; 		struct term* terms;
const uint max_termsets = 1024;		uint ntermsets = 0; 		struct termset* termsets;
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
void pushg(struct ground** g, struct rule* r, struct subst* s); // push rule and subst pair into ground
void pushe(struct evidence** _e, struct term* t, struct ground* g); // push evidence item into evidence
struct subst* clone(struct subst* s); // deep-copy a subst
void pushq(struct queue**, struct proof* p); // push a proof to the back of a proofs queue
void unshift(struct queue** _q, struct proof* p); // push a proof to the front of the queue
void setsub(struct subst** s, int p, struct term* pr); // pushes a substitution
struct term* evaluate(struct term* p, struct subst* s); // evaluates a term given a subst
bool unify(struct term* s, struct subst* ssub, struct term* d, struct subst** dsub, bool f); // unification
void prove(struct termset* goal, struct ruleset* cases, bool interactive); // backchaining
bool equals(struct term* x, struct term* y); // deep-compare terms
void pushp(struct termset** l, struct term* p); // push a term to a termset
void pushr(struct ruleset** _rs, struct term* s, struct term* o); // push subject and object of s=>o into a ruleset
void printterm(struct term* p, struct dict* d); // print a term
struct term* term(int s, int p, int o); // create an s/p/o term
void prints(struct subst* s, struct dict* d); // print a subst
void printl(struct termset* l, struct dict* d); // print a termset
void printg(struct ground* g, struct dict* d); // print a ground struct
void printe(struct evidence* e, struct dict* d); // print evidence
void printei(struct evidence_item* ei, struct dict* d); // print single evidence step
void printr(struct rule* r, struct dict* d);  // print rule
void printps(struct term* p, struct subst* s, struct dict* d); // print a term with a subst
void printss(struct session* s); // print session memory
void printp(struct proof* p, struct dict* d); // print a proof element
void printrs(struct ruleset* rs, struct dict* d); // print a ruleset
void printq(struct queue* q, struct dict* d); // print a proof queue
struct ruleset* findruleset(struct ruleset* rs, int p); // find in a ruleset according to term
struct ruleset* find_or_create_rs_p(struct ruleset* rs, struct term* p); // finds a term in a ruleset or creates a new rule with that term if not exists
int pushw(struct dict** _d, const char* s); // push a string into a dictionary and get its int id
const char* dgetw(struct dict* d, int n); // decrypt string from dictionary given id
bool dgetwn(struct dict* d, const char* s, int* n); // returns the id of a given string in a dict
int readp(struct dict* d, struct ruleset** _r); // read a term from user and stores it in a ruleset
void readr(struct ruleset** _r); // read a rule from the user and stores it in a ruleset
void menu(struct session* ss); // run menu
void printcmd(const char* line, struct session*); // handle print commands
bool is_implication(int p); // identifies an implication resource
void trim(char *s); // trim strings from both ends using isspace
void str2term(const char* s, struct term** p, struct dict* d); // parses a string as term in partial basic N3
void term2rs(struct term* t, struct ruleset** r, bool fact); // converts a (complex) term into a ruleset by tracking the implication
struct ruleset* ts2rs(struct termset* t, bool fact); // same as term2rs but from termset
void throw(const char*);

struct dict* gdict; // global dictionary, currently the only used
int impl1, impl2, impl3;

const char implication1[] = "http://www.w3.org/2000/10/swap/log#implies";
const char implication2[] = "log:implies";
const char implication3[] = "=>";

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
All inputs that ends with '.' (facts) or '?' (queries) are interpreted as new terms or rules, otherwise as commands.\n\n\
A non-command line should contain triples, possibly nested with {} brackets (like N3). '=>' term is interpreted\n\
as implication (<http://www.w3.org/2000/10/swap/log#implies>) as well as simply log:implies.\n\
Commands:\n\n\
---------\n\n\
Printing commands:\n\
Syntax: <command> [id] where kb selects kb to list the desired values, and id of the desired value.\n\
If id is not specified, all values from the specified kind will belisted.\n\
pp [id]\tprints terms.\n\
pr [id]\tprints rules.\n\
ps [id]\tprints substitutions.\n\
pg [id]\tprints grounds.\n\
pq [id]\tprints queues.\n\
ppr [id]\tprints proofs.\n\
pd [id]\tprints dicts.\n\
pl [id]\tprints term lists.\n\
pll [id]\tprints rule lists.\n\
pe [id]\tprints evidence.\n\
pss [id]\tprint session.\n\
\n\nProof commands:\n\n\
qi\tinteractive proof.\n\
qa\tprocess all frames until full resolution.\n\
\n\nSession commands:\n\n\
Each session contain a proof queue, a kb (rule list), a goal (term list) and a dict.\n\
Goal and kb are represented both as terms and as implication tree (rules).\n\
sc\tClear session\n\
sn\tCreate session\n\
sw <id>\tSwitch session\n\n";

const char prompt[] = "tau >> ";

#define syn_err { printf("Syntax error.\n"); continue; }
#define str_input_len 255

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

void throw(const char* s) {
	puts(s);
	free(0);
}

bool has_no_spaces(const char* s) {
	uint len = strlen(s);
	while (len--)
		if (isspace(s[len]))
			return false;
	return true;
}

void str2term(const char* s, struct term** _p, struct dict* d) {
	if (!s || !*s) {
		*_p = 0;
		return;
	}
	if (!*_p)
		*_p = &terms[nterms++];
	struct term* p = *_p;
	p->p = 0;
	p->s = 0;//&terms[nterms++];
	p->o = 0;//&terms[nterms++];
	int n = 0, m, k;
	int braces = 0;
	bool had_braces = false;
	char pr[str_input_len], __s[str_input_len];
	const char* _s = s;
	while (*s) {
		if (*s == '{') {
			had_braces = true;
			++braces;
		}
		else if (*s == '}') {
			--braces;
			++s;
		}
		if (!braces) {
			while (isspace(*s) && *s)
				++s;
			if (had_braces)
				m = s - _s;
			while (!isspace(*s) && *s) {
				if (*s == '{' || *s == '}')
					syn_err;
				if (had_braces)
					pr[n++] = *s;
				++s;
			}
			if (!had_braces)
				m = s - _s;
			while (isspace(*s) && *s)
				++s;
			if (!had_braces)
				while (!isspace(*s) && *s) {
					if (*s == '{' || *s == '}')
						syn_err;
					pr[n++] = *s++;
				}
			pr[n] = 0;
			p->p = pushw(&d, pr);
			strcpy(__s, s);
			trim(__s);
			if (strlen(__s)) {
				if (has_no_spaces(__s)) {
					int id = pushw(&d, __s);
					for (k = 0; k < nterms; ++k)
						if (terms[k].p == id)
							if (!terms[k].s && !terms[k].o)
								p->o = &terms[k];
					if (!p->o) {
						p->o = &terms[nterms++];
						p->o->p = id;
						p->o->s = p->o->o = 0;
					}
				}
				else
					str2term(__s, &p->o, d);
			}
			else
				p->o = 0;
			memcpy(__s, _s, m);
			__s[m] = 0;
			trim(__s);
			if (strlen(__s)) {
				if (has_no_spaces(__s)) {
					int id = pushw(&d, __s);
					for (k = 0; k < nterms; ++k)
						if (terms[k].p == id)
							if (!terms[k].s && !terms[k].o)
								p->s = &terms[k];
					if (!p->s) {
						p->s = &terms[nterms++];
						p->s->p = id;
						p->s->s = p->s->o = 0;
					}
				}
				else 
					str2term(__s, &p->s, d);
			}
			else
				p->s = 0;
			break;
		}
		++s;
	}
}

void printss(struct session* ss) {
	printf("Queue:\n");
	printq(ss->q, ss->d);
	printf("KB terms:\n");
	printl(ss->kb, ss->d);
	printf("Goal terms:\n");
	printl(ss->goal, ss->d);
	printf("KB rules:\n");
	printrs(ss->rkb, ss->d);
//	printf("Goal rules:\n");
//	printrs(ss->rgoal, ss->d);
}

void printcmd(const char* line, struct session* ss) {
	struct dict* d = ss->d;
	int id;
	uint n = 0;
	do 
	if (read_num(line, &id)) {
		switch (*line) {
		case 'p':
			++line;
			if (!*line || isspace(*line)) {
				printterm(&terms[id], d);
				puts("");
			} else if (*line == 'r') {
				printp(&proofs[id], d);
				puts("");
			}
			else
				syn_err;
			break;
		case 'r':
			printr(&rules[id], d);
			puts("");
			break;
		case 's':
			++line;
			if (!*line || isspace(*line)) {
				prints(&substs[id], d);
				puts("");
			} else if (*line == 's') {
				printss(ss);
				puts("");
			}
			break;
		case 'g':
			printg(&grounds[id], d);
			puts("");
			break;
		case 'q':
			printq(&queues[id], d);
			puts("");
			break;
		case 'e':
			printe(&evidences[id], d);
			puts("");
			break;
		case 'l':
			++line;
			if (!*line || isspace(line)) {
				printl(&termsets[id], d);
				puts("");
			} else {
				if (*line == 'l') {
					printrs(&rulesets[id], d);
					puts("");
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
			++line;
			if (!*line || isspace(*line)) {
				puts("Predicates:\n");
				for (; n < nterms; ++n) {
					printf("%d\t", n);
					printterm(&terms[n], d);
					puts("");
				}
			}
			else {
				if (*line == 'r') {
					puts("Proof steps:\n");
					for (; n < nproofs; ++n) {
						printf("%d\t", n);
						printp(&proofs[n], d);
						puts("");
					}
				}
				else
					syn_err;
			}
			break;
		case 'r':
			puts("Rules:\n");
			for (; n < nrules; ++n) {
				printf("%d\t", n);
				printr(&rules[n], d);
				puts("");
			}
			break;
		case 's':
			++line;
			if (!*line || isspace(*line)) {
				puts("Substitutions:\n");
				for (; n < nsubsts; ++n) {
					printf("%d\t", n);
					prints(&substs[n], d);
					puts("");
				}
			} else if (*line == 's') {
				puts("Session:\n");
				printss(ss);
				puts("");
			}
			break;
		case 'g':
			puts("Grounds:\n");
			for (; n < ngrounds; ++n) {
				printf("%d\t", n);
				printg(&grounds[n], d);
				puts("");
			}
		case 'q':
			puts("Queues:\n");
			for (; n < nqueues; ++n) {
				printf("%d\t", n);
				printq(&queues[n], d);
				puts("");
			}
			break;
		case 'e':
			puts("Evidence:\n");
			for (; n < nevidences; ++n) {
				printf("%d\t", n);
				printe(&evidences[n], d);
				puts("");
			}
			break;
		case 'l':
			++line;
			if (!*line || isspace(*line)) {
				puts("Predicate lists:\n");
				for (; n < ntermsets; ++n) {
					printf("%d\t", n);
					printl(&termsets[n], d);
					puts("");
				}
			}
			else {
				if (*line == 'l') {
					puts("Rule lists:\n");
					for (; n < nrulesets; ++n) {
						printf("%d\t", n);
						printrs(&rulesets[n], d);
						puts("");
					}
				}
				else
					syn_err;
			}
			break;
		}
	}
	while(0);
}

// session has to be initialized by the caller
void menu(struct session* ss) {
	char *line, *_line;
	gdict = ss->d;
	puts(main_menu);
	printf("Session id: %p\n", ss);
	rl_bind_key('\t', rl_complete);
	while ((_line = readline(prompt))) {
		line = _line;
		if (!line)
			continue;
		add_history(line);
		uint szline = strlen(line);
		if (szline) {
			if ((line[szline - 1] == '.' || line[szline - 1] == '?') && szline > 1 ) {
				bool isq = line[szline - 1] == '?';
				line[szline - 1] = 0;
				trim(line);
				struct term* t = &terms[nterms++];
				str2term(line, &t, ss->d);
//				printf("String %s parsed into term as:\n", line);
				printterm(t, ss->d);
				puts("");
				if (isq) 
					pushp(&ss->goal, t);
				else
					pushp(&ss->kb, t);
			} else {
				trim(line);
				if (!strcmp(line, "?")) 
					printf(help);
				else if (*line == 'q') {
					++line;
					if (*line == 'a') {
						ss->rkb = ts2rs(ss->kb, true);
						prove(ss->goal, ss->rkb, false);
					}
					else if (*line == 'i') {
						ss->rkb = ts2rs(ss->kb, true);
						prove(ss->goal, ss->rkb, true);
					}
				}
				else if (*line == 'p') {
					++line;
					if (!*line || isspace(*line))
						syn_err;
					printcmd(line, ss);
					if (*line == 'a') {
						printcmd("p", ss);
						printcmd("pr", ss);
						printcmd("r", ss);
						printcmd("s", ss);
						printcmd("g", ss);
						printcmd("q", ss);
						printcmd("e", ss);
						printcmd("l", ss);
						printcmd("ll", ss);
					}
				}
			}
		}
		free(_line);
	}
}

void initmem() {
	memset(terms	 	= malloc(sizeof(struct term) * max_terms), 			0, sizeof(struct term) * max_terms);
	memset(termsets 	= malloc(sizeof(struct termset) * max_termsets), 		0, sizeof(struct termset) * max_termsets);
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
	uint len = strlen(s), n;
	int brackets;
	if (!len)
		return;
	while ( isspace ( *s ) && len ) {
		for (n = 0; n < len - 1; ++n)
			s[n] = s[n + 1];
		--len;
	}
	while ( len && isspace ( s[len - 1] ))
		--len;
	s[len] = 0;
	if (len <= 1)
		return;
	if (*s == '{' && s[len-1] == '}') {
		brackets = 1;
		for (n = 1; n < len - 1; ++n) {
			if (s[n] == '{')
				brackets++;
			if (s[n] == '}')
				brackets--;
			if (!brackets)
				return;
		}
		s[len - 1] = 0;
		*s = ' ';
		trim(s);
	}
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

struct ruleset* find_or_create_rs_p(struct ruleset* rs, struct term* p) {
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
	prev->next->next = 0;
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

void pushg(struct ground** _g, struct rule* r, struct subst* s) {
	if (!r)
		throw("Error: ried to add a null rule!\n");
	struct ground* g;
	if (!*_g) {
		g = &grounds[ngrounds++];
		g->r = r;
		g->s = s;
		g->next = 0;
		*_g = g;
		return;
	}
	g = *_g;
	while (g->next) 
		g = g->next;
	g->next = &grounds[ngrounds++];
	g->next->r = r;
	g->next->s = s;
	g->next->next = 0;
}

void pushe(struct evidence** _e, struct term* t, struct ground* g) {
	struct evidence* e;
	struct evidence_item* ei = &evidence_items[nevidence_items++];
	ei->p = t;
	ei->g = g;
	ei->next = 0;
	if (!*_e) {
		(*_e) = &evidences[nevidences++];
		(*_e)->p = t->p;
		(*_e)->item = ei;
		(*_e)->next = 0;
		return;
	}
	e = *_e;
	while (e->next && e->p != t->p)
		e = e->next;
	if (e->p == t->p) {
		struct evidence_item* i = e->item;
		while (i->next)
			i = i->next;
		i->next = ei;
		return;
	}
	e->next = &evidences[nevidences++];
	e->next->p = t->p;
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

void pushq(struct queue** _q, struct proof* p) {
	if (!p)
		printf("Error: pushq called with null proof\n");
	if (!*_q) {
		*_q = &queues[nqueues++];
		(*_q)->next = (*_q)->prev = 0;
		(*_q)->p = p;
		return;
	}
	struct queue* q = &queues[nqueues++];
	q->p = p;
	while ((*_q)->next)
		*_q = (*_q)->next;
	(*_q)->next = q;
	q->prev = (*_q);
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
	while ((*_q)->prev)
		_q = &(*_q)->prev;
	(*_q)->prev = q;
	q->prev = 0;
	q->next = *_q;
	*_q = q;
}

void term2rs(struct term* t, struct ruleset** r, bool fact) {
	if (!t)
		return;
	if (is_implication(t->p))
		pushr(r, t->s, t->o);
	else if (fact)
		pushr(r, 0, t);
	else pushr(r, t, 0);
}

struct ruleset* ts2rs(struct termset* t, bool fact) {
	struct ruleset* r = 0;
	do {
		term2rs(t->p, &r, fact);
	} while ((t = t->next));
	return r;
}

void setsub(struct subst** s, int p, struct term* pr) {
	struct subst* _s = &substs[nsubsts++];
	_s->p = p;
	_s->pr = pr;
	_s->next = 0;
	if (!*s) 
		*s = _s;
	else {
		while ((*s)->next) 
			*s = (*s)->next;
		(*s)->next = _s;
	}
}

struct term* evaluate(struct term* p, struct subst* s) {
	if (p->p < 0) {
		struct subst* _s = findsubst(s, p->p);
		if (!_s)
			return 0;
		return evaluate(_s->pr, s);
	}
	if (!p->s && !p->o)
		return p;
	struct term *a = evaluate(p->s, s), *r = &terms[nterms++];
	r->p = p->p;
	if (a)
		r->s = a;
	else {
		r->s = &terms[nterms++];
		r->s->p = p->s->p;
		r->s->s = r->s->o = 0;
	}
	a = evaluate(p->o, s);
	if (a)
		r->o = a;
	else {
		r->o = &terms[nterms++];
		r->o->p = p->o->p;
		r->o->s = r->o->o = 0;
	}
	return r;
}

void printps(struct term* p, struct subst* s, struct dict* d) {
	printterm(p, d);
	printf(" [ ");
	prints(s, d);
	printf(" ] ");
}

bool unify(struct term* s, struct subst* ssub, struct term* d, struct subst** dsub, bool f) {
	if (s->p < 0) {
		struct term* sval = evaluate(s, ssub);
		if (sval)
			return unify(sval, ssub, d, dsub, f);
		return true;
	}
	if (d->p < 0) {
		struct term* dval = evaluate(d, *dsub);
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

void prove(struct termset* goal, struct ruleset* cases, bool interactive) {
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
	struct evidence* e = 0;
	do {
		struct queue *q = qu;
		while (q && q->next) 
			q = q->next;
		struct proof* p = q->p;
		if (q->prev)
			q->prev->next = 0;
		else
			qu = 0;
		if (p->last) {
			struct term* t = p->last->p;
			printf("Trying to track back from ");
			printterm(t, gdict);
			puts("");
			struct ruleset* rs = cases;
//			printf("cases:\n");
//			printrs(rs, gdict);
			while ((rs = findruleset(rs, t->p))) {
				struct subst* s = 0;
				printf("\tTrying to unify ");
				printps(t, p->s, gdict);
				printf(" and ");
				printps(rs->r->p, s, gdict);
				printf("... ");
				if (unify(t, p->s, rs->r->p, &s, true)) {
					printf("\tunification succeeded");
					if (s) {
						printf(" with new substitution: ");
						prints(s, gdict);
					}
					puts("");
					if (euler_path(p))
						continue;
					struct proof* r = &proofs[nproofs++];
					r->rul = rs->r;
					printf("\t\tPushed frame with rule ");
					printr(r->rul, gdict);
					r->last = r->rul->body;//p->last->next;
					r->prev = p;
					r->s = 0;
					r->g = copyg(p->g);
					if (!rs->r->body) {
						printf(" adding ground ");
						pushg( &r->g, rs->r, 0 );
						printg(r->g, gdict);
					}
					puts(".");
					unshift(&qu, r);
//					printf("pushed frame...\n");
//					printp(r, gdict);
//					printf("queue:\n");
//					printq(qu, gdict);
				} else
					printf("\tunification failed\n");
				rs = rs->next;
			}
		}
		else if (!p->prev) {
			struct termset* r;
			for (r = p->rul->body; r; r = r->next) { 
				struct term* t = evaluate(r->p, p->s);
				pushe(&e, t, p->g);
			}
			puts("Cannot longer go backward. Evidence we currently have:");
			printe(e, gdict);
		} else {
			struct ground* g = 0;
			printf("Finished one level.");
			if (p->rul->body) {
				printf(" Adding ground: ");
				pushg(&g, p->rul, p->s);
				printg(g, gdict);
			}
			printf(" Going a frame back from rule: ");
			printr(p->prev->rul, gdict);
			puts(".");
			struct proof* r = &proofs[nproofs++];
			*r = *p->prev;
			r->g = g;
			r->s = clone(p->prev->s);
			unify(p->rul->p, p->s, r->last->p, &r->s, true);
			r->last = r->last->next;
			pushq(&qu, r);
			continue;
		}
	} while (qu);
	printe(e, gdict);
}

bool equals(struct term* x, struct term* y) {
	return (!x == !y) && x && equals(x->s, y->s) && equals(x->o, y->o);
}

void pushp(struct termset** _l, struct term* p) {
	if (!p)
		return;
	if (!*_l) {
		*_l = &termsets[ntermsets++];
		(*_l)->p = p;
		(*_l)->next = 0;
		return;
	}
	struct termset* l = *_l;
	while (l->next && !equals(p, l->p))
		l = l->next;
	if (l->next) 
		return; // already in
	l->next = &termsets[ntermsets++];
	l->next->p = p;
	l->next->next = 0;
}

void pushr(struct ruleset** _rs, struct term* s, struct term* o) {
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

void printterm(struct term* p, struct dict* d) {
	const char* s;
	if (!p)
		return;
	printterm(p->s, d);
	if (!d)
		printf(" %d ", p->p);
	else {
		s = dgetw(d, p->p);
		if (s)
			printf(" %s ", s);
	}
	printterm(p->o, d);
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

struct term* term(int s, int p, int o) {
	uint n = 0;
	for (; n < nterms; ++n) {
		struct term* pn = &terms[n];
		if (pn->p == p && pn->s && pn->o)
			if (pn->s->p == s && !pn->s->o && !pn->s->s)
				if (pn->o->p == o && !pn->o->o && !pn->o->s)
					return pn;
	}
	struct term* ps = &terms[nterms++];
	struct term* pp = &terms[nterms++];
	struct term* po = &terms[nterms++];
	pp->p = p;
	pp->s = ps;
	pp->o = po;
	ps->s = ps->o = po->s = po->o = 0;
	ps->p = s;
	po->p = o;
	return pp;
}

void prints(struct subst* s, struct dict* d) {
	if (!s)
		return;
	const char* _s = dgetw(d, s->p);
	if (_s)
		printf("%s / ", _s);
	printterm(s->pr, d);
	if (s->next)
		printf(", ");
	prints(s->next, d);
}

void printl(struct termset* l, struct dict* d) {
	if (!l)
		return;
	printterm(l->p, d);
	printf(", ");
	printl(l->next, d);
}

void printr(struct rule* r, struct dict* d) {
	if (!r)
		return;
	printl(r->body, d);
	printf(" => ");
	printterm(r->p, d);
}

void printg(struct ground* g, struct dict* d) {
	if (!g)
		return;
//	printf("ground: ");
	printr(g->r, d);
	printf(" ");
	prints(g->s, d);
//	printf("\n");
	if (g->next) {
		printf("; ");
		printg(g->next, d);
	}
	else
		printf(". ");
}

void printei(struct evidence_item* ei, struct dict* d) {
	if (!ei)
		return;
	printf("evidence_item:\n");
	printterm(ei->p, d);
	printf("\n");
	printg(ei->g, d);
	printf("\n");
	printei(ei->next, d);
}

void printe(struct evidence* e, struct dict* d) {
	if (!e)
		return;
	if (!d)
		printf("%d:\n", e->p);
	else {
		const char *s = dgetw(d, e->p);
		if (s)
			printf(" %s ", s);
	}

	printei(e->item, d);
	printe(e->next, d);
}

bool is_implication(int p) {
	return p == impl1 || p == impl2 || p == impl3;
}

int pushw(struct dict** _d, const char* s) {
	int c = 1, n;
	if (dgetwn(*_d, s, &n))
		return n;
	if (!*_d) {
		*_d = &dicts[ndicts++];
		(*_d)->next = 0;
		(*_d)->s = malloc(strlen(s) + 1);
		strcpy((*_d)->s, s);
		if (s[0] && (s[0] == '?' || s[0] == '_'))
			c = -c;
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
	if (s[0] && (s[0] == '?' || s[0] == '_'))
		c = -c;
	return d->n = c;
}

const char* dgetw(struct dict* d, int n) {
	if (!d)
		return 0;
	if (d->n == n)
		return d->s;
	return dgetw(d->next, n);
}

bool dgetwn(struct dict* d, const char* s, int* n) {
	if (!d || !s)
		return false;
	if (d->s && !strcmp(d->s, s)) {
		*n = d->n;
		return true;
	}
	return dgetwn(d->next, s, n);
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
	r->r->p = term(*ns, *np, *no);

	return (r->r->p - terms)/sizeof(struct term);
}

int readp(struct dict* d, struct ruleset** _r) {
	char s[str_input_len], p[str_input_len], o[str_input_len];
	int ns, np, no;
	int pid = _readp(d, _r, s, p, o, &ns, &np, &no);
	if (pid != -1)
		printf("term read: %s %s %s ( %d %d %d ) Predicate id: %d\n ", s, p, o, ns, np, no, pid);
	else
		printf("Error.\n");
	return pid;
}

void readr(struct ruleset** r) {
	char s[16], o[16], ch;
	uint pos = 0;
	int ns, no;
	printf("Enter two numbers separated by spaces ('s o') such that term s implies term o: ");
	while ((ch = getchar()) != ' ')
		s[pos++] = ch;
	pos = 0;
	while ((ch = getchar()) != '\n')
		o[pos++] = ch;
	ns = atoi(s);
	no = atoi(o);
	pushr(r, &terms[ns], &terms[no]);
}

int main(int argc, char* argv[]) {
	initmem();
	gdict = &dicts[ndicts++];

	struct session ss;
	ss.kb = 0;
	ss.goal = 0;
	ss.d = 0;
	ss.q = 0;
	impl1 = pushw(&ss.d, implication1); // define implication
	impl2 = pushw(&ss.d, implication2); 
	impl3 = pushw(&ss.d, implication3);

	menu(&ss);
	return 0;
}
