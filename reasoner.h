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
#include "parsers.h"
#include "misc.h"

using namespace std;

const uint K1 = 1024, M1 = K1 * K1;
const uint max_predicates = M1, max_rules = M1, max_frames = M1;

typedef vector<struct predicate*> predlist;
typedef vector<struct rule*> rulelist;
ostream& operator<< ( ostream&, const predlist& );
ostream& operator<< ( ostream&, const rulelist& );

template<typename T> string print ( T t ) {
	stringstream ss;
	ss << t;
	return ss.str();
}


struct predicate {
	int pred = 0;
	predlist args;
	predicate& init ( int _p = 0, predlist _args = predlist() );
};

struct rule {
	predicate* head = 0;
	predlist body;
	rule& init ( predicate* h, predlist b = predlist() );
};

typedef map<int, predicate*> subst;
typedef list<pair<rule*, subst>> ground_t;
typedef map<int, forward_list<pair<rule*, subst>>> evidence_t;
typedef map<int, vector<rule*>> cases_t;

struct frame {
	rule* rul = 0;
	uint src = 0, ind = 0;
	frame* parent = 0;
	subst substitution;
	ground_t ground;
	frame& init ( const frame& f );
	frame& init ( rule* _r = 0, uint _src = 0, uint _ind = 0, frame* p = 0, subst _s = subst(), ground_t _g = ground_t() );
};

ostream& operator<< ( ostream& o, const subst& s );
ostream& operator<< ( ostream& o, const ground_t& s );
ostream& operator<< ( ostream& o, const evidence_t& e );
ostream& operator<< ( ostream& o, const cases_t& e );
ostream& operator<< ( ostream& o, const rule& r );
ostream& operator<< ( ostream& o, const predicate& p );
ostream& operator<< ( ostream& o, const frame& f );
ostream& operator<< ( ostream& o, const rulelist& l );
ostream& operator<< ( ostream& o, const predlist& l );

void printkb();
int builtin ( predicate* p );
rulelist to_rulelist ( const ground_t& g );
predicate* evaluate ( predicate& t, const subst& sub );
bool unify ( predicate* s, const subst& ssub, predicate* d, subst& dsub, bool f );
predlist to_predlist ( const ground_t& g );
evidence_t prove ( rule* goal, int maxNumberOfSteps, cases_t& cases );
evidence_t prove ( const qdb& kb, const qlist& query );
bool test_reasoner();

#endif
