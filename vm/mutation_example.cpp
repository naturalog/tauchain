// Positive integers are vars. Negatives are list length up
// to max list length in kb, integers smaller than -max represent consts.
// NOTE: Assuming de-bruijn. this is crucial!
#include "cfpds.h"
#include <vector>
#include <utility>
#include <map>
using namespace std;

typedef pair<int, int>		constraint;
typedef vector<pair<int, uf*>> 	dna;
typedef vector<class rule*>	kb_t;

class rule : public vector<dna> {
	rule(const rule& r, const constraint& c);
public:	
	rule(int nvars) : nvars(nvars) {}
	const int nvars;
	map<constraint, rule*> m;

	static rule* mutate(rule &r, const constraint &c);
};

rule::rule(const rule& r, const constraint& c) : nvars(r.nvars) {
	for (dna d : r) { // go over all body items
		dna _d;
		for (auto p : d) // go over all body's head matches
			_d.push_back(make_pair(p.first, unio(p.second, c.second, c.second))); 
		push_back(_d);
	}
}

rule* rule::mutate(rule &r, const constraint &c) {
	auto i = r.m.find(c);
	if (i != r.m.end()) return i->second;
	r.m[c] = new rule(r, c);
}
