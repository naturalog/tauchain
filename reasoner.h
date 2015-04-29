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
ostream& operator<< ( ostream& o, const rulelist& l );
ostream& operator<< ( ostream& o, const predlist& l );

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
typedef map<int, list<pair<rule*, ground_t>>> evidence_t;
typedef map<int, list<rule*>> cases_t;

struct frame {
	rule* rul = 0;
	uint src = 0, ind = 0;
	frame* parent = 0;
	subst substitution;
	ground_t ground;
	frame& init ( class reasoner* r, const frame& f );
	frame& init ( class reasoner*, rule* _r = 0, uint _src = 0, uint _ind = 0, frame* p = 0, subst _s = subst(), ground_t _g = ground_t() );
};

class reasoner {
	friend struct frame;
	predicate *predicates = new predicate[max_predicates];
	rule *rules = new rule[max_rules];
	frame *frames = new frame[max_frames];
	uint npredicates = 0, nrules = 0, nframes = 0;
	predicate* GND;
	int builtin ( predicate* p );
//	rulelist to_rulelist ( const ground_t& g );
	predicate* evaluate ( predicate& t, const subst& sub );
	bool unify ( predicate* s, const subst& ssub, predicate* d, subst& dsub, bool f );
	predlist to_predlist ( const ground_t& g );
	void evidence_found ( const frame& current_frame, evidence_t& evidence );
	frame* next_frame ( const frame& current_frame, ground_t& g );
	frame* match_cases ( frame& current_frame, predicate& t, cases_t& cases );
public:
	reasoner();
	~reasoner();
	evidence_t operator() ( rule* goal, int maxNumberOfSteps, cases_t& cases );
	evidence_t operator() ( const qdb& kb, const qlist& query );
	bool test_reasoner();
	void printkb();
	predicate* mkpred ( string s, const vector<predicate*>& v = vector<predicate*>() );
	rule* mkrule ( predicate* p = 0, const vector<predicate*>& v = vector<predicate*>() );
	predicate* triple ( const string& s, const string& p, const string& o );
	predicate* triple ( const jsonld::quad& q );
};

ostream& operator<< ( ostream& o, const subst& s );
ostream& operator<< ( ostream& o, const ground_t& s );
ostream& operator<< ( ostream& o, const evidence_t& e );
ostream& operator<< ( ostream& o, const cases_t& e );
ostream& operator<< ( ostream& o, const rule& r );
ostream& operator<< ( ostream& o, const predicate& p );
ostream& operator<< ( ostream& o, const frame& f );

#endif
