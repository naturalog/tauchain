
#include "prover.h" // namespace "old"

class yprover {
public:

	typedef std::list<old::qdb> results_t;
	results_t results;
	long steps_;
	long unifys_;

	yprover(old::qdb qkb, bool check_consistency = false);
	~yprover();
	void thatsAllFolks(int nresults);
	void query(const old::qdb &goal);
};
