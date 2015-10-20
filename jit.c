#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>

#define gw()	getwchar()
#define pw(x)	putwchar(x)
#define pws(x)	for (const wchar_t *_pws_t = x; *_pws_t; ++_pws_t) pw(*_pws_t);
#define putws(x) fputws(x, stdin)

wchar_t *input;
char in_query = 0; // 1 after first "fin."
const size_t buflen = 256, szwc = sizeof(wchar_t);
const wchar_t* getstr(const wchar_t *s); // implemented in dict.cpp, manages unique strings
int getres(); 
void skip() { while (iswspace(*input)) ++input; }
const wchar_t* take(int p) {
	wchar_t s[p + 1];
	wcsncpy(s, input, p), s[p] = 0, input += p;
	return getstr(s);	
}
char peek(const wchar_t *_s) {
	const wchar_t *s = _s;
	int n = 0;
	while (*s) if (input[n++] != *s++) return 0;
	return 1;
}
void expect(const wchar_t *s) {
	if (!peek(s)) wprintf(L"Error: \"%s\" expected.", s), exit(1);
}

/*
 * Following 3 structs store the kb&query.
 * res::type is '.' for list, '?' for var, and 0 otherwise.
 * All three int* below (args, c, p) have first int as length,
 * and last int always null. Hence empty list consists of two
 * integers, zero each.
 */
struct res { char type; union { const wchar_t *value; const int *args; }; } *rs = 0;
struct triple { const struct res *s, *p, *o; } *ts = 0; 
struct rule { int *c, *p; } *rls = 0;
size_t nrs = 0, nts = 0, nrls = 0;
const size_t chunk = 64;

int mkres(const wchar_t* s, char type) {
	if (!(nrs % chunk)) rs = realloc(rs, chunk * sizeof(struct res) * (nrs / chunk + 1));
	return rs[nrs].type = type, rs[nrs].value = s, nrs++;
}
int mktriple(int s, int p, int o) {
	if (!(nts % chunk)) ts = realloc(ts, chunk * sizeof(struct triple) * (nts / chunk + 1));
	return ts[nts].s = &rs[s], ts[nts].p = &rs[p], ts[nts].o = &rs[o], nts++;
}
int mkrule(int *c, int *p) {
	if (!(nrls % chunk)) rls = realloc(rls, chunk * sizeof(struct rule) * (nrls / chunk + 1));
	return rls[nrls].c = c, rls[nrls].p = p, nrls++;
}

// get* are parsers
int* getlist(int (*f)(), wchar_t till) {
	int t, *args = calloc(2, sizeof(int)), sz;
	while (skip(), *input != till) {
		if (!(t = f())) putws(L"Unexpected item: "), putws(input), exit(1);
		sz = args ? *args : 0;
		args = realloc(args, (++sz + 2) * sizeof(int));
		args[*args = sz] = t;
		args[sz + 1] = 0;
	}
	return ++input, args;
}
int getres() {
	skip();
	int p = wcscspn(input, L" \r\n\t(){}.");
	switch (input[p]) {
	case 0: return 0;
	case L'{': putws(L"Unexpected {:"), putws(input); exit(1);
	case L'(': return ++input, mkres(getlist(getres, L')'), '.');
	default: return mkres(take(p), 0);
	}
}
int gettriple() {
	int s, p, o;
	if (!((s = getres()) && (p = getres()) && (o = getres()))) return 0;
	if (skip(), *input == L'.') ++input, skip();
	return mktriple(s, p, o);
}
int getrule() {
	static int *p;
	if (peek(L"fin.")) {
		if (in_query) return 0;
		in_query = 1;
	}
	if (skip(), *input != L'{') {
		int *r = calloc(3, sizeof(int));
		r[r[r[2] = 0] = 1] = gettriple();
		return in_query ? mkrule(0, r) : mkrule(r, 0);
	}
	return p = getlist(gettriple, L'}'), expect(L"=>"), mkrule(p, getlist(gettriple, L'}'));
}

void print(const struct res* r) {
	if (r->type != '.') { pws(r->value); return; }
	pws(L"( ");
	const int *a = r->args;
	while (*++a) print(&rs[*a]), pw(L' ');
	pw(L')');
}
void printt(const struct triple* t) { print(t->s), pw(L' '), print(t->p), pw(L' '), print(t->o), pw(L'.'); }
void printts(const int *t) {
	if (!t) return;
	pws(L"{ ");
	while (*++t) printt(&ts[*t]);
	pw(L'}');
}
void printr(const struct rule* r) { printts(r->p); pws(L" => "); printts(r->c), puts(""); }

void parse() {
	int r;
	while ((r = getrule()))
		printr(&rls[r]), puts("");
}

int _main(int argc, char** argv) {
	setlocale(LC_ALL, "");
	mkres(0, 0), mktriple(0, 0, 0), mkrule(0, 0);
	int pos = 0;
	input = malloc(szwc * buflen);
	while (!feof(stdin)) { // read whole doc into mem
		input[pos++] = gw();
		if (!(pos % buflen))
			input = realloc(input, szwc * (1 + pos / buflen));
	}
	input[pos] = 0;
	wchar_t *_input = input;
	parse();
	free(_input);
	return 0;
}
