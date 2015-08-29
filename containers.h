
#define vec_t vector<T,ispod>
#define map_t map<K, V, ispod>
#define vec_templ template<typename T, bool ispod>
#define map_tmpl template<typename K, typename V, bool ispod>

const size_t chunk = 4;

template<typename T, bool ispod /*= std::is_pod<T>::value*/>
struct vector {
protected:
	T* a;
	static const size_t szchunk;
	size_t n, c;
public:
	vector();
	vector(const vector<T, ispod>& t);
	vector<T, ispod>& operator=(const vector<T, ispod>& t);
	~vector();

	typedef T* iterator;
	T operator[](size_t k) const;
	size_t size() const;
	T* begin() const;
	T* end() const;
	void clear();
	void clear1(); // dont free mem
	bool empty() const;
	iterator back();
	T& push_back(const T& t);

	struct coro {
		bool state;
		iterator i, e;
		const vector<T, ispod>* t;
		coro(const vector<T, ispod>& _t) : state(false), i(0), e(0), t(&_t) {}
		bool operator()() {
			switch (state) {
			case false: 
				i = t->begin(), e = t->end();
				while (i != e) {
					return state = true;
			case true: 	++i;
				}
				return state = false;
			}
		}
	} m;

protected:	
	void copyfrom(const vector<T, ispod>& t);
};

//template<typename T, bool ispod /*= std::is_pod<T>::value*/>
/*struct tree {
	T elem;
	vector<tree<T, ispod>, ispod> next;
	typedef T* iterator;
	struct coro {
		bool state;
		iterator i, e;
		coro() : s(false), i(0), e(0) {}
		bool operator()() {
			yield i = elem;
			next::coro c;
			while (c())
				yield i = c.i
			switch (state) {
			case false: 
				i = begin(), e = end;
				while (i != e) {
					return state = true;
			case true: 	++i;
				}
				return state = false;
			}
		}
	};
};*/

template<typename K, typename V>
struct mapelem {
	K first;
	V second;
	mapelem(){}
	mapelem(const mapelem& m) : first(m.first), second(m.second) {}
	mapelem(const K& _k, const V& _v) : first(_k), second(_v) {} 
};

template<typename K, typename V, bool ispod>
struct map : public vector<mapelem<K, V>, ispod > {
	typedef mapelem<K, V> vtype;
	typedef vector<mapelem<K, V>, ispod > base;
	typedef map<K, V, ispod > _this;
public:
	map(const base& t);
	map();
	map(const _this& _s);
	_this& operator=(const _this& m);
	V& operator[](const K& k);
	V& set(const K& k, const V& v);
	typedef vtype* iterator;
	iterator find(const K& k) const;
private:
	static int compare(const void* x, const void* y) { return ((vtype*)x)->first - ((vtype*)y)->first; }
};

class bidict {
	map<resid, string, false> ip;
	map<string, resid, false> pi;
public:
	resid set(string v);
	string operator[](resid k);
	resid operator[](string v);
};

vec_templ vec_t::vector() : a(0),n(0),c(0),m(*this) {}
map_tmpl map<K, V, ispod>::map() : base() {}
vec_templ vec_t::vector(const vector<T, ispod>& t) : a(0),n(0),c(0),m(*this) { copyfrom(t); }
vec_templ vec_t& vector<T, ispod>::operator=(const vector<T, ispod>& t) { copyfrom(t); return *this; }
vec_templ T vec_t::operator[](size_t k) const 		{ return a[k]; }
vec_templ size_t vec_t::size() const			{ return n; }
vec_templ T* vec_t::begin() const			{ return a; }
vec_templ T* vec_t::end() const				{ return n ? &a[n] : 0; }
vec_templ void vec_t::clear()				{ n = c = 0; if (!a) return; free(a); a = 0; }
vec_templ void vec_t::clear1() 				{ n = c = 0; }
vec_templ bool vec_t::empty() const			{ return !n; }
vec_templ vec_t::~vector()				{ if(a)free(a); }
vec_templ T* vec_t::back()				{ return n ? &a[n-1] : 0; }
map_tmpl map<K, V, ispod>::map(const base& t)		{ copyfrom(t); }
map_tmpl map<K, V, ispod>::map(const _this& _s) : base(){ base::copyfrom(_s); }

map_tmpl map<K, V, ispod>& map<K, V, ispod>::operator=(const _this& m)	{ base::copyfrom(m); return *this; }
map_tmpl V& map<K, V, ispod>::operator[](const K& k){ mapelem<K, V>* z = find(k); return z ? z->second : set(k, V()); }

vec_templ T& vec_t::push_back(const T& t) {
	if (!(n % chunk))
		a = (T*)realloc(a, szchunk * ++c);
	if (ispod) a[n] = t;
	else new (&a[n])(T)(t);
	return a[n++];
}
vec_templ void vec_t::copyfrom(const vector<T, ispod>& t) {
	if (!(n = t.n)) { c = 0; return; }
	memcpy(a = (T*)realloc(a, t.c * szchunk), t.a, (c = t.c) * szchunk);
}

template<typename T, bool ispod /*= std::is_pod<T>::value*/>
const size_t vector<T, ispod>::szchunk = chunk * sizeof(T);

map_tmpl V& map<K, V, ispod>::set(const K& k, const V& v) {
	vtype* z = find(k);
	if (z) { return z->second = v; }
	return base::push_back(vtype(k, v)).second;
//	qsort(base::a, base::n, sizeof(vtype), compare);
}
map_tmpl mapelem<K, V>* map<K, V, ispod>::find(const K& k) const {
	if (!base::n) return 0;
	iterator e = base::end();
	for (vtype* x = base::begin(); x != e; ++x) if (x->first == k) return x;
	return 0;
//	vtype v(k, V()); vtype* z = (vtype*)bsearch(&v, base::a, base::n, sizeof(vtype), compare); return z;
}
//map_tmpl int map<K, V, ispod>::compare(const void* x, const void* y) { return ((vtype*)x)->first - ((vtype*)y)->first; }

class nqparser {
private:
	wchar_t *t;
	const wchar_t *s;
	int pos;
public:
	nqparser();
	~nqparser();

	bool readcurly(vector<term*, true>& ts);
	term* readlist();
	term* readiri();
	term* readbnode();
	term* readvar();
	term* readlit();
	term* readany(bool lit = true);
	void addhead(vector<term*, true>& ts, term* t);
	vector<term*, true> operator()(const wchar_t* _s);
	static vector<term*, true> readterms (std::wistream& is);
};

resid bidict::set ( string v ) {
	if (!v.size()) throw "bidict::set called with a node containing null value";
	map<string, resid, false>::iterator it = pi.find ( v );
	if ( it ) return it->second;
	resid k = pi.size() + 1;
	if ( v[0] == L'?' ) k = -k;
	pi[v] = k, ip[k] = v;
	return k;
}

void trim(string& s) {
	string::iterator i = s.begin();
	while (iswspace(*i)) {
		s.erase(i);
		i = s.begin();
	}
	size_t n = s.size();
	if (n) {
		while (iswspace(s[--n]));
		s = s.substr(0, ++n);
	}
}
string wstrim(string s) { trim(s); return s; }
string bidict::operator[](resid k) { return ip[k]; }
resid bidict::operator[](string v) { return set(v); }

