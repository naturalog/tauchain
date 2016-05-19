#include "ast.h"

struct wrd {
	int **c;
	pcch *str;
	uint sz;
	wrd(const word& w);
};

ostream& operator<<(ostream& os, const wrd& w);
