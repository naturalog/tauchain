#include <map>
#include <set>
#include <vector>
#include <cassert>
#include <cstring>

using namespace std;

typedef int hrule;
typedef int hprem;

struct term {
	const int p;
	const vector<const term*> args;
	term(int p, const vector<const term*> &args) : p(p), args(args) { }
};

struct n2vm {
	bool isvar(const term &t) const { return t.p > 0; }
	bool isvar(const term *t) const { return t->p > 0; }
	bool islist(const term &t) const { return !t.p; }

	term* add_term(int p, const auto& args = vector<const term*>());
	bool add_constraint(hrule, hprem, hrule, term*, term*);
	bool add_constraint(auto&, hrule, const term&, const term&);
	bool add_constraint(auto&, hrule, int, const term&);
	bool tick();

	struct term_cmp {
		int operator()(const term *x, const term *y) const;
	};
private:
	vector<vector<map<hrule, map<int, const term*>>>> kb;
	set<term*, term_cmp> terms;
	struct frame {
		hrule r;
		hprem p;
		frame *next, *up;
		frame(hrule r, hprem p, frame *up) :
			r(r), p(p), up(up) {}
	};
	frame *last = 0, *curr = 0;

	bool add_lists(auto&, hrule, const term&, const term&);
	hrule mutate(hrule r, auto env);
	bool resolve(frame *f);
};
