#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <string>
#include <set>

typedef struct frame* pframe;
const size_t chunk = 4;
const wchar_t endl[] = L"\r\n";
typedef std::wstring string;
typedef int resid;
struct term;

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

template<typename T, bool ispod /*= std::is_pod<T>::value*/>
class vector {
protected:
	T* a;
	static const size_t szchunk;
	size_t n, c;
public:
	vector();
	vector(const vector<T, ispod>& t);
	vector<T, ispod>& operator=(const vector<T, ispod>& t);

	typedef T* iterator;
	T operator[](size_t k) const;
	size_t size() const;
	T* begin() const;
	T* end() const;
	void clear();
	void clear1(); // dont free mem
	bool empty() const;
	~vector();
	iterator back();
	T& push_back(const T& t);
protected:	
	void copyfrom(const vector<T, ispod>& t);
};

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

typedef map<term*, term*, true> subs;
typedef vector<term*, true> termset;
string format(const term* t, bool body = false);
string format(const termset& t, int dep = 0);
string format(const subs& s);
void trim(string& s);
string wstrim(string s);

class bidict {
	map<resid, string, false> ip;
	map<string, resid, false> pi;
public:
	resid set(string v);
	string operator[](resid k);
	resid operator[](string v);
};

term* mkterm(termset& kb, termset& query);
term* mkterm(resid p);
term* mkterm(resid p, const termset& args);
typedef term* (*fevaluate)(term&);
typedef term* (*fevaluates)(term&, const subs&);
typedef bool (*funify)(term& s, term& d, subs& dsub);
typedef bool (*funify_ep)(term& s, term& d, const subs& dsub);

term* evvar(term&);
term* evnoargs(term&);
term* ev(term&);
term* evs(term&,const subs&);
term* evvars(term&,const subs&);
term* evnoargss(term&,const subs&);
bool u1(term& s, term& d, subs& dsub);
bool u2(term& s, term& d, const subs& dsub);
bool u3(term& s, term& d, subs& dsub);
bool u4(term& s, term& d, const subs& dsub);

struct term {
	term();
	term(termset& kb, termset& query);
	term(resid _p, const termset& _args = termset());
	const resid p;
	const termset args;
	const size_t szargs;
	fevaluate evaluate;
	fevaluates evaluates;
	funify unify;
	funify_ep unify_ep;
	termset matches, body;

	term* addbody(const termset& t);
	void trymatch(termset& heads);
	void _unify(pframe f, pframe& lastp);
};

struct tcmp {
	bool operator()(term* _x, term* _y) const {
		term &x = *_x, &y = *_y;
		if (x.szargs != y.szargs) return x.szargs < y.szargs;
		if (x.p != y.p) return x.p < y.p;
		for (termset::iterator i = x.args.begin(), e = x.args.end(), j = y.args.begin(); i != e; ++i, ++j) 
			if ((*i)->p != (*j)->p) return (*i)->p < (*j)->p;
		return false;
	}
};
std::set<term*, tcmp> terms;

const size_t tchunk = 8192, nch = tchunk / sizeof(term);

class nqparser {
private:
	wchar_t *t;
	const wchar_t *s;
	int pos;
public:
	nqparser();
	~nqparser();

	bool readcurly(termset& ts);
	term* readlist();
	term* readiri();
	term* readbnode();
	term* readvar();
	term* readlit();
	term* readany(bool lit = true);
	void addhead(termset& ts, term* t);
	termset operator()(const wchar_t* _s);
};

termset readterms (std::wistream& is);

typedef vector<mapelem<term*, subs>, false > ground;
typedef map<resid, vector<mapelem<term*, ground>, false >, false > evidence;

struct frame {
	term* rule;
	termset::iterator b;
	pframe prev, creator, next;
	subs s;
	int ref;
	ground g() const; // calculate the ground
	frame(pframe c, frame& p, const subs& _s);
	frame(pframe c, term* r, termset::iterator* l, pframe p);
	frame(pframe c, term* r, termset::iterator* l, pframe p, const subs&  _s);
	void decref();
};

const size_t szf = sizeof(frame), fchunk = 32, szc = szf * fchunk;
size_t fn = 0, fc = 0;
frame* fbuf = 0;//(frame*)malloc(szc);

frame* mkframe(pframe c, frame& p, const subs& _s) {
	if (!(fn%fchunk)) fbuf = (frame*)realloc(fbuf, szc * ++fc);
	new (&((frame*)fbuf)[fn])(frame)(c,p,_s);
	return &((frame*)fbuf)[fn++];
}

frame* mkframe(pframe c, term* r, termset::iterator* l, pframe p) {
	if (!(fn%fchunk)) fbuf = (frame*)realloc(fbuf, szc * ++fc);
	new (&((frame*)fbuf)[fn])(frame)(c,r,l,p);
	return &((frame*)fbuf)[fn++];
}

frame* mkframe(pframe c, term* r, termset::iterator* l, pframe p, const subs& _s) {
	if (!(fn%fchunk)) fbuf = (frame*)realloc(fbuf, szc * ++fc);
	new (&((frame*)fbuf)[fn])(frame)(c,r,l,p,_s);
	return &((frame*)fbuf)[fn++];
}



void prove(pframe _p, pframe& lastp);
