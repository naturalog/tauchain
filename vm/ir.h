#ifndef __IR_H__
#define __IR_H__
#include <set>
#include "containers.h"

namespace ir {

struct match {
	unsigned rl;
	struct uf *conds;
	std::set<int> vars;
	match(unsigned rl, struct uf *conds) : rl(rl), conds(conds) {}
};
typedef vector<match*> premise;

struct mutation {
	struct rule *r;
	match *m;
	unsigned b;
};

struct rule : private vector<premise*> {
	match *mutator;
	rule(match *mutator = 0) : mutator(mutator) {}
	mutation mutate(unsigned);
	vector<rule*> mutations;
	rule *prev; // wrt mutation tree
	~rule();
};

struct frame {
	mutation *m;
	frame *prev, *next;
	static frame* last;
	void run() {

	}
};

struct kb_t : public vector<rule*> { };
extern kb_t *kb;
}
#endif
