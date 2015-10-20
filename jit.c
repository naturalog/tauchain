#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
#define pws(x)	for (const wchar_t *_pws_t = x; *_pws_t; ++_pws_t) putwchar(*_pws_t);
#define putws(x) fputws(x, stdin)

wchar_t *input; // complete input is read into here first, then free'd after a parser pass
char in_query = 0; // 1 after first "fin."
const size_t buflen = 256, szwc = sizeof(wchar_t);
const wchar_t* getstr(const wchar_t *s); // implemented in dict.cpp, manages unique strings
int getres(); 
void skip() { while (*input && iswspace(*input)) ++input; }
void printts(const int *t);
const wchar_t* take(int p) {
	wchar_t s[p + 1];
	wcsncpy(s, input, p), s[p] = 0, input += p;
	return getstr(s);	
}
char peek(const wchar_t *_s) {
	const wchar_t *s = _s;
	int n = 0;
	while (*s) if (!input[n] || input[n++] != *s++) return 0;
	return 1;
}
void expect(const wchar_t *s) {
	if (skip(), !peek(s)) wprintf(L"Error: \"%s\" expected.", s), exit(1);
	input += wcslen(s);
}

// Following 3 structs store the kb&query.
// res::type is '.' for list, '?' for var, and 0 otherwise.
// All three int* below (args, c, p) have first int as length,
// and last int always null. Hence empty list consists of two
// integers, zero each.
struct res { char type; union { const wchar_t *value; const int *args; }; } *rs = 0;
struct triple { int s, p, o; } *ts = 0; 
struct rule { int *p, *c; } *rls = 0; // premises and conclusions
size_t nrs = 0, nts = 0, nrls = 0;
const size_t chunk = 64;

int mkres(const wchar_t* s, char type) {
	if (s && !type && *s == L'?') type = '?';
	if (!(nrs % chunk)) rs = realloc(rs, chunk * sizeof(struct res) * (nrs / chunk + 1));
	return rs[nrs].type = type, rs[nrs].value = s, nrs++;
}
int mktriple(int s, int p, int o) {
	if (!(nts % chunk)) ts = realloc(ts, chunk * sizeof(struct triple) * (nts / chunk + 1));
	return ts[nts].s = s, ts[nts].p = p, ts[nts].o = o, nts++;
}
int mkrule(int *p, int *c) {
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
	int t = mktriple(s, p, o);
	return t;
}
int getrule() {
	static int *p, r;
	if (skip(), peek(L"fin.")) {
		if (!in_query) return in_query = 1, input += 4, getrule();
		return 0;
	}
	if (skip(), !*input) return 0;
	if (*input != L'{') {
		int *r = calloc(3, sizeof(int));
		if (!(r[r[r[2] = 0] = 1] = gettriple())) return 0;
		return in_query ? mkrule(r, 0) : mkrule(0, r);
	}
	++input;
	p = getlist(gettriple, L'}'), expect(L"=>"), skip(), expect(L"{"), r = mkrule(p, getlist(gettriple, L'}'));
	if (*input == L'.') ++input;
	return r;
}

void print(const struct res* r) {
	if (!r) return;
	if (r->type != '.') { pws(r->value); return; }
	pws(L"( ");
	const int *a = r->args;
	while (*++a) print(&rs[*a]), putwchar(L' ');
	putwchar(L')');
}
void printt(const struct triple* t) { print(&rs[t->s]), putwchar(L' '), print(&rs[t->p]), putwchar(L' '), print(&rs[t->o]), putwchar(L'.'); }
void printts(const int *t) { if (!t) return; pws(L"{ "); while (*++t) printt(&ts[*t]); putwchar(L'}'); }
void printr(const struct rule* r) { printts(r->p); pws(L" => "); printts(r->c), puts(""); }

void parse() {
	int r;
	while ((r = getrule())) printr(&rls[r]), putwchar(L'\n');
}
/*
// using the following coros:
// 'it' will return the iterated value.
// state has to be initialized with zero and is for
// the coro's internal use.
// the coro return 0 after no more values to be read,
// hence 'it' on this case should be ignored,
// or 1 otherwise.
// when coros are finished they set the state back to zero.

// iterate resource, either returns itself or list members
char res_coro(int r, int *it, int* state) {
	switch (*state) {
	case 0: return ++*state, *it = r;
	case 1: if (rs[r].type != '.') return 0;
	default:return rs[r].args[*state] ? *it = rs[r].args[*state++], 1 : (*state = 0);
	}
}
// iterate resource with type too, without returning the 
// resource itself if not a list
char tres_coro(struct res* r, int *it, int *state) {
	switch (*state) {
	case 0: return *it = r->type, ++*state, 1;
	case 1: if (r->type != '.') return 0;
	default:return r->args[*state] ? *it = r->args[*state++], 1 : (*state = 0);
	}
}
*/
#define eithervar(x, y) ((x).type == '?' || (y).type == '?')

// Conditions have the form x=y=z=...=t.
// and represented as int** c, two dimensional array
// where both arrays are ordered in ascending order,
// and the outer array is ordered by the first element
// of the inner arrays. 
// c[0] contains lengths only: **c contains the number
// of arrays in c, and (*c)[n] contains the length of
// the n'th equivalence class. the classes therefore
// begin from index 1.

// highest element that is still less or equal to x
#define FWD(a, x) while (*(a) && *(a) < (x)) ++(a);

int** makeset(int **c, int x, int y) {
	int *r = malloc(sizeof(int) * 3);
	if (x > y) r[0] = y, r[1] = x;
	else r[0] = x, r[1] = y;
	r[2] = 0;
	c = realloc(c, ++**c * sizeof(int*)), c[**c] = r;
	*c = realloc(*c, **c * sizeof(int*)), (*c)[**c - 1] = 3;
	return c;
}

int** insert_sorted(int **c, int **rx, int y) { }
int** merge_sorted(int **c, int **rx, int **ry) { }

int** require(int** c, int x, int y) {
	assert(x != y);
	int **_c = c, **rx = 0, **ry = 0, *p, t = x;
	if (x > y) { t = y, y = x, x = t; }
	while ((p = *c)) {
		while (*p && *p < x) ++p;
		if (*p == x) rx = c;
		while (*p && *p < y) ++p;
		if (*p == y) ry = c;
		if (**++c > t) {
			if (t == x) t = y;
			else break;
		}
	}
	if (!rx && !ry) return makeset(x, y);
	c = _c;
	if (rx == ry) return c;
	if (rx && ry && **rx < 0 && **ry < 0) return 0;
	if (!rx) return	insert_sorted(c, ry, x);
	if (!ry) return	insert_sorted(c, rx, y);
	if (**rx > **ry) { int **rr = ry; ry = rx, rx = rr; }
	return merge_sorted(c, rx, ry);
}
int** merge(const int** x, const int** y) { }

char canmatch(int x, int y) {
	struct res s = rs[x], d = rs[y];
	if (!eithervar(s, d)) {
		if (s.type != d.type) return 0;
		if (s.type != '.') return x == y ? 1 : 0;
		if (*s.args++ != *d.args++) return 0;
		while (*s.args) if (!canmatch(*s.args++, *d.args++)) return 0;
		return 1;
	}
	
//	return setcond(x, y);
	return 1;
}

int _main(int argc, char** argv) {
	setlocale(LC_ALL, "");
	mkres(0, 0), mktriple(0, 0, 0), mkrule(0, 0); // reserve zero indices to signal failure
	int pos = 0;
	input = malloc(szwc * buflen);
	while (!feof(stdin)) { // read whole doc into mem
		if ((input[pos++] = getwchar()) == WEOF) break;
		if (!(pos % buflen))
			input = realloc(input, buflen * szwc * (1 + pos / buflen));
	}
	input[--pos] = 0;
	wchar_t *_input = input;
	parse();
	free(_input); // input doc can be free'd right after parse
	return 0;
}
