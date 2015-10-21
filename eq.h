
// Conditions have the form x=y=z=...=t.
// and represented as int** c, two dimensional array
// where both arrays are ordered in ascending order,
// and the outer array is ordered by the first element
// of the inner arrays. 
// c[0] contains lengths only: **c contains the number
// of arrays in c, and (*c)[n] contains the length of
// the n'th equivalence class. the classes therefore
// begin from index 1. the last element of c[0] contains
// the sum of all sizes of all rows (except row 0). all rows
// except the first are zero-terminated, and the sizes on
// the first row include the zero, except the total size
// on the last int on the first row, (*c)[**c] or c[0][c[0][0]].

#ifdef DEBUG
// various very expensive assertions, for debug/unittest only
void check(int **c);
#else
#define check(x)
#endif
// simple binary search over integer array of length l
int bfind(int x, const int *a, int l);
int find1(int **c, int x);
// find rows for two given labels/vars. it's
// slightly more efficient than searching each separately
void find(int **c, int x, int y, int *i, int *j);
// add a new class to existing relation, specifying
// its first two members
void makeset(int ***_c, int x, int y);
// add x to row i in c
void put_in_row(int **c, int i, int x);
// add a row of length l to given location
void put_row_to(int ***_d, int *r, int l, int row);
// add a row of length l, return row num
int put_row(int ***d, int *r, int l);
// returns union taking into account equal items.
// e returns the number of equal items.
int* set_union(int *x, int *y, int lx, int ly, int *e);
void delrow(int **c, int r);
// merge rows x,y and store merged row in another
// equivalence relation d
int merge_rows_to(int *x, int *y, int lx, int ly, int ***d);
// merge rows i,j in c.
// if d!=0, merge into a new row in d.
void merge_rows(int **c, int ***d, int i, int j);
// require() assumes that canmatch() returned true
// hence does not perform checks for the validity
// of the requirement.
char require(int*** _c, int x, int y);
// copy while omitting one row
int **cprm(int **c, int r);
// merge two equivalence relations into one
// if possible
char merge(int** x, int** y, int ***r);
void test();
