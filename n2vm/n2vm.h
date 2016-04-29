#ifndef __n2vm_h__
#define __n2vm_h__

#include <map>
#include <list>
//#include <vector>
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <forward_list>
#include "containers.h"

typedef int hrule;
typedef int hprem;

struct term {
	const bool isvar;
	wchar_t* p;
	const term *const* args;
	const unsigned sz;
	term(const wchar_t* p);
	term(const vector<const term*> &_args);
	term(const term& t) : isvar(t.isvar), p(t.p ? wcsdup(t.p) : 0), args(t.args), sz(t.sz) { }
	term(term&& t) : isvar(t.isvar), p(t.p), args(t.args), sz(t.sz) {
		t.p = 0;
	}
	~term();
	operator std::wstring() const;
	int cmp(const term& t) const {
		if (isvar != t.isvar) return isvar ? 1 : -1;
		if (sz != t.sz) return sz > t.sz ? 1 : -1;
		if (!sz) return (p && t.p) ? wcscmp(p, t.p) : p ? 1 : -1;
		int r;
		for (unsigned n = 0; n < sz; ++n)
			if ((r = args[n]->cmp(*t.args[n])))
				return r;
		return 0;
	}
};

struct rule {
	const term *h;
	const term *const* b;
	unsigned sz;
	operator std::wstring() const;

	rule(const term *h, const vector<const term*> &b = vector<const term*>());
	rule(const term *h, const term *t);
};

inline bool isvar(const term &t) { return t.isvar; }
inline bool isvar(const term *t) { return t->isvar; }
inline bool islist(const term &t) { return !t.p; }

struct n2vm {
//	n2vm() : kb(*new kb_t) {}
	n2vm(std::wistream& is, bool fin = true);
	const term* add_term(const term& t);
	void add_rules(rule *rs, unsigned sz);
	bool tick();
	void printkb();
	typedef std::map<const term*, const term*> sub;
	typedef std::map<hrule, sub> iprem;
	typedef std::list<iprem*> irule;

private:
	typedef vector<irule*> kb_t;
	typedef std::forward_list<const wchar_t*> varmap;
	rule *orig;
	unsigned origsz;
	struct frame {
		hrule r;
		irule::iterator p;
		frame *next, *up;
		frame(hrule r, irule::iterator p, frame *up = 0) :
			r(r), p(p), next(0), up(up) {}
	} *last = 0, *curr = 0;

	kb_t &kb;
	std::map<unsigned, varmap> vars;
	unsigned query;
	bool add_lists(iprem&, hrule, const term&, const term&);
	hrule mutate(hrule r, const sub&);
	bool add_constraint(iprem&, hrule, const term*, const term*);
	bool add_constraint(iprem&, hrule, const term&, const term&);
	bool add_constraint(iprem&, hrule, const wchar_t*, const term&);
	bool mutate(iprem &dn, const sub &e, auto m, const sub &env);
	void getvarmap(const term& t, varmap& v);
};

template<typename T>
const T* vec_to_nt_arr(const vector<T>& v) {
	auto res = new T[v.size() + 1];
	((T*)memcpy(res, &v[0], v.size() * sizeof(T)))[v.size()] = 0;
	return res;
}

template<typename T>
T* vec_to_nt_arr(const vector<const T>& v) {
	auto res = new T[v.size() + 1];
	((T*)memcpy(res, &v[0], v.size() * sizeof(T)))[v.size()] = 0;
	return res;
}

template<typename T>
vector<T> singleton_vector(const T& t) {
	vector<T> v;
	v.push_back(t);
	return v;
}
/*
template<typename T>
T* vec_to_nt_arr(const vector<T>& v) {
	T* res = new T[v.size() + 1];
	((T*)memcpy(res, &v[0], v.size() * sizeof(T)))[v.size()] = 0;
	return res;
}
*/
#endif
