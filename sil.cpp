#include <iostream>
#include <cstdlib>
using namespace std;

// sorted integer list
struct sil {
	const int *d; // first item - length
	sil(const int* _d) : d(_d) { }
	void print() {
		for (int n = 0; n < *d; ++n)
			cout << d[1+n] << ',';
		cout << endl;
	}
};

sil operator+(const sil& x, const sil& y) { // union
	register const int *a, *b;
	const int sa = *(a = x.d)++, sb = *(b = y.d)++;
	register int *r, &rr = *(r = new int[sa + sb])++ = 0;
	register const int *ea = sa + a, *eb = sb + b;
	register int u = *a, v = *b;
	for (; a != ea && b != eb; ++rr)
		if (u > v) *r++ = v, v = *++b;
		else if (u < v) *r++ = u, u = *++a;
		else *r++ = v, u = *++a, v = *++b;
	while (a != ea) ++rr, *r++ = *a++;
	while (b != eb) ++rr, *r++ = *b++;
	return sil(&rr);
}

sil operator*(const sil& x, const sil& y) { // intersection
	register const int *a, *b;
	const int sa = *(a = x.d)++, sb = *(b = y.d)++;
	register int *r, &rr = *(r = new int[min(sa, sb)])++ = 0;
	register const int *ea = sa + a, *eb = sb + b;
	register int u = *a, v = *b;
	while (a != ea && b != eb)
		if (u > v)
			v = *++b;
		else if (v > u)
			u = *++a;
		else
			++rr, *r++ = *a++, v = *++b, u = *a;
	return sil(&rr);
}

int main() {
	int _x[] = { 5, 1, 2, 3, 4, 5 };
	int _y[] = { 4, 2, 4, 6, 8 };
	sil x(_x), y(_y);
	sil u = x + y;
	sil i = x * y;
	x.print(), y.print();
	u.print(), i.print();
}
