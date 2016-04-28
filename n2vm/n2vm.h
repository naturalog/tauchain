#include <map>
#include <set>
#include <vector>
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>

using namespace std;

typedef int hrule;
typedef int hprem;

struct term {
	const int p;
	const vector<const term*> args;
	term(int p, const vector<const term*> &args) : p(p), args(args) { }
	operator string() const;
};

struct rule { const term *h, **b; unsigned sz; operator string() const; };

inline bool isvar(const term &t) { return t.p < 0; }
inline bool isvar(const term *t) { return t->p < 0; }
inline bool islist(const term &t) { return !t.p; }

struct n2vm {
	n2vm() : kb(*new kb_t) {}
	term* add_term(int p, const vector<const term*>& args = vector<const term*>());
	void add_rules(rule *rs, unsigned sz);
	bool tick();

private:
	typedef vector<vector<map<hrule, map<int, const term*>>>> kb_t;
	rule *orig;
	struct frame {
		hrule r;
		hprem p;
		frame *next, *up;
		frame(hrule r, hprem p, frame *up) :
			r(r), p(p), up(up) {}
	} *last = 0, *curr = 0;

	kb_t &kb;
	unsigned query;
	bool add_lists(auto&, hrule, const term&, const term&);
	hrule mutate(hrule r, auto env);
	bool add_constraint(hrule, hprem, hrule, const term*, const term*);
	bool add_constraint(auto&, hrule, const term&, const term&);
	bool add_constraint(auto&, hrule, int, const term&);
	void printkb();
};
