#ifndef __CONTAINERS_H__
#define __CONTAINERS_H__
#include <string>
#include <cstring>
#include <iostream>
#include <functional>
#include <tuple>
#include <exception>
//#include <sstream>
#include <cassert>
#include <cstdlib>

typedef int word;
struct arr  { int s; word *a; };
struct diff { int i; word v; struct parrint* t;};

struct parrint {
	bool isdiff;
	union {
		arr a;
		diff d;
	};
};
typedef parrint parr;

parrint mkarr(int *a, word s);
parrint* alloc_arr(int *a, word s);
parrint mkdiff(int i, word v, parrint* t);
void reroot(parrint *t);
word get(parrint *t, int i);
parrint* set(parrint *t, int i, int v);

struct uf {
	parr *rank, *rep;
	uf*   create(int);
	int   find(int);
	uf*   unio(int, int);
};

uf*   create(int);
int   find(uf*, int);
uf*   unio(uf*, int, int);
void  uftest();

typedef uf puf;

template <typename T> struct sp { // smart pointer
	T *p = 0;
	int *r = 0;
	sp() {}
	sp(T *v) : p(v) { *(r = new int) = 1; }
	sp(const sp<T> &s) : p(s.p), r(s.r) { if (r) ++*r; } 
	~sp() { if (r && !--*r) delete p, delete r; } 
	T &operator*() { return *p; }
	T *operator->() { return p; }
	const T &operator*() const { return *p; }
	const T *operator->() const { return p; } 
	sp<T> &operator=(const sp<T> &s) {
		if (this == &s) return *this;
		if (r && !--*r) delete p, delete r;
		p = s.p;
		if ((r = s.r)) ++*r;
		return *this;
	}
	operator bool() const { return p; }
};

const size_t chunk = 4;
template <typename T, bool ispod = std::is_pod<T>::value> class vector {
protected:
	T *a;
	const size_t szchunk = chunk * sizeof(T);
	size_t n, c;

public:
	typedef T *iterator;
	vector() : a(0), n(0), c(0) {}
	vector(const vector<T> &t) : a(0), n(0), c(0) { copyfrom(t); }
	vector<T> &operator=(const vector<T> &t) { copyfrom(t); return *this; }
	bool operator==(const vector<T> &t) const {
		return n == t.n && !memcmp(a, t.a, n * sizeof(T));
	} 
	T &operator[](size_t k) const { return a[k]; }
	size_t size() const	{ return n; }
	T *begin() const	{ return a; }
	T *end() const		{ return a ? &a[n] : 0; }
	bool empty() const	{ return !a; }
	~vector()		{ clear(); }
	iterator back()		{ return n ? &a[n - 1] : 0; }
	void clear() {
		n = c = 0;
		if (!a) return;
		free(a), a = 0;
	}
	T &push_back(const T &t) {
		if (!(n % chunk)) a = (T *)realloc(a, szchunk * ++c);
		if (ispod) a[n] = t;
		else new (&a[n])(T)(t);
		return a[n++];
	} 
protected:
	void copyfrom(const vector<T> &t) {
		clear();
		if (!(n = t.n)) return;
		memcpy(a = (T *)malloc(t.c * szchunk), t.a, (c = t.c) * szchunk);
	}
};

template <typename K, typename V> struct mapelem {
	K first;
	V second;
	mapelem() {}
	mapelem(const mapelem &m) : first(m.first), second(m.second) {}
	mapelem(const K &_k, const V &_v) : first(_k), second(_v) {}
};

template <typename K, typename V, bool sorted = false>
struct map : public vector<mapelem<K, V>> {
	typedef mapelem<K, V> vtype;
	typedef vector<mapelem<K, V>> base;

public:
	map(const vector<vtype> &t) { copyfrom(t); }
	map() : base() {}
	map(const map<K, V> &_s) : base() { base::copyfrom(_s); }
	map<K, V> &operator=(const map<K, V> &m) {
		base::copyfrom(m);
		return *this;
	}
	V &operator[](const K &k) {
		mapelem<K, V> *z = find(k);
		return z ? z->second : set(k, V());
	}
	V &set(const K &k, const V &v = V()) {
		vtype *z = find(k);
		if (z) { return z->second = v; }
		//		if (sorted) qsort(base::a, base::n, sizeof(vtype), compare);
		return base::push_back(vtype(k, v)).second;
	}
	typedef vtype *iterator;
	iterator find(const K &k) const {
		if (!base::n) return 0;
//		if (!sorted) {
			for (vtype &x : *this) if (k == x.first) return &x;
			return 0;
//		}
//		vtype v(k, V());
//		vtype* z = (vtype*)bsearch(&v, base::a, base::n, sizeof(vtype), compare);
//		return z;
	}

private:
	static int compare(const void *x, const void *y) {
		return ((vtype *)x)->first - ((vtype *)y)->first;
	}
};
#endif
