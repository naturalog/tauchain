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

int** makeset(int **c, int x, int y) {
	int *r = MALLOC(int, 3), c00 = **c;
	if (x > y) r[0] = y, r[1] = x;
	else r[0] = x, r[1] = y;
	r[2] = 0;
	(c = REALLOC(c, 1 + c00, int*))[c00] = r, // add row
	**c = ++c00, // update row count
	(*c = REALLOC(*c, c00, int*))[c00 - 1] = 3; // update row size
	return c;
}

// add x to row i in c
int** insert_sorted(int **c, int i, int x) {
	int *p = ROW(c, i) = REALLOC(ROW(c, i), ++ROWLEN(c, i), int); // increase i'th row's space
	while (*p && *p < x) ++p; // walk till row gets geq x
	if (!*p) return *p++ = x, *p = 0, c; // push new largest member
	int *t = ROW(c, i) + ROWLEN(c, i) + 1; // up till end of row
	*t-- = 0;
	do { *t = *(t - 1); } while (*--t != *p);
	*t = x;
}

void printc(int **c) {
	int rows = **c;
	wchar_t s[1024], ss[1024];
	*s = *ss = 0;
	for (int r = 0; r < rows; ++r) {
		swprintf(s, 1024, L"row %d:\t", r);
		for (int n = 0; n < ROWLEN(c, r); ++n)
			swprintf(ss, 1024, L"%d\t", ROW(c, r)[n]), wcscat(s, ss);
		putws(s);
		*s = 0;
	}
}

void test_insert_sorted() {
	int **c = MALLOC(int*, 3);
	for (int n = 0; n < 3; ++n) c[n] = MALLOC(int, 4);
	c[0][0] = 3, c[0][1] = 4, c[0][2] = 3,
	c[1][0] = 4, c[1][1] = 6, c[1][2] = 7, c[1][3] = 0,
	c[2][0] = 8, c[2][1] = 9, c[2][2] = 0;
	putws(L"before:\n");
	printc(c);
	insert_sorted(c, 1, 5);
	putws(L"after:\n");
	printc(c);
	fflush(stdout);
}

int** merge_sorted(int **c, int i, int j) { }

void find(int **c, int x, int y, int *i, int *j) {
	if (x > y) { find(**c, y, x, j, i); return; }
	int **_c = c, **rx = c, **ry = c, t = x;
	for (int *p = *c; p = *c;) {
		while (*p && *p < x) ++p; if (*p == x) rx = c;
		while (*p && *p < y) ++p; if (*p == y) ry = c;
		if (**++c > t) { if (t == x) t = y; else break; }
	}
	*i = rx - c, *j = ry - c;
}

int** require(int** c, int x, int y) {
	assert(x != y);
	assert(x > 0 || y > 0); // at least one var
	int i, j;
	find(c, x, y, &i, &j);
	if (!i)		return j ? insert_sorted(c, j, x) : makeset(c, x, y);
	if (!j)		return insert_sorted(c, i, y);
	if (i == j)	return c;
	return (*c[i] < 0 && *c[j] < 0) ? 0 : merge_sorted(c, i, j);
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
	test_insert_sorted();
	int pos = 0;
	input = MALLOC(wchar_t, buflen);
	while (!feof(stdin)) { // read whole doc into mem
		if ((input[pos++] = getwchar()) == WEOF) break;
		if (!(pos % buflen))
			input = REALLOC(input, buflen * (1 + pos / buflen), wchar_t);
	}
	input[--pos] = 0;
	wchar_t *_input = input;
	parse();
	free(_input); // input doc can be free'd right after parse
	return 0;
}
