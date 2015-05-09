//#define READLINE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifdef READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include <iostream>
#include "prover.h"
using namespace std;

namespace prover {
const uint max_terms = 1024*1024;		uint nterms = 0; 		term* terms;
const uint max_termsets = 1024*1024;		uint ntermsets = 0; 		termset* termsets;
const uint max_rules = 1024*1024;		uint nrules = 0; 		rule* rules;
const uint max_substs = 1024*1024; 		uint nsubsts = 0; 		subst* substs;
const uint max_rulesets = 1024*1024; 	uint nrulesets = 0; 		ruleset* rulesets;
const uint max_grounds = 1024*1024; 		uint ngrounds = 0; 		ground* grounds;
const uint max_proofs = 1024*1024; 		uint nproofs = 0; 		proof* proofs;
const uint max_queues = 1024*1024; 		uint nqueues = 0; 		queue* queues;
const uint max_evidence_items = 1024*1024; 	uint nevidence_items = 0; 	evidence_item* evidence_items;
const uint max_evidences = 1024*1024; 	uint nevidences = 0; 		evidence* evidences;
const uint max_dicts = 1024*1024; 		uint ndicts = 0; 		dict* dicts;

void printps(term* p, subst* s, dict* d); // print a term with a subst
ruleset* find_or_create_rs_p(ruleset* rs, term* p);	// finds a term in a ruleset or creates a new rule with that term if not exists
void str2term(const char* s, term** p, dict* d);	// parses a string as term in partial basic N3
void term2rs(term* t, ruleset** r, bool fact, dict** d);// converts a (complex) term into a ruleset by tracking the implication
ruleset* ts2rs(termset* t, bool fact);	// same as term2rs but from termset

//dict* ss->d; // global dictionary, currently the only used
//session* gsession;
int impl1, impl2, impl3;
int dict_counter = 1;

const char implication1[] = "log:implies";
const char implication2[] = "=>";
const char implication3[] = "http://www.w3.org/2000/10/swap/log#implies";

term* GND;

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
#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(X)
#endif

#define PUSH_FUNC(set, item, member, sets, nsets) \
	void push##item(set** s, item* i) { \
	if (!*set) { \
		*set = &sets[nsets]; \
		(*set)->m = i; \
		(*set)->next = 0; \
	} \
	set* ss = *set; \
	while (ss->next) \
		ss = ss->next; \
	ss->next = &sets[nsets++]; \
	ss->next->m = i; \
	ss->next->next = 0;


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

bool has_no_spaces(const char* s) {
	uint len = strlen(s);
	while (len--)
		if (isspace(s[len]))
			return false;
	return true;
}
/*
struct quad {
	const char *s, *p, *o, *c;
};

ruleset* quad(quad* qs, uint sz, session* ss, bool isq) {
	ruleset* rs = 0;
	quad* qq = qs;
	uint n;
	while (qs != &qq[sz]) {
		term* t = &terms[nterms++];
		t->p = pushw(&ss->d, qs->p);
		t->s = pushw(&ss->d, qs->s);
		t->o = pushw(&ss->d, qs->o);
		if (is_implication(t->p)) 
			for (n = 0; n < sz; ++n) {
				if (strcmp(qq[k].c, qs->s))
					continue;
				rule* r = &rules[nrules++];
				r->p = mkterm(qq[n]);
				r->body = 0;
				for (k = 0; k < sz; ++k)
					if (!strcmp(qq[k].c, qs->o))
						pushp(&r->body, qq[k].o);
			}
		
	}
}

rule* quad(int p, term* s, term* o, bool isq) {
	rule* r = &rules[nrules++];
	r->p = 0;
	r->body = 0;
	if (is_implication(p)) {
		r->p = o;
		pushp(&r->body, s);
	}
	term* t = &terms[nterms++];
	t->p = p;
	t->s = s;
	t->o = o;
	if (isq)
		pushp(&r->body, t);
	else
		r->p = t;
	return r;
}
*/

void str2term(const char* s, term** _p, dict* d) {
	if (!s || !*s) {
		*_p = 0;
		return;
	}
	if (!*_p)
		*_p = &terms[nterms++];
	term* p = *_p;
	p->p = 0;
	p->s = 0;//&terms[nterms++];
	p->o = 0;//&terms[nterms++];
	int n = 0, m;
	size_t k;
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
					return;
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
					if (*s == '{' || *s == '}') {
						return;
					}
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

void printss(session* ss) {
	printf("Queue:\n");
	printq(ss->q, ss->d);
	printf("KB terms:\n");
	printl(ss->kb, ss->d);
	printf("Goal terms:\n");
	printl(ss->goal, ss->d);
	printf("KB rules:\n");
	printrs(ss->rkb, ss->d);
	printf("Dictionary:\n");
	dict* d = ss->d;
	while (d) {
		printf("%d = %s\n", d->n, d->s);
		d = d->next;
	}
//	printf("Goal rules:\n");
//	printrs(ss->rgoal, ss->d);
}

void printcmd(const char* line, session* ss) {
	dict* d = ss->d;
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
			if (!*line || isspace(*line)) {
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

#ifndef READLINE
char* readline(const char* p) {
	char* l = new char[str_input_len];
	size_t sz = str_input_len;
	printf("%s ", p);
	if (!cin.getline(l, sz))
		return 0;
	l[strlen(l) - 1] = 0;
	return l;
}
#endif

// session has to be initialized by the caller
void menu(session* ss) {
	char *line, *_line;
	puts(main_menu);
	printf("Session id: %p\n", (void*)ss);
#ifdef READLINE	
	rl_bind_key('\t', rl_complete);
#endif
	while ((_line = readline(prompt))) {
		line = _line;
		if (!line)
			continue;
#ifdef READLINE			
		add_history(line);
#endif
		uint szline = strlen(line);
		if (szline) {
			if ((line[szline - 1] == '.' || line[szline - 1] == '?') && szline > 1 ) {
				bool isq = line[szline - 1] == '?';
				if (isq)
					line[szline - 1] = '.';
				trim(line);
//				pushr(
				term* t = &terms[nterms++];
//				pushr(isq ? &ss->goal : ss->
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
						prove(ss);
					}
					else if (*line == 'i') {
						ss->rkb = ts2rs(ss->kb, true);
						prove(ss);
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
				} else if (*line == 's') {
					++line;
					if (*line == 'c') {
						initmem(false);
						puts("Memory cleared.");
					}
				}
			}
		}
		free(_line);
	}
}

// set alloc to false in order to only zero the memory
void initmem(bool) {
	terms	 	= new term[max_terms];
	termsets 	= new termset[max_termsets];
	rules 		= new rule[max_rules];
	substs 		= new subst[max_substs];
	rulesets 	= new ruleset[max_rulesets];
	grounds		= new ground[max_grounds];
	proofs 		= new proof[max_proofs];
	dicts 		= new dict[max_dicts];
	queues 		= new queue[max_queues];
	evidences 	= new evidence[max_evidences];
	evidence_items = new evidence_item[max_evidence_items];
	nterms = ntermsets = nrules = nsubsts = nrulesets = ngrounds = nproofs = nqueues = nevidence_items = nevidences = ndicts = 0;
//	ss->d = 0;//&dicts[ndicts++];
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


subst* findsubst(subst* l, int m) { 
	while (l) {
		if (l->p == m) 
			return l;
		l = l->next;
	}
	return 0; 
}

ruleset* findruleset(ruleset* rs, int p) {
	for (; rs; rs = rs->next)
		if (rs->r && rs->r->p && rs->r->p->p == p)
			break;
	return rs;
}

ruleset* find_or_create_rs_p(ruleset* rs, term* p) {
	ruleset* prev = 0;
	while (rs) {
		if (/*rs->r && rs->r->p &&*/ equals(rs->r->p, p))
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

ground* copyg(ground* g) {
	if (!g)
		return 0;
	ground* r = &grounds[ngrounds++];
	r->r = g->r;
	r->s = g->s;
	r->next = copyg(g->next);
	return r;
}

void pushg(ground** _g, rule* r, subst* s) {
	if (!r)
		throw("Error: ried to add a null rule!\n");
	ground* g;
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

void pushe(evidence** _e, term* t, ground* g) {
	evidence* e;
	evidence_item* ei = &evidence_items[nevidence_items++];
	ei->p = t;
	ei->g = g;
	ei->next = 0;
	if (!*_e) {
		(*_e) = &evidences[nevidences++];
		(*_e)->next = 0;
		(*_e)->p = t->p;
		(*_e)->item = ei;
		return;
	}
	e = *_e;
	while (e->next && e->p != t->p)
		e = e->next;
	if (e->p == t->p) {
		evidence_item* i = e->item;
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

subst* clone(subst* s) {
	if (!s) 
		return 0;
	subst* r = &substs[nsubsts++];
	r->p = s->p;
	r->pr = s->pr;
	r->next = clone(s->next);
	return s;
}

void pushq(queue** _q, proof* p) {
	if (!p)
		printf("Error: pushq called with null proof\n");
	if (!*_q) {
		*_q = &queues[nqueues++];
		(*_q)->next = (*_q)->prev = 0;
		(*_q)->p = p;
		return;
	}
	queue* q = *_q;
	while (q->next)
		q = q->next;
	q->next = &queues[nqueues++];
	q->next->p = p;
	q->next->prev = q;
	q->next->next = 0;
}

void unshift(queue** _q, proof* p) {
	if (!p)
		printf("Error: unshift called with null proof\n");
	if (!*_q) {
		pushq(_q, p);
		return;
	}
	queue* q = &queues[nqueues++];
	while ((*_q)->prev)
		_q = &(*_q)->prev;
	q->p = p;
	(*_q)->prev = q;
	q->prev = 0;
	q->next = *_q;
	*_q = q;
}

void term2rs(term* t, ruleset** r, bool fact) {
	if (!t)
		return;
	if (is_implication(t->p)) 
		pushr(r, t->s, t->o);
	else {
		if (fact)
			pushr(r, 0, t);
		else 
			pushr(r, t, 0);
	}
}

ruleset* ts2rs(termset* t, bool fact) {
	ruleset* r = 0;
	do {
		term2rs(t->p, &r, fact);
	} while ((t = t->next));
	return r;
}

void setsub(subst** s, int p, term* pr) {
	subst* _s = &substs[nsubsts++];
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

term* evaluate(term* p, subst* s) {
	if (p->p < 0) {
		subst* _s = findsubst(s, p->p);
		if (!_s)
			return 0;
		return evaluate(_s->pr, s);
	}
	if (!p->s && !p->o)
		return p;
	term *a = evaluate(p->s, s), *r = &terms[nterms++];
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

void printps(term* p, subst* s, dict* d) {
	printterm(p, d);
	printf(" [ ");
	prints(s, d);
	printf(" ] ");
}

bool unify(term* s, subst* ssub, term* d, subst** dsub, bool f) {
	if (s->p < 0) {
		term* sval = evaluate(s, ssub);
		if (sval)
			return unify(sval, ssub, d, dsub, f);
		return true;
	}
	if (d->p < 0) {
		term* dval = evaluate(d, *dsub);
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

bool euler_path(proof* p) {
	proof* ep = p;
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

int builtin(term*, proof*, session*) {
//	const char* s = dgetw(ss->d, t->p);
//	if (s && !strcmp(s, "GND"))	
//		return 1;
	return -1;
}
/*
void queue_test() {
	queue *qu = 0;//&queues[nqueues++];
	int n = 0;
	pushq(&qu, 10);
	queue* qq = qu;
	do {
		printf("step: %d data: %d\n", n, (int)qq->p);
	} while (qq = qq->next);
	for (; n < 7; ++n) {
		queue *q = qu;
		while (q && q->next) 
			q = q->next;
		proof* p = q->p;
		if (q->prev)
			q->prev->next = 0;
		else
			qu = 0;
		if (!(n%3))
			pushq(&qu, n+1);
//		else if (!(n%2))
			unshift(&qu, (n + 1) * 10);
		qq = qu;
		if (!qq) {
			printf("step: %d queue is null\n", n);
		}
		else do {
			printf("step: %d data: %d\n", n, (int)qq->p);
		} while (qq = qq->next);
	}
}
*/
void prove(session* ss) {
	termset* goal = ss->goal;
	ruleset* cases = ss->rkb;
	queue *qu = 0;
	rule* rg = &rules[nrules++];
	proof* p = &proofs[nproofs++];
	ss->e = 0;
	rg->p = 0;
	rg->body = goal;
	p->rul = rg;
	p->s = 0;
	p->g = 0;
	p->last = rg->body;
	p->prev = 0;
	pushq(&qu, p);
	TRACE(puts("\nprove() called with facts:"));
	TRACE(printrs(cases, ss->d));
	TRACE(puts("\nand query:"));
	TRACE(printl(goal, ss->d));
	TRACE(puts(""));
	do {
		queue *q = qu;
		while (q && q->next) 
			q = q->next;
		p = q->p;
		if (q->prev)
			q->prev->next = 0;
		else
			qu = 0;
		if (p->last) {
			term* t = p->last->p;
			TRACE(printf("Tracking back from "));
			TRACE(printterm(t, ss->d));
			TRACE(puts(""));
			int b = builtin(t, p, ss);
			if (b == 1) {
				proof* r = &proofs[nproofs++];
				rule* rl = &rules[nrules++];
				*r = *p;
				r->g = 0;
				rl->p = evaluate(t, p->s);
				rl->body = 0;
				r->g = copyg(p->g);
				pushg(&r->g, rl, 0);
				r->last = r->last->next;
				pushq(&qu, r);
		            	continue;
		        }
		    	else if (!b) 
				continue;

			ruleset* rs = cases;
			while ((rs = findruleset(rs, t->p))) {
				subst* s = 0;
				TRACE(printf("\tTrying to unify "));
				TRACE(printps(t, p->s, ss->d));
				TRACE(printf(" and "));
				TRACE(printps(rs->r->p, s, ss->d));
				TRACE(printf("... "));
				if (unify(t, p->s, rs->r->p, &s, true)) {
					(printf("\tunification succeeded"));
					if (s) {
						TRACE(printf(" with new substitution: "));
						TRACE(prints(s, ss->d));
					}
					TRACE(puts(""));
					if (euler_path(p)) {
						rs = rs->next;
						continue;
					}
					proof* r = &proofs[nproofs++];
					r->rul = rs->r;
					r->last = r->rul->body;
					r->prev = p;
					r->s = s;
					r->g = copyg(p->g);
					if (!rs->r->body) 
						pushg( &r->g, rs->r, 0 );
					unshift(&qu, r);
//					TRACE(printf("queue:\n"));
//					TRACE(printq(qu, ss->d));
				} else {
					TRACE(printf("\tunification failed\n"));
				}
				rs = rs->next;
			}
		}
		else if (!p->prev) {
			termset* r;
			for (r = p->rul->body; r; r = r->next) 
				pushe(&ss->e, evaluate(r->p, p->s), p->g);
			TRACE(printf("no prev frame. queue:\n"));
//			TRACE(printq(qu, ss->d));
		} else {
			ground* g = copyg(p->g);
			proof* r = &proofs[nproofs++];
			if (p->rul->body)
				pushg(&g, p->rul, p->s);
			*r = *p->prev;
			r->g = g;
			r->s = clone(p->prev->s);
			unify(p->rul->p, p->s, r->last->p, &r->s, true);
			r->last = r->last->next;
			pushq(&qu, r);
			TRACE(printf("finished a frame. queue:\n"));
//			TRACE(printq(qu, ss->d));
			continue;
		}
	} while (qu);
	puts("\nEvidence:");
	puts("=========");
	printe(ss->e, ss->d);
}

bool equals(term* x, term* y) {
	return (!x == !y) && x && equals(x->s, y->s) && equals(x->o, y->o);
}

void pushp(termset** _l, term* p) {
	if (!p)
		return;
	if (!*_l) {
		*_l = &termsets[ntermsets++];
		(*_l)->p = p;
		(*_l)->next = 0;
		return;
	}
	termset* l = *_l;
	while (l->next && !equals(p, l->p))
		l = l->next;
	if (l->next) 
		return; // already in
	l->next = &termsets[ntermsets++];
	l->next->p = p;
	l->next->next = 0;
}

void pushr(ruleset** _rs, rule* r) {
	if (!*_rs) {
		*_rs = &rulesets[nrulesets++];
		(*_rs)->r = r;
		(*_rs)->next = 0;
	}
	ruleset* rs = *_rs;
	if (rs->r == r)
		return;
	while (rs->next) {
		rs = rs->next;
		if (rs->r == r)
			return;
	}
	rs->next = &rulesets[nrulesets++];
	rs->next->r = r;
	rs->next->next = 0;
}

void pushr(ruleset** _rs, term* s, term* o) {
	ruleset* rs;
	if (!*_rs) {
		*_rs = &rulesets[nrulesets++];
		(*_rs)->r = 0;
		(*_rs)->next = 0;
	}
	rs = *_rs;
	rs = find_or_create_rs_p(rs, o);
	pushp(&rs->r->body, s);
}

void printterm(term* p, dict* d) {
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

void printp(proof* p, dict* d) {
	if (!p)
		return;
	printf("\trule:\t");
	printr(p->rul, d);
	printf("\n\tremaining:\t");
	printl(p->last, d);
	if (p->prev)
		printf("\n\tprev:\t%lu\n\tsubst:\t", (p->prev - proofs)/sizeof(proof));
	else
		printf("\n\tprev:\t(null)\n\tsubst:\t");
	prints(p->s, d);
	printf("\n\tground:\t");
	printg(p->g, d);
	printf("\n");
}

void printrs(ruleset* rs, dict* d) {
	if (!rs)
		return;
	printr(rs->r, d);
	puts("");
	printrs(rs->next, d);
}

void printq(queue* q, dict* d) {
	if (!q)
		return;
	printp(q->p, d);
	if (q->next)
		puts("");
	printq(q->next, d);
}
/*
term* mkterm(int s, int p, int o) {
	uint n = 0;
	for (; n < nterms; ++n) {
		term* pn = &terms[n];
		if (pn->p == p && pn->s && pn->o)
			if (pn->s->p == s && !pn->s->o && !pn->s->s)
				if (pn->o->p == o && !pn->o->o && !pn->o->s)
					return pn;
	}
	term* ps = &terms[nterms++];
	term* pp = &terms[nterms++];
	term* po = &terms[nterms++];
	pp->p = p;
	pp->s = ps;
	pp->o = po;
	ps->s = ps->o = po->s = po->o = 0;
	ps->p = s;
	po->p = o;
	return pp;
}
*/
void prints(subst* s, dict* d) {
	if (!s)
		return;
	const char* _s = dgetw(d, s->p);
	if (_s)
		printf("[%s / ", _s);
	printterm(s->pr, d);
	if (s->next) {
		printf("], ");
		prints(s->next, d);
	}
	else
		printf("] ");
}

void printl(termset* l, dict* d) {
	if (!l)
		return;
	printterm(l->p, d);
	if (!l->next)
		return;
	printf(", ");
	printl(l->next, d);
}

void printr(rule* r, dict* d) {
	if (!r)
		return;
	printl(r->body, d);
	printf(" => ");
	printterm(r->p, d);
}

void printg(ground* g, dict* d) {
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

void printei(evidence_item* ei, dict* d) {
	if (!ei)
		return;
//	printf("evidence_item:\n");
	printterm(ei->p, d);
	printf(": ");
	printg(ei->g, d);
	printf("\n");
	printei(ei->next, d);
}

void printe(evidence* e, dict* d) {
	if (!e)
		return;
/*	if (!d)
		printf("%d:\n", e->p);
	else {
		const char *s = dgetw(d, e->p);
		if (s)
			printf(" %s ", s);
	}
*/
	printei(e->item, d);
	printe(e->next, d);
}

bool is_implication(int p) {
	return p == impl1 || p == impl2 || p == impl3;
}

int pushw(dict** _d, const char* s) {
	int n;
	if (!*_d) {
		*_d = &dicts[ndicts++];
		(*_d)->s = 0;
		(*_d)->n = 0;
		(*_d)->next = 0;
	}
	dict* d = *_d;
	if (dgetwn(d, s, &n))
		return n;
	while (d->next) 
		d = d->next;
	d->next = &dicts[ndicts++];
	d->next->next = 0;
	d->next->s = new char[strlen(s) + 1];
	strcpy(d->next->s, s);
	int c = dict_counter++;
	if (*s == '?'/* || *s == '_'*/)
		c = -c;
	TRACE(printf("Added to dictionary '%s' = %d.\n", s, c));
	return d->next->n = c;
}

const char* dgetw(dict* d, int n) {
	if (!d) {
		TRACE(printf("Note: dictionary request failed for id %d.\n", n));
//		TRACE(printss(gsession));
		return 0;
	}
	if (d->n == n)
		return d->s;
	return dgetw(d->next, n);
}

bool dgetwn(dict* d, const char* s, int* n) {
	if (!d || !s)
		return false;
	if (d->s && !strcmp(d->s, s)) {
		*n = d->n;
		return true;
	}
	return dgetwn(d->next, s, n);
}

void init_prover() {
	initmem(true);
//	queue_test();
//	exit(0);

	session ss;
//	gsession = &ss;
	ss.d = 0;//ss->d;
	ss.kb = 0;
	ss.rkb = 0;
	ss.goal = 0;
	ss.d = 0;
	ss.q = 0;
	pushp(&ss.kb, GND);
	impl1 = pushw(&ss.d, implication1); // define implication
	impl2 = pushw(&ss.d, implication2); 
	impl3 = pushw(&ss.d, implication3);
	GND = &terms[nterms++];
	GND->p = pushw(&ss.d, "GND");
	GND->s = GND->o = 0;

//	menu(&ss);
}
}
