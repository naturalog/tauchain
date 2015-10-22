#include "defs.h"
#include "ir.h"

term *terms = 0;
rule *rules = 0; 
int nts = 0, nrs = 0;
const size_t chunk = 64;

int mkterm(const wchar_t* s, char type) {
	if (s && !type) {
		if (*s == L'?') type = '?';
		else if (*s == L'_') type = '_';
	}
	if (!(nts % chunk)) terms = REALLOC(terms, chunk * (nts / chunk + 1), term);
	return terms[nts].type = type, terms[nts].value = s, nts++;
}
int mktriple(int s, int p, int o) {
	int t = mkterm(0, 'T'), *a;
	terms[t].args = a = MALLOC(int, 3), a[0] = p, a[1] = s, a[2] = o;
	return t;
}
int mkrule(premise *p, int np, int *c) {
	if (!(nrs % chunk)) rules = REALLOC(rules, chunk * (nrs / chunk + 1), rule);
	return rules[nrs].c = c, rules[nrs].p = p, rules[nrs].np = np, nrs++;
}
premise* mkpremise(int r) {
	premise *p = MALLOC(premise, 1);
	p->p = 0, p->e = MALLOC(int***, nrs), p->e[r] = MALLOC(int**, *rules[r].c);
	return p;
}

void _print(int t) { print(&terms[t]); }
void print(const term* r) {
	if (!r) return;
	if (r->type == 'T') {
		_print(r->args[1]), putwchar(L' '), _print(r->args[0]),
		putwchar(L' '), _print(r->args[2]), putwchar(L'.');
		return;
	}
	if (r->type != '.') { wprintf(r->value); return; }
	wprintf(L"( ");
	const int *a = r->args;
	while (*++a) print(&terms[*a]), putwchar(L' ');
	putwchar(L')');
}
void printps(const premise *p, int np) {
	if (!p) return;
	wprintf(L"{ ");
	for (int n = 0; n < np; ++n) print(&terms[p[n].p]);
	putwchar(L'}');
}
void printr(const rule* r) {
	if (!r) return;
	printps(r->p, r->np), wprintf(L" => {"); 
	if (r->c) for (int* t = r->c; *t; ++t) _print(*t);
	putwchar(L'}');
}
void printa(int *a, int l) {
	for (int n = 0; n < l; ++n) wprintf(L"%d ", a[n]);
}
void printc(int **c) {
	if (!c) return;
	static wchar_t s[1024], ss[1024];
	for (int r = *s = 0; r < **c; ++r) {
		*s = 0;
		swprintf(s, 1024, L"row %d:\t", r);
		for (int n = 0; n < (r ? ROWLEN(c, r) : **c + 1); ++n)
			swprintf(ss, 1024, L"%d\t", ROW(c, r)[n]), wcscat(s, ss);
		putws(s);
	}
}
void putws(const wchar_t* x) {
	for (const wchar_t *_putws_t = x; *_putws_t; ++_putws_t)
		putwchar(*_putws_t);
	putwchar(L'\n');
}
void putcs(char* x) {
	for (const char *_putws_t = x; *_putws_t; ++_putws_t)
		putchar(*_putws_t);
	putchar('\n');
}
