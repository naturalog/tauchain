#include <map>
#include <set>
#include <vector>
#include <cassert>
#include <cstring>

using namespace std;

typedef int hrule;
typedef int hprem;
typedef int hvar;
typedef int hlist;
typedef int henv;

struct term {
	int p;
	vector<term*> args;
	term(){}
	term(int p, vector<term*> args) : p(p), args(args) {}
};

struct n2vm {
	bool isvar(const term &t) const { return t.p > 0; }
	bool islist(const term &t) const { return !t.p; }

	void add_term(int p, vector<term*> args) { terms.emplace(p, args); }
	bool add_constraint(hrule, hprem, hrule, term&, term&);
	bool tick();
private:
	struct term_cmp {
		int operator()(const term& x, const term& y) const {
			return memcmp(&x, &y, sizeof(term));
		}
	};

	vector<vector<map<hrule, map<term, term, term_cmp>>>> kb;
	set<term, term_cmp> terms;
	struct frame {
		hrule r;
		hprem p;
		frame *next, *up;
		frame(hrule r, hprem p, frame *up) :
			r(r), p(p), up(up) {}
	};
	frame *last = 0, *curr = 0;

	bool add_constraint(auto&, hrule, const term&, const term&);
	bool add_var_constraint(auto&, hrule, const term&, const term&);
	bool add_lists(auto&, hrule, const term&, const term&);
	hrule mutate(hrule r, auto env);
	bool resolve(frame *f);
};
