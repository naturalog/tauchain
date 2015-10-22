#include "defs.h"
#include "ir.h"

res *rs = 0;
triple *ts = 0; 
rule *rls = 0; 
int nrs = 0, nts = 0, nrls = 0;
const size_t chunk = 64;

int mkres(const wchar_t* s, char type) {
	if (s && !type && *s == L'?') type = '?';
	if (!(nrs % chunk)) rs = REALLOC(rs, chunk * (nrs / chunk + 1), res);
	return rs[nrs].type = type, rs[nrs].value = s, nrs++;
}
int mktriple(int s, int p, int o) {
	if (!(nts % chunk)) ts = REALLOC(ts, chunk * (nts / chunk + 1), triple);
	return ts[nts].s = s, ts[nts].p = p, ts[nts].o = o, nts++;
}
int mkrule(premise *p, int np, int *c) {
	if (!(nrls % chunk)) rls = REALLOC(rls, chunk * (nrls / chunk + 1), rule);
	return rls[nrls].c = c, rls[nrls].p = p, rls[nrls].np = np, nrls++;
}

void print(const res* r) {
	if (!r) return;
	if (r->type != '.') { wprintf(r->value); return; }
	wprintf(L"( ");
	const int *a = r->args;
	while (*++a) print(&rs[*a]), putwchar(L' ');
	putwchar(L')');
}
void printt(const triple* t) {
	print(&rs[t->s]), putwchar(L' '), print(&rs[t->p]), putwchar(L' '), print(&rs[t->o]), putwchar(L'.');
}
void printts(const int *t) {
	if (!t) return;
	wprintf(L"{ ");
	while (*++t) printt(&ts[*t]); putwchar(L'}');
}
void printps(const premise *p, int np) {
	if (!p) return;
	wprintf(L"{ ");
	for (int n = 0; n < np; ++n) printt(&ts[p[n].p]);
	putwchar(L'}');
}
void printr(const rule* r) {
	if (!r) return;
	if (r->p) printps(r->p, r->np);
	wprintf(L" => ");
	if (r->c) printts(r->c);
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
