#include <map>
#include <vector>

using namespace std;

typedef int hrule;
typedef int hprem;
typedef int hvar;
typedef int hlist;
typedef int henv;
typedef int hterm;

struct n2vm {
	bool isvar(hterm);
	bool islist(hterm);
	hlist list(hterm);
	hterm next(hlist&);

	bool add_constraint(hrule r, hprem p, hrule h, hterm x, hterm y);
	bool tick();
private:
	vector<vector<map<hrule, map<hvar, hterm>>>> kb;
	struct frame {
		hrule r;
		hprem p;
		frame *next, *up;
		frame(hrule r, hprem p, frame *up) :
			r(r), p(p), up(up) {}
	};
	frame *last = 0, *curr = 0;

	bool add_constraint(auto &p, hrule h, hterm x, hterm y);
	bool add_var_constraint(auto &p, hrule h, hterm x, hterm y);
	bool add_lists(auto &p, hrule h, hlist x, hlist y);
	hrule mutate(hrule r, auto env);
	bool resolve(frame *f);
};
