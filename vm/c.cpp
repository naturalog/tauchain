#include <string>
#include <cstring>
#include <iostream>
#include <functional>
#include <tuple>
#include <exception>
#include <sstream>
#include <cassert>
#include <map>
using std::function;
using std::wostream;
using std::wstring;
using std::stringstream;
using std::endl;
using std::wcin;
using std::tuple;
using std::make_tuple;
using std::runtime_error;
#define FOR(x, y) for (auto x = 0, __e##__LINE__ = y; x < __e##__LINE__; ++x)
wostream &dout = std::wcout;

#define THROW(x, y)                                                            \
  {                                                                            \
    stringstream s;                                                            \
    s << x << ' ' << y;                                                        \
    throw runtime_error(s.str());                                              \
  }

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
		//		vtype* z = (vtype*)bsearch(&v, base::a, base::n, sizeof(vtype),
		// compare);
		//		return z;
	}

private:
	static int compare(const void *x, const void *y) {
		return ((vtype *)x)->first - ((vtype *)y)->first;
	}
};

vector<struct atom> atoms;
struct atom {   // iri, var, or list
	wchar_t type; // the only member used in runtime
	union {
		int *args; // list items, last item must be null.
		const wchar_t *val;
	};
//	inline unsigned id() const { return this - atoms[0]; }
	atom(const wchar_t *v) : type(*v), val(wcsdup(v)) { dout << val << endl; }
	atom(vector<int> _args) : type(L'.'), args(new int[_args.size() + 1]) {
		int n = 0;
		for (auto x : _args) args[n++] = x;
		args[n] = 0;
	}
	~atom() {
		if (args && type == L'.') delete[] args;
		if (val) free((wchar_t *)val);
	}
};

struct triple {
	int r[3]; // spo
	triple(int s, int p, int o) { r[0] = s, r[1] = p, r[2] = o; }
	triple(const triple &t) : triple(t.r[0], t.r[1], t.r[2]) {}
};

typedef map<int, int> sub;

struct premise {
	const triple *t;
	map<int, sub> down;
	//	sub up;
	premise(const triple *t) : t(t) {}
};
typedef vector<premise> body_t;
struct rule {
	const triple *head;
	sub ep;
	body_t body;
	rule(const triple *h, const body_t &b = body_t()) :
		head(h), body(b) { assert(h); }
	rule(const triple *h, const triple *b) : head(h) {
		assert(!h);
		body.push_back(premise(b));
	}
	rule(const rule &r) : head(r.head), body(r.body) {}
	//
	sub up;
	map<int, sub> right;
};
vector<rule> rules;

wostream &operator<<(wostream &, const atom &);
wostream &operator<<(wostream &, const triple &t);
wostream &operator<<(wostream &, const struct vm &);
wostream &operator<<(wostream &, const rule &);
wostream &operator<<(wostream &, const premise &);

map<const atom *, int> dict;
int mkatom(const wstring &v) {
	static std::map<wstring, int> r;
	auto i = r.find(v);
	if (i != r.end()) return i->second;
	int id = atoms.size();
	atoms.push_back(new atom(v.c_str()));
	dict.set(atoms[id - 1], id);
	if (v[0] != L'?') id = -id;
	r.emplace(v, id);
	return id;
}

int mkatom(const vector<int> &v) {
	struct cmp {
		bool operator()(const vector<int> &x, const vector<int> &y) const {
			int s1 = x.size(), s2 = y.size();
			if (s1 != s2) return s1 < s2;
			return memcmp(&x[0], &y[0], sizeof(int) * s1) < 0;
		}
	};
	static std::map<vector<int>, int, cmp> r;
	auto i = r.find(v);
	if (i != r.end()) return i->second;
	int id = atoms.size();
	atoms.push_back(new atom(v));
	r.emplace(v, -id);
	dict.set(atoms[id - 1], -id);
	return -id;
}
const triple *mktriple(int s, int p, int o) {
	static map<tuple<int, int, int>, const triple *> spo;
	auto t = make_tuple(s, p, o);
	auto i = spo.find(t);
	return i ? i->second : spo.set(t, new triple(s, p, o));
}

struct din_t {
	wchar_t ch;
	bool f = false, done, in_query = false;
	// due to wcin.peek() not working
	wchar_t peek() { return f ? ch : ((f = true), (wcin >> ch), ch); }
	wchar_t get() { return f ? ((f = false), ch) : ((wcin >> ch), ch); }
	wchar_t get(wchar_t c) {
		wchar_t r = get();
		if (c != r) THROW(L"expected: ", c);
	}
	bool good() { return wcin.good(); }
	void skip() { while (good() && iswspace(peek())) get(); }
	wstring getline() {
		wstring s;
		f = false, std::getline(wcin, s);
		return s;
	}
	din_t() { wcin >> std::noskipws; } 
	wstring &trim(wstring &s) {
		static wstring::iterator i = s.begin();
		while (iswspace(*i)) s.erase(i), i = s.begin();
		size_t n = s.size();
		if (n) {
			while (iswspace(s[--n])) ;
			s = s.substr(0, ++n);
		}
		return s;
	}
	wstring edelims = L")}.";
	wstring till() {
		skip();
		static wchar_t buf[4096];
		static size_t pos;
		pos = 0;
		while (good() && edelims.find(peek()) == wstring::npos && !iswspace(peek()))
			buf[pos++] = get();
		buf[pos] = 0;
		return wcslen(buf) ? buf : 0;
	}
	int readlist() {
		get();
		vector<int> items;
		while (skip(), (good() && peek() != L')'))
			items.push_back(readany()), skip();
		return get(), skip(), mkatom(items);
	}
	int readany() {
		if (skip(), !good()) return 0;
		if (peek() == L'(') return readlist();
		wstring s = till();
		if (s == L"fin" && peek() == L'.') return get(), done = true, 0;
		return s.size() ? mkatom(s) : 0;
	}
	const triple *readtriple() {
		int s, p, o;
		if (skip(), !(s = readany()) || !(p = readany()) || !(o = readany()))
			return 0;
		const triple *r = mktriple(s, p, o);
		if (skip(), peek() == L'.') get(), skip();
		return r;
	}
	void readdoc(bool query) { // TODO: support prefixes
		const triple *t;
		done = false;
		while (good() && !done) {
			body_t body;
			switch (skip(), peek()) {
			case L'{':
				if (query) THROW("can't handle {} in query", "");
				get();
				while (good() && peek() != L'}')
					body.push_back(premise(readtriple()));
				get(), skip(), get(L'='), get(L'>'), skip(), get(L'{'), skip();
				if (peek() == L'}')
					rules.push_back(rule(0, body)), skip();
				else
					while (good() && peek() != L'}')
						rules.push_back(rule(readtriple(), body));
				get();
				break;
			case L'#':
				getline();
				break;
			default:
				if ((t = readtriple()))
					rules.push_back(query ? rule(0, t) : rule(t));
				else if (!done)
					THROW("parse error: triple expected", "");
			}
			if (done) return;
			if (skip(), peek() == L'.') get(), skip();
		}
	}
} din;

// output
wostream &operator<<(wostream &os, const premise &p) { return os << *p.t; }
wostream &operator<<(wostream &os, const rule &r) {
	os << L'{';
	for (auto t : r.body) os << t << ' ';
	return os << L"} => { ", (r.head ? os << *r.head : os << L""), os << " }.";
}
wostream &operator<<(wostream &os, const triple &t) {
	return os << *atoms[abs(t.r[0])] << ' ' << *atoms[abs(t.r[1])] << ' '
	       << *atoms[abs(t.r[2])] << L'.';
}
wostream &operator<<(wostream &os, const atom &r) {
	if (r.type != L'.') return r.val ? os << r.val : os;
	os << L'(';
	const int *a = r.args;
	while (*a) os << atoms[*a++] << L' ';
	return os << L')';
}

void print() {
	dout << "rules: " << rules.size() << endl;
	for (auto r : rules) dout << r << endl;
}

typedef const triple* ctriptr;

struct trip2 {
    ctriptr fst, snd;
    bool operator<(const trip2& x) const { return fst < x.fst && snd < x.snd; }
};

int main() {
	atoms.push_back(new atom(L"GND"));
	din.readdoc(false);
	print();
	din.readdoc(true);
	print();
	// now create M and unifiers
	map<trip2, sub> M;
	for (const rule& r : rules) {
		for (const premise& p : r.body)
			for (const rule& r1 : rules) {
				ctriptr h = r.head, b = p.t;
			}
	}
}
