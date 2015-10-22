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
#define swap(t, x, y) { t tt = y; y = x, x = tt; }
#define ROW(x, y) x[y]
#define ROWLEN(x, y) x[0][y]
