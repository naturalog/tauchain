#ifndef __IR_H__
#define __IR_H__
#include <set>
#include "containers.h"

struct match {
	unsigned rl;
	struct uf *conds;
	std::set<int> vars;
	match(unsigned rl, struct uf *conds) : rl(rl), conds(conds) {}
};

namespace ir {
struct rule : private vector<vector<match*>*> {
	rule* mutate(struct uf &c);
	vector<rule*> mutations;
	rule *prev; // in the mutation tree
	~rule();
};

struct kb_t : public vector<rule*> { };

extern kb_t *kb;

}
#endif
