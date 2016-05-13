#ifndef __n2vm_h__
#define __n2vm_h__

#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <list>
#include <forward_list>
#include "containers.h"

typedef int hrule;
typedef int hprem;

typedef const struct term cterm;
struct term {
	const bool isvar;
	wchar_t* p;
	cterm *const* args;
	const unsigned sz;
	term(const wchar_t* p);
	term(const vector<cterm*> &_args);
	term(cterm& t);
	term(term&& t);
	~term();
	operator std::wstring() const;
	int cmp(cterm& t) const;
};

struct rule {
	cterm *h;
	cterm *const* b;
	unsigned sz;
	operator std::wstring() const;

	rule(cterm *h, const vector<cterm*> &b = vector<cterm*>());
	rule(cterm *h, cterm *t);
};

inline bool isvar(cterm &t) { return t.isvar; }
inline bool isvar(cterm *t) { return t->isvar; }
inline bool islist(cterm &t) { return !t.p; }

struct n2vm {
	n2vm(std::wistream& is, bool fin = true);
	cterm* add_term(cterm& t);
	void add_rules(rule *rs, unsigned sz);
	bool tick();
	void printkb();
	typedef std::map<cterm*, cterm*> sub;
	typedef std::map<hrule, sub> iprem;
	typedef std::list<iprem*> irule;

private:
	typedef vector<irule*> kb_t;
	typedef std::set<term*> varmap;
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
	bool add_lists(iprem&, hrule, cterm&, cterm&);
	hrule mutate(hrule r, const sub&);
	bool add_constraint(iprem&, hrule, cterm*, cterm*);
	bool add_constraint(iprem&, hrule, cterm&, cterm&);
	bool add_constraint(iprem&, hrule, const wchar_t*, cterm&);
	bool mutate(iprem &dn, const sub &e, auto m, const sub &env);
	void getvarmap(cterm& t, varmap& v);
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
#endif
