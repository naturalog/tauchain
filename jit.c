#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <assert.h>

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

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
// begin from index 1. the last element of c[0] contains
// the sum of all sizes of all rows (except row 0)
#ifdef DEBUG
// various very expensive assertions, for debug/unittest only
void check(int **c) {
	int s = 0;
	// recalc total size
	for (int n = 0; n < **c; ++n) assert(c[n][0] && c[0][n]), s += c[0][n];
	assert(s - 2 * **c == c[0][**c] - 1); // assert total size
	for (int n = 1; n < **c; ++n) {
		char bound = c[n][0] < 0 ? 1 : 0;
		assert(c[n][c[0][n] - 1] == 0); // assert zero termination matching the sizes vector
		for (int k = 0; k < c[0][n] - 1; ++k) {
			if (!bound || k) assert(c[n][k] > 0); // assert only one nonvar binding
			for (int n1 = 1; n < **c; ++n)
				for (int k1 = 0; k < c[0][n] - 1; ++k)
					if (n != n1 && k != k1)
						assert(c[n][k] != c[n1][k1]); // assert global uniqueness
		}
	}
}
#else
#define check(x)
#endif

// simple binary search over integer array of length l
int bfind(int x, const int *a, int l) {
	int f = 0, m = --l / 2;
	while (f <= l)
		if (a[m] < x) f = m + 1, m = (f + l) / 2;
		else if (a[m] == x) return m;
		else l = m - 1, m = (f + l) / 2;
	return -1;
}

int find1(int **c, int x) {
	check(c);
	for (int row = 1, col, rows = **c; row < rows; ++row) {
		const int *p = ROW(c, row), l = ROWLEN(c, row) - 1;
		if ((col = bfind(x, p, l)) != -1) return row;
	}
	return 0;
}

// find rows for two given labels/vars. it's
// slightly more efficient than searching each separately
void find(int **c, int x, int y, int *i, int *j) {
	if (x > y) { find(c, y, x, j, i); return; }
	*i = *j = 0;
	for (int row = 1, col, rows = **c; row < rows; ++row) {
		const int *p = ROW(c, row), l = ROWLEN(c, row) - 1;
		if ((col = bfind(x, p, l)) != -1) {
			*i = row;
			if (bfind(y, p + col, l - col) != -1) *j = row;
		}
		if (!*j) { if (bfind(y, p, l) != -1) { *j = row; if (*i) return; } }
		else if (*i) return;
	}
	check(c);
}

void makeset(int ***_c, int x, int y) {
	int *r = MALLOC(int, 3), rows = ***_c;
	if (x > y) r[0] = y, r[1] = x; else r[0] = x, r[1] = y;
	r[2] = 0;
	int **c = *_c = REALLOC(*_c, 1 + rows, int*), t = c[0][rows];
	c[rows] = r, **c = ++rows, // add row and update rows count
	(*c = REALLOC(*c, 2 + rows, int*))[rows - 1] = 3; // update row size
	c[0][rows] = t + 2; // update total count
#ifdef DEBUG	
	int i, j; check(c), find(*_c, x, y, &i, &j), assert(i == j);
#endif	
}

// add x to row i in c
void insert_sorted(int **c, int i, int x) {
	int *p = c[i] = REALLOC(c[i], ++c[0][i], int); // increase i'th row's space
	++c[0][**c];
	while (*p && *p < x) ++p; // walk till row gets geq x
	if (!*p) *p++ = x, *p = 0; // push new largest member
	else { // up till end of row
		int *t = c[i] + c[0][i] - 1; 
		*t-- = 0;
		do { *t = *(t - 1); } while (*--t != *p);
		*t = x;
	}
#ifdef DEBUG	
	check(c), assert(i == find1(c, x));
#endif	
}

// add a row of length l, return row num
int insert(int ***_d, int *r, int l) {
	int dsz = ***_d, n = 1, **d = *_d = REALLOC(*_d, 1 + dsz, int*), s = _d[0][0][dsz];
	d[0] = REALLOC(d[0], 2 + dsz, int);
	if (dsz == 1) {
		d[0][0] = 2, d[0][2] = (d[0][1] = l) - 1, d[1] = r;
		check(d);
		return 1;
	}
	while (n < **d && *d[n] < *r) ++n;
	if (n != dsz) {
		memmove(&d[n + 1], &d[n], sizeof(int*) * (dsz - n));
		memmove(&d[0][n + 1], &d[0][n], sizeof(int) * (dsz - n + 1));
	}
	d[n] = r, d[0][n] = l, (d[0][1 + dsz] = s + l - 1), ++**d;
	check(*_d);
	return n;
}

// set union
int* merge_rows(int *x, int *y, int lx, int ly, int *e) {
	int l = lx + ly - 1;
	int *row = MALLOC(int, l), *r = row;
	*e = 0;
	while (*x && *y)
		if (*x == *y) (*r++ = *x++), *y++, ++*e;
		else *r++ = (*x < *y ? *x++ : *y++);
       	while (*x) *r++ = *x++;
       	while (*y) *r++ = *y++;
	return *r = 0, row;
}

void merge_sorted(int **c, int ***d, int i, int j);
void merge_into(int *x, int *y, int lx, int ly, int ***d) {
	int e, row, n;
	int *r = merge_rows(x, y, lx, ly, &e);
	int s = lx + ly - 1 - e;
	check(*d);
	for (n = 0; n < s; ++n)
		if ((row = find1(*d, r[n]))) {
			merge_sorted(*d, 0, row, insert(d, r, s));
			return;
		}
	insert(d, r, s);
	check(*d);
}

// merge rows i,j in c.
// if d!=0, merge into a new row in d.
void merge_sorted(int **c, int ***d, int i, int j) {
	if (i > j) { merge_sorted(c, d, j, i); return; }
	int li = c[0][i], lj = c[0][j], l = li + lj - 1, e;
	if (d) {
		merge_into(c[i], c[j], li, lj, d);
		check(*d);
		return;
	}
	int *row = merge_rows(c[i], c[j], li, lj, &e), csz = **c;
	c[0][**c] -= e, // update total count
	free(c[i]), c[i] = row, free(c[j]), // put new row and free old ones
	memmove(&c[j], &c[j + 1], (csz - j - 1) * sizeof(int*)), // shift rows down
	memmove(&c[0][j], &c[0][j + 1], (csz - j) * sizeof(int)); // shift sizes down
	c[0][i] = l, --**c; // update row and rows size
	check(c);
}

// require() assumes that canmatch() returned true
// hence does not perform checks for the validity
// of the requirement.
char require(int*** _c, int x, int y) {
	assert(x != y && (x > 0 || y > 0)); // at least one var
	int **c = *_c, i, j;
	check(c);
	find(c, x, y, &i, &j);
	if (!i)	{ if (j) insert_sorted(c, j, x); else makeset(_c, x, y); }
	else if (i == j) return 1; // x,y appear at the same row
	else if (!j) insert_sorted(c, i, y); // x appears and y doesnt, add y to x's row
	else if (!(*c[i] < 0 && *c[j] < 0)) // both exist, at least one var
		merge_sorted(c, 0, i, j); // merge their rows
	else return 0; // can't match two different nonvars
	return 1;
}

// copy while omitting one row
int **cprm(int **c, int r) {
	int csz = **c, **a = MALLOC(int*, csz - 1);
	assert(csz), assert(csz - 1);
	*(*a = MALLOC(int, csz)) = csz - 1;
	memmove(&a[1],		&c[1], 		sizeof(int*)* (r - 1));
	memmove(&a[0][1],	&c[0][1], 	sizeof(int) * (r - 1));
	memmove(&a[r],		&c[r + 1], 	sizeof(int*)* (csz - r - 1));
	memmove(&a[0][r],	&c[0][r + 1], 	sizeof(int) * (csz - r));
	a[0][**a] -= c[0][r] - 1;
	check(a);
	return a;
}

void merge(int** x, int** y, int ***r) {
	check(x); check(y);
	if (x[0][**x] > y[0][**y]) { merge(y, x, r); return; }
	for (int row = 1, rows = **x; row < rows; ++row)
		for (int col = 0, cols = ROWLEN(x, row) - 1; col < cols; ++col)
			for (int cc = 1, e = **y; cc < e; ++cc)
				if (bfind(x[row][col], y[cc], y[0][cc]) != -1) {
					putws(L"found!");
					merge_into(y[cc], x[row], y[0][cc], x[0][row], r);
					check(*r);
					printc(*r);
					check(x);
					check(y);
					check(*r);
					merge(cprm(x, row), cprm(y, cc), r);
				}
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
	c[0][0] = 3, c[0][1] = 4, c[0][2] = 3, c[0][3] = 5,
	c[1][0] = 4, c[1][1] = 6, c[1][2] = 7, c[1][3] = 0,
	c[2][0] = 2, c[2][1] = 9, c[2][2] = 0;
	check(c);
	putws(L"before:"), printc(c), insert_sorted(c, 1, 5),
	putws(L"after:"), printc(c), fflush(stdout);
	check(c);
	int i, j;
	find(c, 6, 4, &i, &j), assert(i == 1 && j == 1);
	find(c, 6, 7, &i, &j), assert(i == 1 && j == 1);
	find(c, 6, 0, &i, &j), assert(i == 1 && j == 0);
	find(c, 9, 7, &i, &j), assert(i == 2 && j == 1);
	check(c);
	merge_sorted(c, 0, 1, 2);
	check(c);
	putws(L"merge_sorted(c, 1, 2):"), printc(c), fflush(stdout);
	int **d = MALLOC(int*, 1);
	*d = MALLOC(int, 2);
	d[0][0] = 1, d[0][1] = 0, printc(d);
	require(&d, 1, 2), putws(L"require 1,2:"), printc(d);
	require(&d, 3, 5), putws(L"require 3,5:"), printc(d);
	require(&d, 4, 2), putws(L"require 4,2:"), printc(d);
	require(&d, -1, 1), putws(L"require -1,1:"), printc(d);
	require(&d, 5, 2), putws(L"require 5,2:"), printc(d);
	require(&d, -2, 5), putws(L"require -2,5:"), printc(d);
	putws(L"merging:"), printc(c), putws(L""),
	putws(L"with:"), printc(d), putws(L"");
	int **r = MALLOC(int*, 1);
	*r = MALLOC(int, 2);
	r[0][0] = 1, r[0][1] = 0;
	check(r);
	printc(r);
	merge(c, d, &r);
	putws(L"result:"), printc(r), putws(L""), fflush(stdout);
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
