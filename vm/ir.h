#ifndef __IR_H__
#define __IR_H__
#include <vector>
#include <set>
using namespace std;

namespace ir {
#include "data.h"
struct match {
	unsigned rl;
	puf *conds;
	set<int> vars;
	match(unsigned rl, puf *conds) : rl(rl), conds(conds) {}
};

struct rule : private vector<vector<match*>*> {
	rule* mutate(puf &c);
	vector<rule*> mutations;
	rule *prev; // in the mutation tree
	~rule();
};

struct kb_t : public vector<rule*> { };
}
#endif
