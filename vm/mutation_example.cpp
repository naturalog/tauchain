template<typename T>
struct parr<T> { // persistent array
	const T& get(unsigned);
	parr<T>* set(unsigned, const T&);
	void reroot();
};

struct puf : private parr<int> { // persistent dsds of ints
	int find(int);
	puf* unio(int, int);
};
struct premise;
struct rule : public parr<premise*> { };
struct kb_t : public parr<rule*> { };
kb_t kb;

bool merge(puf &dst, const puf& src);

struct premise {
	unsigned rl; // rule number of the matching head
	// doubly linked list:
	premise *next, *prev; // since we may have many matching heads
	puf *conds; // conditions for matching this head
	rule* operator()() { // the reasoning core
		rule &r = kb.get(rl); // the rule to match to this premise
		for (premise* p : r)
			if (!merge(*p->conds, *conds))
				p->prev = p->next; // TODO: GC
				// TODO: handle edge cases
			// we're done! target rule's premise is mutated
	}
};
