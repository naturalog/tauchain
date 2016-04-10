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

struct mutation {
	struct rule *r;
	match *m;
};

struct rule : private vector<vector<match*>*> {
	mutation mutate(match&);
	vector<rule*> mutations;
	rule *prev; // wrt mutation tree
	~rule();
};

struct frame {
	mutation *m;
	unsigned b;
	frame *prev, *next;
	static frame* last;
};


struct kb_t : public vector<rule*> { };
extern kb_t *kb;
}
#endif
