#include <string.h> // for memcpy

template<typename T>
struct sp { // smart pointer
	T* p = 0;
	int* r = 0;
	sp() {}
	sp(T* v) : p(v) 			{ *(r = new int) = 1; }
	sp(const sp<T>& s) : p(s.p), r(s.r) 	{ if (r) ++*r; }

	~sp() { if (r && !--*r) delete p , delete r; }

	T& operator*()  { return *p; } 
	T* operator->() { return p; }
	const T& operator*()  const { return *p; } 
	const T* operator->() const { return p; }
    
	sp<T>& operator=(const sp<T>& s) {
		if (this == &s) return *this;
		if (r && !--*r) delete p , delete r;
		p = s.p;
		if ((r = s.r)) ++*r;
		return *this;
	}
	operator bool() const { return p; }
};

template<typename T>
class list {
	struct impl {
		T t;
		sp<impl> next;
		impl(const T& _t) : t(_t), next(0) {}
		impl(const T& _t, sp<impl> n) : t(_t), next(n) {}
	};
       	sp<impl> first, last;
public:
	list() : first(0), last(0) {}
	list(const T& t) 		{ first = last = new impl(t); }
	list(const T& x, const T& y)	{ first = new impl(x, last = new impl(y)); }
	list(const T& x, const T& y, const T& z) { first = new impl(x, new impl(y, last = new impl(z))); }
	void push_back(const T& t) 	{ last = last->next = new impl(t); } // dont call for empty list
	void push_front(const T& t) 	{ first = new impl(t, first); }
	T& back() 			{ return last->t; }
	T& front() 			{ return first->t; }
	const T& back() const		{ return last->t; }
	const T& front() const		{ return first->t; }

	class iterator {
		friend class list<T>;
		sp<impl> curr, prev = 0;
	public:
		iterator(sp<impl> c = 0)		: curr(c) {}
		T& operator*()				{ return curr->t; }
		T* operator->()				{ return &curr->t; }
		const T& operator*() const		{ return curr->t; }
		const T* operator->() const		{ return &curr->t; }
		iterator& operator++() 			{ return prev = curr, curr = curr->next, *this; }
		bool operator==(const iterator& i) const{ return curr == i.curr; }
		bool operator!=(const iterator& i) const{ return curr != i.curr; }
		operator bool() const			{ return curr; }
	};
	iterator begin() const { return iterator(first); }
	const iterator& end() const { static iterator z; return z; }
	T& erase(iterator& i) {
		T& r = *i;
		if (i.prev) i.prev->next = i.curr->next;
		else i.curr = i.curr->next;
		return r;
	}
	void splice(list<T>& l) { last->next = l.first; }
};

const size_t chunk = 4;
template<typename T, bool ispod = std::is_pod<T>::value>
class vector {
protected:
	T* a;
	const size_t szchunk = chunk * sizeof(T);
	size_t n, c;
public:
	vector() : a(0),n(0),c(0) {}
	vector(const vector<T>& t) : a(0),n(0),c(0) { copyfrom(t); }
	vector<T>& operator=(const vector<T>& t) { copyfrom(t); return *this; }
	bool operator==(const vector<T>& t) const { return n == t.n && !memcmp(a, t.a, n * sizeof(T)); }

	typedef T* iterator;
	T operator[](size_t k) const { return a[k]; }
	size_t size() const	{ return n; }
	T* begin() const	{ return a; }
	T* end() const		{ return a ? &a[n] : 0; }
	void clear()		{ n = c = 0; if (!a) return; free(a), a = 0; }
	bool empty() const	{ return !a; }
	~vector()		{ clear(); }
	iterator back()		{ return n ? &a[n-1] : 0; }
	T& push_back(const T& t) {
		if (!(n % chunk))
			a = (T*)realloc(a, szchunk * ++c);
		if (ispod) a[n] = t;
	       	else new (&a[n])(T)(t);
		return a[n++];
	}
protected:	
	void copyfrom(const vector<T>& t) {
		clear();
		if (!(n = t.n)) return; 
		memcpy(a = (T*)malloc(t.c * szchunk), t.a, (c = t.c) * szchunk);
	}
};

template<typename K, typename V>
struct mapelem {
	K first;
	V second;
	mapelem(){}
	mapelem(const mapelem& m) : first(m.first), second(m.second) {}
	mapelem(const K& _k, const V& _v) : first(_k), second(_v) {} 
};

template<typename K, typename V, bool sorted = false>
struct map : public vector<mapelem<K, V> > {
	typedef mapelem<K, V> vtype;
	typedef vector<mapelem<K, V> > base;
public:
	map(const vector<vtype>& t) { copyfrom(t); }
	map() : base() {}
	map(const map<K, V>& _s) : base()      	{ base::copyfrom(_s); }
	map<K, V>& operator=(const map<K, V>& m){ base::copyfrom(m); return *this; }
	V& operator[](const K& k)		{ mapelem<K, V>* z = find(k); return z ? z->second : set(k, V()); }
	V& set(const K& k, const V& v = V()) {
		vtype* z = find(k);
		if (z) { return z->second = v; }
//		if (sorted) qsort(base::a, base::n, sizeof(vtype), compare);
		return base::push_back(vtype(k, v)).second;
	}
	typedef vtype* iterator;
	iterator find(const K& k) const {
		if (!base::n) return 0;
//		if (!sorted) {
			for (vtype& x : *this) if (k == x.first) return &x;
			return 0;
//		}
//		vtype v(k, V());
//		vtype* z = (vtype*)bsearch(&v, base::a, base::n, sizeof(vtype), compare);
//		return z;
	}
private:
	static int compare(const void* x, const void* y) { return ((vtype*)x)->first - ((vtype*)y)->first; }
};
