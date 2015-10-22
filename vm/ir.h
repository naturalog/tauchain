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
struct premise {
	int p;
	// equality relations (calculated from premises
	// and conclusions). e[r][k] is the int** equality 
	// relation of the k'th conclusion in the r's rule.
	int ** **e;
};
struct rule {
	int *c; // conclusions
	struct premise *p; // premises
	int np; // number of premises
};
typedef struct res res;
typedef struct triple triple;
typedef struct rule rule;
typedef struct premise premise;
extern res *rs;
extern triple* ts;
extern rule* rls;
extern int nrs, nts, nrls;

int mkres(const wchar_t* s, char type);
int mktriple(int s, int p, int o);
int mkrule(premise *p, int np, int *c);
void print(const res* r);
void printt(const triple* t);
void printts(const int *t);
void printps(const premise *p, int np);
void printr(const rule* r);
void printa(int *a, int l);
void printc(int **c);
