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
	premise *next; // since we may have many matching heads
	puf *conds; // conditions for matching this head
	rule* operator()() { // the reasoning core
		rule &r = kb.get(rl); // the rule to match to this premise
		for (premise* p : r) { // mutate its premises
			puf *tmp = p->conds; // in case we fail
			if (!merge(*p->conds, *conds))
				p->conds = tmp; // TODO: GC
			// we're done! target rule's premise is mutated
		}
	}
};
