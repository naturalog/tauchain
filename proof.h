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

using namespace std;
using namespace jsonld;

const uint K1 = 1024, M1 = K1 * K1;
const uint max_predicates = M1, max_rules = M1, max_proofs = M1;

typedef /*boost::interprocess::*/vector<const struct predicate*> predlist;
typedef /*boost::interprocess::*/vector<const struct rule*> rulelist;
ostream& operator<< ( ostream& o, const rulelist& l );
ostream& operator<< ( ostream& o, const predlist& l );

typedef boost::interprocess::map<int, const class predicate*> subst;

struct predicate {
	int pred = 0;
	predlist args;
	predicate& init ( int _p = 0, predlist _args = predlist() );
	const predicate* evaluate ( const subst& sub ) const;
};

struct rule {
	const predicate* head = 0;
	predlist body;
	rule& init ( const predicate* h, predlist b = predlist() );
};

typedef boost::interprocess::list<pair<const rule*, subst>> ground_t;
typedef boost::interprocess::map<int, boost::interprocess::list<pair<const predicate*, ground_t>>> evidence_t;
typedef boost::interprocess::map<int, list<const rule*>> cases_t;

class proof {
public:
	static proof& find ( const rule* goal, const cases_t& cases);
	friend ostream& operator<< ( ostream& o, const proof& p );
private:
	const rule* rul = 0;
	uint ind = 0;
	const proof* parent = 0;
	subst sub;
	ground_t ground;
	int builtin ( const predicate* t );
	void process ( const cases_t& cases, evidence_t& evidence );
	static proof& init ( const proof& f );
	static proof& init ( const rule* _r = 0, uint _ind = 0, const proof* p = 0, subst _s = subst(), ground_t _g = ground_t() );
};

const predicate* unify ( const predicate& s, const subst& ssub, const predicate& d, subst& dsub, bool f );

extern predicate *predicates;
extern rule *rules;
extern proof *proofs;
extern uint npredicates, nrules, nproofs;

class reasoner {
	friend struct proof;
	predicate* GND;
	predlist to_predlist ( const ground_t& g );

	deque<proof*> init ( const rule* goal );

	predicate* mkpred ( string s, const predlist& v = predlist() );
	rule* mkrule ( const predicate* p = 0, const predlist& v = predlist() );
	const predicate* triple ( const string& s, const string& p, const string& o );
	const predicate* triple ( const jsonld::quad& q );
public:
	reasoner();
	~reasoner();
	evidence_t prove ( const rule* goal, int, const cases_t& cases ) { proof::find(goal, cases); return evidence_t();  }
	evidence_t prove ( const qdb& kb, const qlist& query );
	bool test_reasoner();
	void printkb();
};

ostream& operator<< ( ostream& o, const subst& s );
ostream& operator<< ( ostream& o, const ground_t& s );
ostream& operator<< ( ostream& o, const evidence_t& e );
ostream& operator<< ( ostream& o, const cases_t& e );
ostream& operator<< ( ostream& o, const rule& r );
ostream& operator<< ( ostream& o, const predicate& p );

#endif
