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

struct match {
	unsigned rl;
	puf *conds;
	set<int> vars;
};

struct rule : private vector<vector<match*>*> {
	rule* mutate(puf &c);
	vector<rule*> mutations;
	rule *prev; // in the mutation tree
	pair<rule*, unsigned> up; // up the proof tree, rule and body item id
};

rule* rule::mutate(puf &c) {
	rule &res = *new rule;
	match *t;
	for (vector<match*> *p : *this) { // per body item
		auto &v = *new vector<match*>;
		for (match *mt : *p) { // per matching head
			int r;
			match &m = *new match;
			m.rl = mt->rl;
			m.conds = mt->conds;
			for (int x : mt->vars)
				if (!m.conds->unio(x, r = c.find(x))) {
					delete &m;
					goto stop;
				} else if (r > 0) // rep is var
					m.vars.insert(x);
			v.push_back(&m);
		stop:
		}
		if (v.empty()) { // body item can't ever be satisfied
			delete &res;
			delete &v;
			return 0;
		}
		res.push_back(&v);
	}
	return mutations.push_back(&res), res.prev = this, &res;
}

struct kb_t : public vector<rule*> { } *kb;
