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

	void add_term(int p, const vector<const term*> &args) {
		terms.emplace(new term(p, args));
	}
	bool add_constraint(hrule, hprem, hrule, term*, term*);
	bool add_constraint(auto&, hrule, const term&, const term&);
	bool add_constraint(auto&, hrule, int, const term&);
	bool tick();
	struct term_cmp {
		static const term_cmp tc;
		int operator()(const term& x, const term& y) const {
			if (x.p > y.p) return 1;
			if (x.p < y.p) return -1;
			int r;
			auto ix = x.args.begin(), ex = x.args.end(), iy = y.args.begin();
			for (; ix != ex;  ++ix, ++iy)
				if ((r = tc(**ix, **iy)))
					return r;
			return 0;
		}
		int operator()(const term *x, const term *y) const {
			if (x == y) return 0;
			return tc(*x, *y);
		}
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
