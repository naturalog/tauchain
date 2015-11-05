#include <stdbool.h>

typedef struct arr arr;
typedef struct diff diff;
typedef union data data;
typedef struct parr parr;
typedef struct uf uf;

struct arr  { int s,*a; 		};
struct diff { int i, v; parr* t;	};
union  data { arr  a;	diff d; 	};
struct parr { data d;	bool isdiff;	};
struct uf   { parr *rank, *rep;		};

uf*   create(int n);
int   find(uf* h, int x);
uf*   unio(uf *h, int x, int y);
