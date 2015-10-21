#include <wchar.h>

// Following 3 structs store the kb&query.
// All int* arrays below (args, c, p) have first int as length,
// and are also zero terminated. Hence empty list consists of two
// integers, zero each.
struct res { // resource (IRI/literal/variable/list)
	char type; // '.' for list, '?' for var, else otherwise
	union {
		const wchar_t *value; // resource's value
		const int *args; // list's elements ids
	};
};
struct triple {
	int s, p, o; // subject predicate object
};
struct rule {
	int *p; // premises
	int *c; // conclusions
	int *** **e; // equality relations (calculated from p&c)
		// e[n][k][r] is the int** equality relation of
		// the k'th conclusion of the n'th premise
		// in the r's rule
};
extern struct res *rs;
extern struct triple* ts;
extern struct rule* rls;
extern size_t nrs, nts, nrls;

int mkres(const wchar_t* s, char type);
int mktriple(int s, int p, int o);
int mkrule(int *p, int *c);
void print(const struct res* r);
void printt(const struct triple* t);
void printts(const int *t);
void printr(const struct rule* r);
void printa(int *a, int l);
void printc(int **c);
