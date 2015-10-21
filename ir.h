#include <wchar.h>

// Following 3 structs store the kb&query.
// res::type is '.' for list, '?' for var, and 0 otherwise.
// All three int* below (args, c, p) have first int as length,
// and last int always null. Hence empty list consists of two
// integers, zero each.
struct res { char type; union { const wchar_t *value; const int *args; }; };
struct triple { int s, p, o; }; 
struct rule { int *p, *c; }; // premises and conclusions
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
