#include <map>
#include <vector>
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <forward_list>

namespace n2vm {

typedef int hrule;
typedef int hprem;

struct term {
	const int p;
	term** args;
	const unsigned sz;
	term(int p) : p(p), args(0), sz(0) {}
	term(const std::vector<term*> &_args) 
		: p(0), args(new term*[_args.size() + 1]), sz(_args.size()) {
		int n = 0;
		for (auto x : _args) args[n++] = x;
		args[n] = 0;
	}
	operator std::string() const;
};

struct rule { const term *h, **b; unsigned sz; operator std::string() const; };

inline bool isvar(const term &t) { return t.p < 0; }
inline bool isvar(const term *t) { return t->p < 0; }
inline bool islist(const term &t) { return !t.p; }

struct n2vm {
	n2vm() : kb(*new kb_t) {}
	term* add_term(int p, const std::vector<term*>& args = std::vector<term*>());
	void add_rules(rule *rs, unsigned sz);
	bool tick();

private:
	typedef std::map<int, const term*> sub;
	typedef std::map<hrule, sub> iprem;
	typedef std::vector<iprem*> irule;
	typedef std::vector<irule*> kb_t;
	typedef std::forward_list<int> varmap;
	rule *orig;
	unsigned origsz;
	struct frame {
		hrule r;
		hprem p;
		frame *next, *up;
		frame(hrule r, hprem p, frame *up) :
			r(r), p(p), next(0), up(up) {}
	} *last = 0, *curr = 0;

	kb_t &kb;
	std::map<unsigned, varmap> vars;
	unsigned query;
	bool add_lists(iprem&, hrule, const term&, const term&);
	hrule mutate(hrule r, const sub&);
	bool add_constraint(hrule, hprem, hrule, const term*, const term*);
	bool add_constraint(iprem&, hrule, const term&, const term&);
	bool add_constraint(iprem&, hrule, int, const term&);
	bool mutate(iprem &dn, const sub &e, auto m, const sub &env);
	void getvarmap(const term& t, varmap& v);
	void printkb();
};
}
