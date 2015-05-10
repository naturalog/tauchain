/*
    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

#ifndef __REASONER_H__
#define __REASONER_H__

#include <forward_list>
#include <deque>
#include <climits>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <map>
#include <list>
#include <vector>
#include <cstdio>
#include "misc.h"
#include "rdf.h"
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <thread>

using namespace std;
using namespace jsonld;

const uint K1 = 1024, M1 = K1 * K1;
const uint max_predicates = 1, max_rules = 1, max_proofs = 1;

extern boost::interprocess::list<thread*> threads;
typedef vector<shared_ptr<const struct predicate>> predlist;
struct predicate {
	int pred = 0;
	predlist args;
	predicate( int _p = 0, predlist _args = predlist() ) : pred(_p), args(_args) { }
};
ostream& operator<< ( ostream& o, const predlist& l );
ostream& operator<< ( ostream& o, const predicate& p );

namespace prover {
struct session;
}

class reasoner {
	friend struct proof;
	shared_ptr<predicate> mkpred ( string s, const predlist& v = predlist() );
	shared_ptr<const predicate> triple ( const string& s, const string& p, const string& o );
	shared_ptr<const predicate> triple ( const jsonld::quad& q );
	void addrules(string s, string p, string o, prover::session& ss, const qdb& kb);
public:
	bool prove ( qdb kb, qlist query );
	bool test_reasoner();
};
#endif
