#include "rdf.h"
#include "misc.h"

class yprover {
public:
	//Structure
	typedef std::list<qdb> results_t;
	
	results_t results;

	//Correspond to the global steps & unifys defined in univar.cpp
	//Measurements 
	long steps_;
	long unifys_;

	//Constructor/Destructor
	yprover(qdb qkb);
	~yprover();

	//Access	
	void query(qdb &goal);
	void cppout(qdb &goal);

	//
	void thatsAllFolks(int nresults);
};
