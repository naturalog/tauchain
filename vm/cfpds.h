#include <stdbool.h>

typedef struct arr arr;
typedef struct diff diff;
typedef union data data;
typedef struct parr parr;
typedef struct uf uf;

struct arr  { int s,*a; 		};
struct diff { int i, v; parr* t;	};
//union  data { arr  a;	diff d; 	};
struct parr { union { arr a; diff d; }; bool isdiff;};
struct uf   { parr *rank, *rep;		};

uf*   create(int);
int   find(uf*, int);
uf*   unio(uf*, int, int);
void  uftest();
