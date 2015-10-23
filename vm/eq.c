//#include "eq.h"
//#include "vm.h"
#include <assert.h>
#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

#define MALLOC(x, y) malloc(sizeof(x) * (y))
#define CALLOC(x, y) calloc(y, sizeof(x))
#define REALLOC(x, y, z) realloc(x, (y) * sizeof(z))
#define SHR(x, y, l, z) memmove(&(x)[y + 1], &(x)[y], sizeof(z) * (l - (y)))
#define SHL(x, y, l, z) memmove(&(x)[y], &(x)[y - 1], sizeof(z) * (l - (y)))
#define FOR(x, y) 	for (int x = 0; x < y; ++x)

struct rel { // equivalence relations
	int rows, members; // number of rows, and their total sizes
	int *sizes; // size of each row (class) without the terminating null
	int **classes; // null-terminated arrays
};
typedef struct rel rel;

// rel macros
#define size(x, y) 	(x).sizes[y]
#define row(x, y) 	(x).classes[y]
#define get(x, y, z) 	row(x, y)[z]

#define resize_row(x, y, z) \
			(x).members += z, size(x, y) += z \
			row(x, y) = REALLOC(int*, size(x, y))
#define resize_rel(x, y) \
			(x).rows += y, (x).sizes = REALLOC(int, (x).rows) \
			(x).classes = REALLOC(int*, (x).rows)

#define zero_rel(x) 	(x).rows = (x).members = (x).classes = (x).sizes = 0
#define rep(x, y)	get(x, y, 0)

#define shr_rel(x, y) \
	resize_rel(x, 1), \
	SHR(x, y, int*, (x).rows), \
	SHR(x.sizes, y, int, (x).rows)

#define shl_rel(x, y) \
	SHL(x, y, int*, (x).rows), \
	SHL(x.sizes, y, int, (x).rows), \
	resize_rel(x, -1)

#define shr_row(x, y, z) \
	resize_row(x, y, 1), \
	SHR(row(x, y), z, int, size(x, y)), \

#define shl_row(x, y) \
	SHL(row(x, y), z, int, size(x, y)), \
	resize_row(x, y, 1)

#define row_has(x, y, z) get(x, y, find_min_geq(row(x, y), size(x, y), z)) == z

#define add_to_rel(r, x, l, p) \ 
	(p = find_min_rep_geq(r, *x)), \
	shr_rel(x, p), size(r, p) = l, row(r, p) = x, check(r), p

#define add_to_row(r, c, x, p)
	(p = find_min_geq(row(r, c), x)), \
	shr_row(r, c, p), get(r, c, p) = x, check(r), p

int add_to_rel(rel r, int *x, int l) {
int add_to_row(rel r, int c, int x) {
int find_min_geq(const int* r, int l, int x) {
int find_min_rep_geq(rel r, int x) {
int rel_has(rel r, int x) {

int add_to_rel(rel r, int *x, int l) {
	int p = find_min_rep_geq(r, *x);
	return shr_rel(x, p), size(r, p) = l, row(r, p) = x, check(r), p;
}

int add_to_row(rel r, int c, int x) {
	int p = find_min_geq(row(r, c), x);
	return shr_row(r, c, p), get(r, c, p) = x, check(r), p;
}

int find_min_geq(const int* r, int l, int x) {
	int i=0, k;
	while (i!=l) if (r[k=(i+l)/2] <= x) i=k+1; else l=k;
	return assert(r[i] == x || (r[i] > x && (!i || r[i-1] < x))), i;
}

// row with minimal geq representative
int find_min_rep_geq(rel r, int x) {
	int i=0, j=r.rows, k;
	while (i!=j) if (rep(r, k=(i+j)/2) <= x) i=k+1; else j=k;
	return assert(rep(r, i) == x || (rep(r, i) > x && (!i || rep(r, i-1) < x))), i;
}

int rel_has(rel r, int x) {
	for (int n = 0; n < r.rows(); ++n)
		if (row_has(row(r, n), x))
			return n;
	return -1;
}

rel merge(rel r, int *r, int l) {
	rel m;
	for (int n = 0; n < l; ++n)
		if (rel_has(r, r[n]))
}

void check(rel r) {
	int t = 0;
	FOR (n, r.rows) {
		// assert null termination and positive size
		assert(size(r, n) > 0 && !get(r, n, size(r, n))),
		assert(!n || rep(r, n-1) < rep(r, n)); // assert order
		t += size(r, n);
		FOR (k, size(r, n)) {
			assert(get(r, n, k)); // assert nonzero
			if (k) { // item isn't first
				assert(get(r, n, k) > 0), // positivity
				assert(get(r, n, k-1) < get(r, n, k)); // assert order
			}
			FOR (n1, r.rows) if (n != n1) // assert uniqueness
				FOR (k1, size(r, n1)) if (k != k1)
					assert(get(r, n, k) != get(r, n1, k1));
			assert(k == find_min_geq(row(r, n), size(r, n)), get(r, n, k));
		}
	}
	assert(t == r.members);
}

#ifdef OLD

rels make_set(int x, int y) {
	if (x > y) return make_set(y, x);
	rels r;
	r.sz = 1, r.tot = 2,
	r.sizes = MALLOC(int, 1), *r.sizes = 2,
	r.classes = MALLOC(int*, 1), *r.classes = MALLOC(int, 2),
	get(r, 0, 0) = x, get(r, 0, 1) = x;
}

int* get_row(int **c, int r)		{ return c[r]; }
// row len without terminating null
int row_len(int **c, int r)	 	{ return c[0][r] - 1; }
int row_len2(int **c, int* r)	 	{ return c[0][r - c[0]] - 1; }
int num_items(int **c)			{ return c[0][**c]; }
int num_rows(int **c)			{ return **c; }
int* row_begin(int **c, int r) 		{ return c[r]; }
int* row_end(int **c, int r) 		{ return &get_row(c, r)[row_len(c, r)]; }
char is_bound(int **c, int r)		{ return c[r][0] < 0; }
int get(int **c, int r, int i)		{ return c[r][i]; }
void set(int **c, int r, int i, int x)	{ c[r][i] = x; }
void set_row_len(int **c, int r, int l) { c[0][r] = l; }
void set_num_items(int **c, int n)	{ c[0][**c] = n; }
char can_merge(int *x, int *y)		{ return *x > 0 || *y > 0 || *x == *y; }
void inc_tot_num(int **c, int n)	{ c[0][**c] += n; }
int** rows_begin(int **c, int *sz)	{ return num_rows(c) ? *sz = row_len(c, 1), &c[1] : 0; }
int   row_id(int **c, int **r)		{ return r - rows_begin(c); }
int** rows_end(int **c) 		{ return &c[num_rows(c)]; }
void  rows_next(int **c, int ***r, int *sz) {
	if (++*r == rows_end()) *r = 0;
	else *sz = row_len2(c, **r);
}
void inc_rows_num(int ***c, int n) {
	***c += n,
	*c = REALLOC(*c, ***c, int**),
	**c = REALLOC(**c, ***c + 1, int*);
}
void array_shr(int *r, int from, int to, int x) {
	assert(x && r && from && from < to),
	((int*)memmove(&r[from + 1], &r[from], to - from))[from] = x;
}
void array_shr2(int **r, int from, int to, int *x) {
	assert(x && r && from && from < to),
	((int**)memmove(&r[from + 1], &r[from], to - from))[from] = x;
}
void array_shl(int *r, int from, int to) {
	assert(r && from && from < to),
	(int*)memmove(&r[from], &r[from + 1], to - from);
}
// returns the location of the first in r
// element that is >=x. l is the number of
// items in r.
int find_min_geq(const int* r, int x, int l) {
	int i = 0, j = l - 1, k;
	while (i != j) r[k = (i + j) / 2] <= x ? i = k + 1 : j = k;
	assert(l && r && x && i == j && x < r[i] && (!i || x > r[i-1]);
	return i;
}
int find_min_row_geq(const int** c, int x) {
	int i = 0, j = l - 1, k;
	while (i != j) *c[k = (i + j) / 2] <= x ? i = k + 1 : j = k;
	assert(l && c && x && i == j && x < *c[i] && (!i || x > *c[i-1]);
	return i;
}
int find_eq(const int* r, int x, int l) {
	int m = find_min_geq(r, x, l);
	return r[m] == x ? m : 0;
}
int find_in_rel(int **c, int x) {
	assert(x);
	int sz, **r = rows_begin(c, &sz);
	while (r) {
		if (find_eq(*r, x, sz)) return row_id(*r);
		rows_next(c, &r, &sz), assert(sz || !r);
	};
	return 0;
}
void find2(int **c, int x, int y, int *i, int *j) {
	if (x > y) { find(c, y, x, j, i); return; }
	*i = *j = 0;
	int sz, **r = rows_begin(c, &sz), *g;
	while (r) {
		if ((g = find_eq(r, x, sz))) {
			*i = row_id(*r);
			if (*j) return;
			if ((g = find_eq(r + g, y, sz - g))) {
				*j = row_id(*r);
				return;
			}
		}
		if ((g = find_eq(r, y, sz))) {
			*j = row_id(*r);
			if (*i) return;
		}
		rows_next(c, &r, &sz), assert(sz || !r);
	}
}


#ifdef DEBUG
void check(int **c) {
//	char bound;
	int s = 0, n = 0, n1, *k1, rr, **r;
	printc(c), fflush(stdout);
	// recalc total size
	for (; n < num_rows(c); ++n) s += row_len(c, n);
	assert(s == num_items(c)); // assert total size
	for (r = rows_begin(c); r != rows_end(c); ++r) {
		rr = row_id(c, r);
		assert(!*row_end(c, rr)); // assert zero termination matching the sizes vector
		for (int *k = row_begin(c, rr); k != row_end(c, rr); ++k) {
			assert(*k); // verify nonzero
			if (!is_bound(c, rr) || *k) {
				if (!*k)
					wprintf(L"n=%d,k=%d,bound=%d,c[n][k]=%d\n", n, k, (int)is_bound(c, rr), *k),
					fflush(stdout);
				assert(*k > 0); // assert only one nonvar binding
			}
			for (n1 = 1; n1 < **c; ++n1)
				for (k1 = row_begin(c, rr); k1 != row_end(c, rr); ++k1)
					if (n != n1 && k != k1)
						assert(*k != *k1); // assert global uniqueness
		}
	}
}
#endif

char test_findmin_geq() {
	int *a = MALLOC(int, 10);
	for (int n = 0; n < 10; ++n) a[n] = n;
	for (int n = 0; n < 10; ++n) assert(find_min_geq(n, a, 10) == n);
	for (int n = 0; n < 10; ++n) a[n] = 2 * n + 1;
	for (int n = 0; n < 10; ++n) assert(a[find_min_geq(2 * n, a, 10)] == 2 * n + 1);
	free(a);
	return 1;
}
	
int* make_row(int x, int y) {
	if (x > y) return make_row(y, x);
	int *r = MALLOC(int, 3);
	return r[0] = x, r[1] = y, r[2] = 0, r;
}

void makeset(int ***_c, int x, int y) {
	if (x > y) return makeset(y, x);
	int *r = make_row(x, y), **c = *_c;
	inc_tot_num(c, 2), inc_rows_num(c, 1);
	int t = find_min_row_geq(c, x);
	array_shr(r, t);


	
	int **c = *_c = REALLOC(*_c, 1 + rows, int*), t = c[0][rows];
	c[rows] = r, **c = ++rows, // add row and update rows count
	(*c = REALLOC(*c, 2 + rows, int*))[rows - 1] = 2; // update row size
	c[0][rows] = t + 2; // update total count
#ifdef DEBUG	
	int i, j; check(c), find(*_c, x, y, &i, &j), assert(i == j);
#endif	
}

int enlarge_row(int **c, int r) { // enlarge row by one
	assert(r && !c[r][c[0][r]]); // invalid for first row
	check(c);
       	c[r] = REALLOC(c[r], ++c[0][r], int);
	++c[0][**c]; // update total count
	return c[0][r]; // return new size
}
// duplicate the p'th item, then put x in the first location
void shift_right(int **c, int r, int p, int x) { 
	enlarge_row(c, r);
	memmove(&c[r][p+1], &c[r][p], c[0][r] - p + 2);
	assert(c[r][p] > x);
	c[r][p] = x;
}
int *firstgeq(int *a, int x) { while (*a < x) ++a; return a; }

void put_in_row(int **c, int i, int x) { // put x in the i'th row
	check(c);
	assert(!c[i][c[0][i]]);
	if (x < *c[i]) { shift_right(c, i, 0, x); return; }
	if (x > c[i][c[0][i] - 2]) { enlarge_row(c, i); c[i][c[0][i] - 2] = x; return; }
	shift_right(c, i, firstgeq(c[i], x) - c[i], x);
#ifdef DEBUG	
	check(c), assert(i == find1(c, x));
#endif	
}

void put_row_to(int ***_d, int *r, int l, int row) {
	int dsz = ***_d, **d = *_d = REALLOC(*_d, 1 + dsz, int*), s = _d[0][0][dsz];
	d[0] = REALLOC(d[0], 2 + dsz, int);
	if (dsz == 1) {
		d[0][0] = 2, d[0][2] = d[0][1] = l, d[1] = r;
		return;
	}
	if (row != dsz) {
		memmove(&d[row + 1], &d[row], sizeof(int*) * (dsz - row));
		memmove(&d[0][row + 1], &d[0][row], sizeof(int) * (dsz - row + 1));
	}
	d[row] = r, d[0][row] = l, (d[0][1 + dsz] = s + l), ++**d;
}

int put_row(int ***d, int *r, int l) {
	int n = 0;
	while (n < ***d && *d[0][n] < *r) ++n;
	put_row_to(d, r, l, n);
	return n;
}

int* set_union(int *x, int *y, int *r, int *e) {
	int *rr = r;
	*e = 0;
	while (*x && *y)
		if (*x == *y) (*r++ = *x++), *y++, ++*e;
		else *r++ = (*x < *y ? *x++ : *y++);
       	while (*x) *r++ = *x++;
       	while (*y) *r++ = *y++;
	return *r = 0, rr;
}

void delrow(int **c, int r) {
	check(c);
	free(c[r]),
	c[0][**c] -= c[0][r],
	memmove(&c[r], &c[r + 1], (**c - r - 1) * sizeof(int*)), // shift rows down
	memmove(&c[0][r], &c[0][r + 1], (**c - r + 1) * sizeof(int)), // shift sizes down
	--**c; // update row and rows size
	check(c);
}

int merge_rows_to(int *x, int *y, int s, int ***d) {
	int e, row, n, *r = set_union(x, y, MALLOC(int, s), &e), s -= e;
	for (n = 0; n < s; ++n)
		if ((row = find1(*d, r[n])) && get_row(*d, row) != r) {
			if (is_bound(*d, row) && *r < 0 && get(*d, row, 0) != r[0])
				return 0;
			r = set_union(r, get_row(*d, row), s, row_len(*d, row), &e),
			(s += row_len(*d, row) - e), delrow(*d, row), n = 0;
		}
	put_row(d, r, s);
	check(*d);
	return 1;
}

void merge_rows(int **c, int ***d, int i, int j) {
	if (i > j) { merge_rows(c, d, j, i); return; }
	int li = row_len(c, i), lj = row_len(c, j), l = li + lj, e;
	check(c);
	if (d) {
		merge_rows_to(get_row(c, i), get_row(c, j), li, lj, d);
		check(*d);
		return;
	}
	int *row = set_union(c[i], c[j], li, lj, &e);
	l -= e, delrow(c, j),
	set_num_items(c, num_items(c) + l - row_len(c, i)),
	free(c[i]), c[i] = row, set_row_len(c, i, l);
	if (d) check(*d);
}

char require(int*** c, int x, int y) {
	assert(x != y && (x > 0 || y > 0)); // at least one var
	int i, j;
	check(*c);
	find(*c, x, y, &i, &j);
	if (!i)	{ if (j) put_in_row(*c, j, x); else makeset(c, x, y); }
	else if (i == j) return 1; // x,y appear at the same row
	else if (!j) put_in_row(*c, i, y); // x appears and y doesnt, add y to x's row
	else if (!(*c[0][i] < 0 && *c[0][j] < 0)) // both exist, at least one var
		merge_rows(*c, 0, i, j); // merge their rows
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
		for (int col = 0, cols = x[0][row] - 1; col < cols; ++col)
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

int** create_relation() {
	int **e = MALLOC(int*, 1);
	return *e = MALLOC(int, 2), e[0][0] = 1, e[0][1] = 0, e;
}

void test() {
	test_bfindgeq();
	int **c = MALLOC(int*, 3);
	for (int n = 0; n < 3; ++n) c[n] = MALLOC(int, 4);
	c[0][0] = 3, c[0][1] = 3, c[0][2] = 2, c[0][3] = 5,
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
	int **d = create_relation();
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
#endif
