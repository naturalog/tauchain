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
	typedef map<int, const term*> sub;
	typedef map<hrule, sub> iprem;
	typedef vector<iprem*> irule;
	typedef vector<irule*> kb_t;
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
	unsigned query;
	bool add_lists(auto&, hrule, const term&, const term&);
	hrule mutate(hrule r, auto env);
	bool add_constraint(hrule, hprem, hrule, const term*, const term*);
	bool add_constraint(iprem&, hrule, const term&, const term&);
	bool add_constraint(iprem&, hrule, int, const term&);
	void printkb();
};
