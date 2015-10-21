#include "defs.h"
#include "ir.h"

struct res *rs = 0;
struct triple *ts = 0; 
struct rule *rls = 0; 
size_t nrs = 0, nts = 0, nrls = 0;
const size_t chunk = 64;

int mkres(const wchar_t* s, char type) {
	if (s && !type && *s == L'?') type = '?';
	if (!(nrs % chunk)) rs = REALLOC(rs, chunk * (nrs / chunk + 1), struct res);
	return rs[nrs].type = type, rs[nrs].value = s, nrs++;
}
int mktriple(int s, int p, int o) {
	if (!(nts % chunk)) ts = REALLOC(ts, chunk * (nts / chunk + 1), struct triple);
	return ts[nts].s = s, ts[nts].p = p, ts[nts].o = o, nts++;
}
int mkrule(int *p, int *c) {
	if (!(nrls % chunk)) rls = REALLOC(rls, chunk * (nrls / chunk + 1), struct rule);
	return rls[nrls].c = c, rls[nrls].p = p, nrls++;
}

void print(const struct res* r) {
	if (!r) return;
	if (r->type != '.') { putws(r->value); return; }
	putws(L"( ");
	const int *a = r->args;
	while (*++a) print(&rs[*a]), putwchar(L' ');
	putwchar(L')');
}
void printt(const struct triple* t) { print(&rs[t->s]), putwchar(L' '), print(&rs[t->p]), putwchar(L' '), print(&rs[t->o]), putwchar(L'.'); }
void printts(const int *t) { if (!t) return; putws(L"{ "); while (*++t) printt(&ts[*t]); putwchar(L'}'); }
void printr(const struct rule* r) { printts(r->p); putws(L" => "); printts(r->c), putws(L""); }
void printa(int *a, int l) { for (int n = 0; n < l; ++n) wprintf(L"%d ", a[n]); }
void printc(int **c) {
	static wchar_t s[1024], ss[1024];
	for (int r = *s = 0; r < **c; ++r) {
		*s = 0;
		swprintf(s, 1024, L"row %d:\t", r);
		for (int n = 0; n < (r ? ROWLEN(c, r) : **c + 1); ++n)
			swprintf(ss, 1024, L"%d\t", ROW(c, r)[n]), wcscat(s, ss);
		putws(s);
	}
}
void putws(const wchar_t* x) {for (const wchar_t *_putws_t = x; *_putws_t; ++_putws_t) putwchar(*_putws_t); putwchar(L'\n');}
void putcs(char* x) {for (const char *_putws_t = x; *_putws_t; ++_putws_t) putchar(*_putws_t); putchar('\n');}
