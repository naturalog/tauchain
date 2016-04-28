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
struct rule { const term *h, **b; unsigned sz; };

struct n2vm {
	inline bool isvar(const term &t) const { return t.p > 0; }
	inline bool isvar(const term *t) const { return t->p > 0; }
	inline bool islist(const term &t) const { return !t.p; }

	term* add_term(int p, const vector<const term*>& args = vector<const term*>());
	void add_rules(rule *rs, unsigned sz);
	void commit();
	bool add_constraint(hrule, hprem, hrule, const term*, const term*);
	bool add_constraint(auto&, hrule, const term&, const term&);
	bool add_constraint(auto&, hrule, int, const term&);
	bool tick();
	n2vm() : kb(*new kb_t) {}
private:
	typedef vector<vector<map<hrule, map<int, const term*>>>> kb_t;
	kb_t &kb;
	unsigned query;
	struct frame {
		hrule r;
		hprem p;
		frame *next, *up;
		frame(hrule r, hprem p, frame *up) :
			r(r), p(p), up(up) {}
	} *last = 0, *curr = 0;

	bool add_lists(auto&, hrule, const term&, const term&);
	hrule mutate(hrule r, auto env);
	void resolve(frame *f);
};
