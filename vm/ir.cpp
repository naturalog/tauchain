#include "ir.h"

template<typename T>
struct onret { T f; onret(T f):f(f){} ~onret(){f();} };

wrd::wrd(const word& w) {
	sz = w.size();
	c = new int*[sz];
	str = new pcch[sz];
	uint i = 0, j;
	for (const crd& r : w) {
		c[i]=new int[r.c.size()+1];str[i]=r.str;c[i][r.c.size()]=0;j=0;
		for (int x : r.c) c[i][j++] = x;
		++i;
	}
}

ostream& operator<<(ostream& os, const wrd& w) {
	for (uint n = 0; n < w.sz; ++n) {
		int *i = w.c[n];
		while (*i) os << *i++ << ':';
		if (w.str[n]) os << w.str[n];
		os << endl;
	}
	return os;
}

void refute(wrd &kb, int q) {
}
