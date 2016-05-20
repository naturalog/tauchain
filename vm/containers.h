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

struct refutation {}; 
struct literal_clash : public refutation{};
struct prefix_clash : public refutation {};
struct occur_fail : public prefix_clash {};

template<typename T, typename V>
struct iter {
	V *b;
	int p;
	iter(V *b = 0, int p = 0) : b(b), p(p) {}
	T& operator*()  { return b->a[p]; }
	T* operator->() { return &b->a[p];}
	iter operator++(){
		return 	p < b->sz	?
			(iter)(++p,*this):
			b->n		?
			b->n->begin()	:
			(iter)(b=0,*this);}
	operator bool() { return b; }
}; 
template<typename T>
struct lary { // array given in parts as linked list
	T *a;
	uint sz;
	lary<T> *n;
	bool del;
	lary(T *a, uint sz = 0, lary<T> *n = 0) : a(a), sz(sz), n(n), del(false) {}
	lary(uint sz = 0, lary<T> *n = 0) : a(new T[sz]), sz(sz), n(n), del(true){}
	lary(const lary<T>& t) : a(t.a), sz(t.sz), n(t.n), del(false) {}
	lary(lary<T>&& t) : a(t.a), sz(t.sz), n(t.n), del(t.del) {}
	~lary() { if (del) delete[] a; }
	lary<T>& operator=(const lary<T>& t) { return a=t.a,sz=t.sz,n=t.n,del=false, *this; }

	iter<T,lary<T>> begin(){ return iter<T,lary<T>>(this, 0); }
	iter<T,lary<T>> end()  { return iter<T,lary<T>>(); }
	iter<T,const lary<T>> begin()const{return iter<const T,const lary<T>>(this,0);}
	iter<T,const lary<T>> end()  const{return iter<const T,const lary<T>>();}
	const T& operator[](uint k) { return k < sz ? a[k] : (*n)[k-sz]; }
	void push_back(const T& t) {
		if (n) n->push_back(t);
		else *(n = new lary<T>(1))->a = t;
	}
	void push_back(const lary<T>& t) { if (n) n->push_back(t); else n = &t; }
	lary<T>* push_front(T& t) {
		lary<T> *r = new lary(1, this);
		return *r->a = t, r;
	}

	bool cmp(const lary<T>& x) throw(prefix_clash) {
		auto i1 = begin(), i2 = x.begin();
		while(i1&&i2)if(*i1!=*i2)return false;
		if(!i1==!i2)return true;
		throw prefix_clash();
	}
	std::ostream& operator>>(std::ostream& os) const {
		auto y = *this;
		for (auto x : y) os << x << ' ';
		return os;
	}
};
/*
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
*/
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
	vector(const T& t) : vector() { push_back(t); }
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
