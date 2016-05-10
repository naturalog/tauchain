#include <stdbool.h>
#include <wchar.h>

void* set_create(void *key);
unsigned term_create(int, int **l = 0); // var is pos, nonvar is neg, list is zero
bool set_require(void* set, int r, int l); // term=term
bool set_merge(void*, void*, void**);
unsigned set_compact(void*);
