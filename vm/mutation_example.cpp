// Positive integers are vars. Negatives are list length up
// to max list length in kb, integers smaller than -max represent consts.
// NOTE: Assuming de-bruijn. this is crucial!
#include "cfpds.h"
#include <vector>
#include <utility>
using namespace std;
struct dna : public vector<pair<int, uf*>> { };
struct rule : public vector<dna> {
	rule(int nvars) : nvars(nvars) {}
	const int nvars;
	// mutation without validation
	rule* mutate( uf *c ) {
		rule *r = new rule(nvars);
		for (dna d : *this) { // go over all body items
			dna _d;
			for (auto p : d) { // go over all body's head matches
				uf *u;
				// go over all vars
				for (int n = 0; n < nvars; ++n) 
					// merge uf
					u = unio(p.second, n, find(c, n)); 
				// push dna to new rule
				_d.push_back(make_pair(p.first, u)); 
			}
			r->push_back(_d);
		}
		return r;
	}
};
struct kb : public vector<rule> {};
