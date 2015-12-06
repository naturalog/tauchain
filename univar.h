#include "rdf.h"
#include "misc.h"

class yprover {
public:

	typedef std::list<qdb> results_t;
	results_t results;
	long steps_;
	long unifys_;

	yprover(qdb qkb);
	~yprover();
	void thatsAllFolks(int nresults);
	void query(const qdb &goal);
};
