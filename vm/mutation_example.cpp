#include <vector>
#include <set>
using namespace std;

template<typename T>
struct parr { // persistent array
	const T& get(unsigned);
	parr<T>* set(unsigned, const T&);
	void reroot();
};
// persistent dsds of ints
struct puf : private pair<parr<int>, parr<unsigned>> {
	int find(int);
	puf* unio(int, int); // returns 0 for const1=const2
};

bool ISVAR(int x) { return x > 0; }

struct match {
	unsigned rl;
	puf *conds;
	set<int> vars;
};

typedef vector<vector<match*>*> rule;

match* mutate(match &m, puf& c) {
	int r;
	match &res = *new match;
	res.rl = m.rl;
	res.conds = m.conds;
	for (int v : m.vars)
		if (!res.conds->unio(v, r = c.find(v))) {
			delete &res;
			return 0;
		} else if (ISVAR(r))
			res.vars.insert(v);
	return &res;
}

rule* mutate(rule &r, puf &c) {
	rule &res = *new rule;
	match *t;
	for (vector<match*> *p : r) {
		auto &v = *new vector<match*>;
		for (match *m : *p)
			if ((t = mutate(*m, c)))
				v.push_back(t);
		if (v.empty()) {
			delete &res;
			delete &v;
			return 0;
		}
		res.push_back(&v);
	}
	return &res;
}

struct kb_t : public vector<rule*> { } *kb;
