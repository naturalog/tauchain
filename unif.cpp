#include <map>
#include <iostream>
#include <cstring>
using namespace std;

typedef long i64;
typedef map<i64, i64> subs;

void print(i64* t, size_t sz) {
	while (sz--) cout << (*t++) << ' ';
	cout << endl;
}

bool unify(i64* _s, i64* _d, size_t sz, subs& ssub, subs& dsub) {
	i64 __s[sz], __d[sz];
	i64 *s = __s, *d = __d;
	memcpy(s, _s, sz * 8);
	memcpy(d, _d, sz * 8);
	i64 *end = &s[sz];
	subs::const_iterator it;

	auto dosub = [&](i64 x, i64 y, bool loop = true) {
		for (size_t n = 0; n < sz; ++n) {
			if (__s[n] == x) __s[n] = y;
			if (__d[n] == x) __d[n] = y;
		}
	};

	auto sub = [&](i64 x, i64 y) {
		it = ssub.find(x);
		if (it != ssub.end() && it->second != y && it->second > 0)
			return false;
		ssub.emplace(x, y);
		dsub.emplace(x, y);
		dosub(x, y, false);
		return true;
	};

	for (auto x : ssub) dosub(x.first, x.second);

	while (s != end) {
		if (*s < 0 && *s > *d && !sub(*s, *d)) return false;
		if ((*s < 0 || *d < 0) && !sub(*d, *s)) return false;
		if (*s++ != *d++) return false;
	}

	return true;
}

int main() {
	i64 t1[] = { -1, 2, 3, 4, 5 };
	i64 t2[] = { -9, 2, 3, -9, 5 };
	subs s,d;
	if (unify(t1, t2, 5, s, d)) cout << "passed" << endl;
	else cout << "failed" << endl;
	print(t1,5);
	print(t2,5);
	cout << "S:" << endl;
	for (auto x : s) cout << x.first << ' ' << x.second << endl;
	cout << "D:" << endl;
	for (auto x : d) cout << x.first << ' ' << x.second << endl;
}
