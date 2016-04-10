#include "parr.h"

PARR_DECL(int, parr);

struct uf { parr *rank, *rep; };
typedef struct uf uf;
uf*   create(int);
int   find(uf*, int);
uf*   unio(uf*, int, int);
void  uftest();
