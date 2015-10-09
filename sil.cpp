#include <iostream>
#include <cstdlib>
using namespace std;

///////// null-terminated lists version

// since union is more efficient with leading-size, and intersection
// is more efficient with terminating zero, we'll do both.

void print(const int* x) { while (*++x) cout << *x << ','; cout << endl; }

bool common(const int* a, const int* b) { 
	register int u = *a, v = *b, d;
	while (a && b)
		if (!(d = u-v)) return true;
		else if (d > 0) v = *++b;
		else u = *++a;
	return false;
}

const int* all(const int* a, const int* b) {
	register int *r, &rr = *(r = new int[*a++ + *b++])++ = 0, d, u = *a, v = *b;
	for (; u && v; ++rr)
		if (!(d = u - v)) *r++ = v, u = *++a, v = *++b;
		else if (d > 0) *r++ = v, v = *++b;
		else *r++ = u, u = *++a;
	if (u) while ((*r++ = *a++)) ++rr;
	if (v) while ((*r++ = *b++)) ++rr;
	return &rr;
}

int main() {
	int x[] = { 5, 1, 2, 3, 4, 5, 0 }; // size first, null last
	int y[] = { 4, 2, 4, 6, 8, 0 };
	const int* u = all(x, y);
	print(x), print(y), print(u);
	if (!common(x, y)) throw 0;
}
