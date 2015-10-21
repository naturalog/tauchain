#include "defs.h"
#include "ir.h"
#include "n3.h"
#include "eq.h"

#ifdef DEBUG
void check(int **c) {
	int s = 0;
	printc(c); fflush(stdout);
	// recalc total size
	for (int n = 0; n < **c; ++n) assert(c[n][0] && c[0][n]), s += c[0][n];
	assert(s - 2 * **c == c[0][**c] - 1); // assert total size
	for (int n = 1; n < **c; ++n) {
		char bound = c[n][0] < 0 ? 1 : 0; // assert max one nonvar
		assert(c[n][c[0][n] - 1] == 0); // assert zero termination matching the sizes vector
		for (int k = 0; k < c[0][n] - 1; ++k) {
			assert(c[n][k]); // verify nonzero
			if (!bound || k) {
				if (!c[n][k])
					wprintf(L"n=%d,k=%d,bound=%d,c[n][k]=%d\n", n, k, (int)bound, c[n][k]),
					fflush(stdout);
				assert(c[n][k] > 0); // assert only one nonvar binding
			}
			for (int n1 = 1; n < **c; ++n)
				for (int k1 = 0; k < c[0][n] - 1; ++k)
					if (n != n1 && k != k1)
						assert(c[n][k] != c[n1][k1]); // assert global uniqueness
		}
	}
}
#endif

int bfind(int x, const int *a, int l) {
	int f = 0, m = --l / 2;
	while (f <= l)
		if (a[m] < x) f = m + 1, m = (f + l) / 2;
		else if (a[m] == x) return m;
		else l = m - 1, m = (f + l) / 2;
	return -1;
}

int find1(int **c, int x) {
	assert(x);
	for (int row = 1, col, rows = **c; row < rows; ++row) {
		const int *p = ROW(c, row), l = ROWLEN(c, row) - 1;
		if ((col = bfind(x, p, l)) != -1) return row;
	}
	return 0;
}

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

void put_in_row(int **c, int i, int x) {
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

void put_row_to(int ***_d, int *r, int l, int row) {
	int dsz = ***_d, **d = *_d = REALLOC(*_d, 1 + dsz, int*), s = _d[0][0][dsz];
	d[0] = REALLOC(d[0], 2 + dsz, int);
	if (dsz == 1) {
		d[0][0] = 2, d[0][2] = (d[0][1] = l) - 1, d[1] = r;
		return;
	}
	if (row != dsz) {
		memmove(&d[row + 1], &d[row], sizeof(int*) * (dsz - row));
		memmove(&d[0][row + 1], &d[0][row], sizeof(int) * (dsz - row + 1));
	}
	d[row] = r, d[0][row] = l, (d[0][1 + dsz] = s + l - 1), ++**d;
}

int put_row(int ***d, int *r, int l) {
	int n = 0;
	while (n < ***d && *d[0][n] < *r) ++n;
	put_row_to(d, r, l, n);
	return n;
}

int* set_union(int *x, int *y, int lx, int ly, int *e) {
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

void delrow(int **c, int r) {
	check(c);
	free(c[r]),
	c[0][**c] -= c[0][r] - 1,
	memmove(&c[r], &c[r + 1], (**c - r - 1) * sizeof(int*)), // shift rows down
	memmove(&c[0][r], &c[0][r + 1], (**c - r) * sizeof(int)), // shift sizes down
	--**c; // update row and rows size
	check(c);
}

int merge_rows_to(int *x, int *y, int lx, int ly, int ***d) {
	int e, row, n;
	int *r = set_union(x, y, lx, ly, &e);
	int s = lx + ly - 1 - e;
	for (n = 0; n < s - 1; ++n)
		if ((row = find1(*d, r[n])) && d[0][row] != r) {
			if (d[0][row][0] < 0 && r[0] < 0 && d[0][row][0] != r[0])
				return 0;
			r = set_union(r, d[0][row], s, d[0][0][row], &e),
			(s += d[0][0][row] - 1 - e), delrow(*d, row), n = 0;
		}
	put_row(d, r, s);
	check(*d);
	return 1;
}

void merge_rows(int **c, int ***d, int i, int j) {
	if (i > j) { merge_rows(c, d, j, i); return; }
	int li = c[0][i], lj = c[0][j], l = li + lj - 1, e;
	check(c);
	if (d) {
		merge_rows_to(c[i], c[j], li, lj, d);
		check(*d);
		return;
	}
	int *row = set_union(c[i], c[j], li, lj, &e);
	l -= e, delrow(c, j), c[0][**c] += l - c[0][i], 
	free(c[i]), c[i] = row, c[0][i] = l;
	if (d) check(*d);
}

char require(int*** _c, int x, int y) {
	assert(x != y && (x > 0 || y > 0)); // at least one var
	int **c = *_c, i, j;
	check(c);
	find(c, x, y, &i, &j);
	if (!i)	{ if (j) put_in_row(c, j, x); else makeset(_c, x, y); }
	else if (i == j) return 1; // x,y appear at the same row
	else if (!j) put_in_row(c, i, y); // x appears and y doesnt, add y to x's row
	else if (!(*c[i] < 0 && *c[j] < 0)) // both exist, at least one var
		merge_rows(c, 0, i, j); // merge their rows
	else return 0; // can't match two different nonvars
	return 1;
}

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

char merge(int** x, int** y, int ***r) {
	check(x); check(y);
	if (x[0][**x] > y[0][**y]) return merge(y, x, r);
	for (int row = 1, rows = **x; row < rows; ++row)
		for (int col = 0, cols = ROWLEN(x, row) - 1; col < cols; ++col)
			for (int cc = 1, e = **y; cc < e; ++cc)
				if (bfind(x[row][col], y[cc], y[0][cc]) != -1) {
					putws(L"found!");
					check(*r);
					if (!merge_rows_to(y[cc], x[row], y[0][cc], x[0][row], r))
						return 0;
					check(*r);
				}
	return 1;
}

void test() {
	int **c = MALLOC(int*, 3);
	for (int n = 0; n < 3; ++n) c[n] = MALLOC(int, 4);
	c[0][0] = 3, c[0][1] = 4, c[0][2] = 3, c[0][3] = 5,
	c[1][0] = 4, c[1][1] = 6, c[1][2] = 7, c[1][3] = 0,
	c[2][0] = 2, c[2][1] = 9, c[2][2] = 0;
	check(c);
	putws(L"before:"), printc(c), put_in_row(c, 1, 5),
	putws(L"after:"), printc(c), fflush(stdout);
	check(c);
	int i, j;
	find(c, 6, 4, &i, &j), assert(i == 1 && j == 1);
	find(c, 6, 7, &i, &j), assert(i == 1 && j == 1);
	find(c, 6, 0, &i, &j), assert(i == 1 && j == 0);
	find(c, 9, 7, &i, &j), assert(i == 2 && j == 1);
	check(c);
	merge_rows(c, 0, 1, 2);
	check(c);
	putws(L"merge_rows(c, 1, 2):"), printc(c), fflush(stdout);
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
