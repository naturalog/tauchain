#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "strings.h"
char *input;
char in_query = 0; // 1 after first "fin."
const char* ustr(const char *s); // implemented in dict.cpp, manages unique strings

struct term {
	union {
		const char *value; // resource's value
		const int *args; // list's elements ids
	};
};

struct premise {
	int p; 	// term (triple) id
};
typedef struct premise premise;
typedef struct rule rule;
typedef struct term term;
struct rule {
	int *c; // conclusions, disjuncted. every int is a term (triple) id
	premise *p; // premises, conjuncted
	int np; // number of premises
};

term *terms = 0;
rule *rules = 0; 
int nts = 0, nrs = 0;
const size_t chunk = 64;

int mkterm(const char* s, char type) {
	if (s && !type) {
		if (*s == L'?') type = '?';
		else if (*s == L'_') type = '_';
	}
	if (!(nts % chunk)) terms = realloc(terms, chunk * (nts / chunk + 1) * sizeof(term));
	return terms[nts].value = s, nts++;
}

int mktriple(int s, int p, int o) {
	int t = mkterm(0, 'T'), *a;
	terms[t].args = a = malloc(sizeof(int) * 3), a[0] = p, a[1] = s, a[2] = o;
	return t;
}

int mkrule(premise *p, int np, int *c) {
	if (!(nrs % chunk)) rules = realloc(rules, chunk * (nrs / chunk + 1) * sizeof(rule));
	return rules[nrs].c = c, rules[nrs].p = p, rules[nrs].np = np, nrs++;
}

void skip() { while (*input && isspace(*input)) ++input; }
const char* take(int p) {
	char s[p + 1];
	strncpy(s, input, p), s[p] = 0, input += p;
	return ustr(s);	
}
char peek(const char *_s) {
	const char *s = _s;
	int n = 0;
	while (*s) if (!input[n] || input[n++] != *s++) return 0;
	return 1;
}
void expect(const char *s) {
	if (skip(), !peek(s)) printf("Error: \"%s\" expected.", s), exit(1);
	input += strlen(s);
}

int* getlist(int (*f)(), char till) {
	int t, *args = calloc(2, sizeof(int)), sz;
	while (skip(), *input != till) {
		if (!(t = f())) puts("Unexpected item: "), fputs(input, stdout), exit(1);
		sz = args ? *args : 0;
		args = realloc(args, (++sz + 2) * sizeof(int));
		args[*args = sz] = t;
		args[sz + 1] = 0;
	}
	return ++input, args;
}
int getres() {
	skip();
	int p = strcspn(input, " \r\n\t(){}.");
	switch (input[p]) {
	case 0:  return 0;
	case '{':fputs("Unexpected {:", stdout), fputs(input, stdout); exit(1);
	case '(':return ++input, mkterm(getlist(getres, ')'), '.');
	default: return mkterm(take(p), 0);
	}
}
int gettriple() {
	int s, p, o;
	if (!((s = getres()) && (p = getres()) && (o = getres()))) return 0;
	if (skip(), *input == '.') ++input, skip();
	int t = mktriple(s, p, o);
	return t;
}
premise* to_prems(int* p) {
	premise *r = malloc(sizeof(premise) * *p);
	for (int n = 1; n <= *p; ++n) r[n - 1].p = p[n];
	assert(p[1 + *p] == 0);
	free(p);
	return r;
}
int getrule() {
	int r;
	premise *p;
	if (skip(), peek("fin.")) {
		if (!in_query) return in_query = 1, input += 4, getrule();
		return 0;
	}
	if (skip(), !*input) return 0;
	if (*input != '{') {
		int r;
		if (!(r = gettriple())) return 0;
		if (in_query) {
			(p = malloc(sizeof(premise)))->p = r;		
			return mkrule(p, 1, 0);
		}
		int *rr = calloc(3, sizeof(int));
		rr[rr[rr[2] = 0] = 1] = r;
		return mkrule(0, 0, rr);
	}
	++input;
	int *pp = getlist(gettriple, '}');
	expect("=>"), skip(), expect("{");
	int *cc = getlist(gettriple, '}');
	r = mkrule(to_prems(pp), *pp, cc);
	if (*input == '.') ++input;
	return r;
}
void parse() {
	int r;
	while ((r = getrule()));// printr(&rls[r]), putwchar('\n');
}
