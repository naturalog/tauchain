#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>
void putws(const wchar_t* x) {for (const wchar_t *_putws_t = x; *_putws_t; ++_putws_t) putwchar(*_putws_t); putwchar(L'\n');}
void putcs(char* x) {for (const char *_putws_t = x; *_putws_t; ++_putws_t) putchar(*_putws_t); putchar('\n');}
#define MALLOC(x, y) malloc(sizeof(x) * (y))
#define CALLOC(x, y) calloc(y, sizeof(x))
#define REALLOC(x, y, z) realloc(x, (y) * sizeof(z))
#define swap(t, x, y) { t tt = y; y = x, x = tt; }
#define ROW(x, y) x[y]
#define ROWLEN(x, y) x[0][y]
#define eithervar(x, y) ((x).type == '?' || (y).type == '?')

wchar_t *input; // complete input is read into here first, then free'd after a parser pass
char in_query = 0; // 1 after first "fin."
const size_t buflen = 256, szwc = sizeof(wchar_t);
const wchar_t* getstr(const wchar_t *s); // implemented in dict.cpp, manages unique strings
int getres(); 
void skip() { while (*input && iswspace(*input)) ++input; }
void printts(const int *t);
void printc(int **c);
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

// get* are parsers
int* getlist(int (*f)(), wchar_t till) {
	int t, *args = CALLOC(int, 2), sz;
	while (skip(), *input != till) {
		if (!(t = f())) putws(L"Unexpected item: "), putws(input), exit(1);
		sz = args ? *args : 0;
		args = REALLOC(args, ++sz + 2, int);
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
		int *r = CALLOC(int, 3);
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
	if (r->type != '.') { putws(r->value); return; }
	putws(L"( ");
	const int *a = r->args;
	while (*++a) print(&rs[*a]), putwchar(L' ');
	putwchar(L')');
}
void printt(const struct triple* t) { print(&rs[t->s]), putwchar(L' '), print(&rs[t->p]), putwchar(L' '), print(&rs[t->o]), putwchar(L'.'); }
void printts(const int *t) { if (!t) return; putws(L"{ "); while (*++t) printt(&ts[*t]); putwchar(L'}'); }
void printr(const struct rule* r) { printts(r->p); putws(L" => "); printts(r->c), putws(L""); }
void printc(int **c) {
	static wchar_t s[1024], ss[1024];
	for (int r = *s = 0; r < **c; ++r) {
		*s = 0;
		swprintf(s, 1024, L"row %d:\t", r);
		for (int n = 0; n < ROWLEN(c, r); ++n)
			swprintf(ss, 1024, L"%d\t", ROW(c, r)[n]), wcscat(s, ss);
		putws(s);
	}
}

void parse() {
	int r;
	while ((r = getrule())) printr(&rls[r]), putwchar(L'\n');
}


// Conditions have the form x=y=z=...=t.
// and represented as int** c, two dimensional array
// where both arrays are ordered in ascending order,
// and the outer array is ordered by the first element
// of the inner arrays. 
// c[0] contains lengths only: **c contains the number
// of arrays in c, and (*c)[n] contains the length of
// the n'th equivalence class. the classes therefore
// begin from index 1.

void makeset(int ***c, int x, int y) {
	int *r = MALLOC(int, 3), c00 = ***c;
	if (x > y) r[0] = y, r[1] = x;
	else r[0] = x, r[1] = y;
	r[2] = 0;
	(*c = REALLOC(*c, 1 + c00, int*))[c00] = r, // add row
	***c = ++c00, // update row count
	(**c = REALLOC(**c, c00, int*))[c00 - 1] = 3; // update row size
}

// add x to row i in c
void insert_sorted(int **c, int i, int x) {
	int *p = ROW(c, i) = REALLOC(ROW(c, i), ++ROWLEN(c, i), int); // increase i'th row's space
	while (*p && *p < x) ++p; // walk till row gets geq x
	if (!*p) *p++ = x, *p = 0; // push new largest member
	else { // up till end of row
		int *t = ROW(c, i) + ROWLEN(c, i) - 1; 
		*t-- = 0;
		do { *t = *(t - 1); } while (*--t != *p);
		*t = x;
	}
}

void merge_sorted(int **c, int i, int j) {
	if (i > j) { merge_sorted(c, j, i); return; }
	int li = ROWLEN(c, i), lj = ROWLEN(c, j), l = li + lj - 1;
	int *row = MALLOC(int, l), *r = row;
	const int *x = ROW(c, i), *y = ROW(c, j);
	while (*x && *y) *r++ = (*x < *y ? *x++ : *y++);
       	while (*x) *r++ = *x++;
       	while (*y) *r++ = *y++;
	*r = 0, free(c[i]), c[i] = row,
	free(c[j]), memcpy(&c[j], &c[j + 1], **c - j - 1),
	ROWLEN(c, i) = l, --**c;
}

int bfind(int x, const int *a, int l) {
	int f = 0, m = l / 2;
	while (f <= l)
		if (a[m] < x) f = m + 1, m = (f + l) / 2;
		else if (a[m] == x) return m;
		else l = m - 1, m = (f + l) / 2;
	return -1;
}

void find(int **c, int x, int y, int *i, int *j) {
	if (x > y) { find(c, y, x, j, i); return; }
	*i = *j = 0;
	for (int row = 1, rows = **c; row < rows; ++row) {
		const int *p = ROW(c, row), l = ROWLEN(c, row) - 1;
		int col = bfind(x, p, l);
		if (col != -1) {
			*i = row;
			if (bfind(y, p + col, l - col) != -1)
				*j = row;
		}
		if (!*j) { if (bfind(y, p, l) != -1) { *j = row; if (*i) return; } }
		else if (*i) return;
	}
}

// require() assumes that canmatch() returned true
// hence does not perform checks for the validity
// of the requirement.
char require(int*** _c, int x, int y) {
	assert(x != y && (x > 0 || y > 0)); // at least one var
	int **c = *_c, i, j;
	find(c, x, y, &i, &j);
	if (!i)	{ if (j) insert_sorted(c, j, x); else makeset(_c, x, y); }
	else if (i == j) return 1;
	else if (!j) insert_sorted(c, i, y);
	else if (!(*c[i] < 0 && *c[j] < 0)) merge_sorted(c, i, j);
	else return 0;
	return 1;
}

const int** merge(const int** x, const int** y) {
	return x=y;
}

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

void test() {
	int **c = MALLOC(int*, 3);
	for (int n = 0; n < 3; ++n) c[n] = MALLOC(int, 4);
	c[0][0] = 3, c[0][1] = 4, c[0][2] = 3,
	c[1][0] = 4, c[1][1] = 6, c[1][2] = 7, c[1][3] = 0,
	c[2][0] = 2, c[2][1] = 9, c[2][2] = 0;
	putws(L"before:"), printc(c), insert_sorted(c, 1, 5),
	putws(L"after:"), printc(c), fflush(stdout);
	int i, j;
	find(c, 6, 4, &i, &j), assert(i == 1 && j == 1);
	find(c, 6, 7, &i, &j), assert(i == 1 && j == 1);
	find(c, 6, 0, &i, &j), assert(i == 1 && j == 0);
	find(c, 9, 7, &i, &j), assert(i == 2 && j == 1);
	merge_sorted(c, 1, 2);
	putws(L"merge_sorted(c, 1, 2):"), printc(c), fflush(stdout);
	c[0][0] = 1, printc(c);
	require(&c, 1, 2), putws(L"require 1,2:"), printc(c);
	require(&c, 3, 5), putws(L"require 3,5:"), printc(c);
	require(&c, 4, 2), putws(L"require 4,2:"), printc(c);
	require(&c, -1, 1), putws(L"require -1,1:"), printc(c);
	require(&c, 5, 2), putws(L"require 5,2:"), printc(c);
}

int _main(/*int argc, char** argv*/) {
	setlocale(LC_ALL, "");
	mkres(0, 0), mktriple(0, 0, 0), mkrule(0, 0); // reserve zero indices to signal failure
	test();
	return 0;
	int pos = 0;
	input = MALLOC(wchar_t, buflen);
	while (!feof(stdin)) { // read whole doc into mem
		if ((input[pos++] = getwchar()) == (wchar_t)WEOF) break;
		if (!(pos % buflen))
			input = REALLOC(input, buflen * (1 + pos / buflen), wchar_t);
	}
	input[--pos] = 0;
	wchar_t *_input = input;
	parse();
	free(_input); // input doc can be free'd right after parse
	return 0;
}
