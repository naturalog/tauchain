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
const uint max_predicates = M1, max_proofs = M1;

extern boost::interprocess::list<thread*> threads;
extern boost::interprocess::vector<class proof*> proofs;
typedef /*boost::interprocess::*/vector<const struct predicate*> predlist;
ostream& operator<< ( ostream& o, const predlist& l );

typedef boost::interprocess::map<int, const class predicate*> subst;

struct predicate {
	int pred = 0;
	predlist args;
	predlist body;
	predicate& init ( int _p, predlist _args, predlist _body );
	const predicate* evaluate ( const subst& sub ) const;
	string dot() const;
};
ostream& operator<< ( ostream& o, const predicate& p );

typedef boost::interprocess::list<pair<const predicate*, subst>> ground_t;
typedef boost::interprocess::map<int, boost::interprocess::list<pair<const predicate*, ground_t>>> evidence_t;
typedef boost::interprocess::map<int, list<const predicate*>> cases_t;

class proof {
public:
	static proof& find ( const predicate* goal, const cases_t& cases);
	friend ostream& operator<< ( ostream& o, const proof& p );
private:
	const predicate* rul = 0;
	uint ind = 0;
	const proof* parent = 0;
	subst sub;
	ground_t ground;
	int builtin ( const predicate* t );
	bool process ( const cases_t& cases, evidence_t& evidence );
	static proof& init ( const proof& f );
	static proof& init ( const predicate* _r = 0, uint _ind = 0, const proof* p = 0, subst _s = subst(), ground_t _g = ground_t() );
public:	
	string dot() const {
		stringstream r;
		if (!rul && !parent) return {};
		r << '\"' << *rul << '\"';
		if (parent) r << " -> \"" << *parent->rul << '\"';
		else r << " [shape=box]";
		r << ';' << endl;
		return r.str();
	}
};

const predicate* unify ( const predicate& s, const subst& ssub, const predicate& d, subst& dsub, bool f );

extern predicate *predicates;
extern predicate *predicates;
//extern proof *proofs;
extern uint npredicates, npredicates, nproofs;

class reasoner {
	friend struct proof;
	predicate* GND;
	predlist to_predlist ( const ground_t& g );

	deque<proof*> init ( const predicate* goal );

	predicate* mkpred ( string s, const predlist& v = predlist() );
	predicate* triple ( const string& s, const string& p, const string& o );
	predicate* triple ( const jsonld::quad& q );
public:
	reasoner();
	~reasoner();
	evidence_t prove ( const predicate* goal, int, const cases_t& cases ) { proof::find(goal, cases); return evidence_t();  }
	evidence_t prove ( const qdb& kb, const qlist& query );
	bool test_reasoner();
	void printkb();
};

ostream& operator<< ( ostream& o, const subst& s );
ostream& operator<< ( ostream& o, const ground_t& s );
ostream& operator<< ( ostream& o, const evidence_t& e );
ostream& operator<< ( ostream& o, const cases_t& e );

#endif
