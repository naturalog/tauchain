#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <wchar.h>
#include <wctype.h>

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

void putws(const wchar_t* x);
void putcs(char* x);
#define MALLOC(x, y) malloc(sizeof(x) * (y))
#define CALLOC(x, y) calloc(y, sizeof(x))
#define REALLOC(x, y, z) realloc(x, (y) * sizeof(z))

// Following 3 structs store the kb&query.
//
// int* arrays below (args, c) have first int as length,
// and are also zero terminated. Hence empty list consists of two
// integers, zero each.
//
// struct term encapsulates triples as well, for efficiency reasons.
// hence when the term repterments a triple, we'll sometimes emphasize
// it on the comments as "term (triple)"

// termource (IRI/literal/variable/list) or triple (where args contain spo)
struct term { 
	// types:
	// '.' list
	// '?' var
	// '_' bnode
	// 'I' IRI
	// 'L' literal
	// 'T' triple
	char type; 
	union {
		const wchar_t *value; // termource's value
		const int *args; // list's elements ids
	};
};
struct premise {
	// term (triple) id
	int p; 
	// equality relations (calculated from premises
	// and conclusions). e[r][k] is the int** equality 
	// relation of the k'th conclusion in the r's rule.
	int ** **e;
};
struct rule {
	int *c; // conclusions. every int is a term (triple) id.
		// having multiple conclusions encapsulated
		// in a rule has two advantages: First, naturally and efficiently
		// expterms generalized modus ponens (with both conjuncions
		// and disjunctions). Second, and more important, is that
		// once conclusions are grouped by premises, all ground
		// rules (aka facts) are grouped within one rule. So identifying
		// ground can be done by saving memory and indirections, or searches.
		// also for saving separation of compilation code.
	struct premise *p; // premises
	int np; // number of premises
};
typedef struct term term;
typedef struct rule rule;
typedef struct premise premise;
extern term *terms;
extern rule *rules;
extern int nts, nrs;

int mkterm(const wchar_t* s, char type); // constucts a nonlist term
int mkrule(premise *premises, int num_of_premises, int *conclusions);
int mktriple(int s, int p, int o); // returns a list term with spo as its items
// allocate premise with its e member.
// since e is indexed by rules id and
// its conclusions, the rule id is given
// to allocate the premise accordingly
premise* mkpremise(int r);

void print(const term* r); // print term
void printts(const int *t); // print array of terms
void printps(const premise *p, int num); // print premises
void printr(const rule* r); // print rule
void printa(int *a, int sz); // print array of integers
void printc(int **c); // print equivalence relation
// complete input is read into here first, then free'd by the callse after a parser pass
extern wchar_t *input; 

void parse();

// compile matching two resources into
// an equivalence relation
char compile_res(int ***e, int x, int y);
// compile equivalence class representing
// conditions of given premise to match
// given conclusion
char compile_pc(int r, int p, int r1, int c);
