
#include "prover.h" // namespace "old"

class yprover {
public:

	typedef std::list<old::qdb> results_t;
	results_t results;

	yprover(old::qdb qkb, bool check_consistency = false);

	void query(const old::qdb &goal);
};
