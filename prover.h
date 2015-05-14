/*
    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

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
#include <thread>
#include "parsers.h"
#include "rdf.h"
#include <thread>
#include "misc.h"

//using namespace std;
using namespace jsonld;

const uint K1 = 1024, M1 = K1 * K1;
const uint max_predicates = 1, max_rules = 1, max_proofs = 1;

namespace prover {
struct session;
}

class reasoner {
	void addrules(string s, string p, string o, prover::session& ss, const qdb& kb);
public:
	bool prove ( qdb kb, qlist query );
	bool test_reasoner();
};

namespace prover {
struct term {
	int p;
	term *s, *o;
	term() : p(0), s(0), o(0) {}
	term(int _p, term* _s = 0, term* _o = 0) : p(_p), s(_s), o(_o) {}
	term(const string& _p, term* _s = 0, term* _o = 0) : term(dict.set(_p), _s, _o) {}
};

typedef std::forward_list<term*> termset;

struct rule {
	term* p;
	termset body;
	rule() : p(0), body(0) {}
};

typedef std::map<int, std::list<rule*>> ruleset;
typedef std::map<int, term*> subst;
typedef std::list<std::pair<rule*, subst>> ground;
typedef std::map<int, std::list<std::pair<term*, ground>>> evidence;

struct proof {
	rule* rul;
	termset::iterator last;
	proof* prev;
	subst s;
	ground g;
	proof() : rul(0), last(0), prev(0) {}
};
typedef std::deque<proof*> queue;

struct session {
	ruleset rkb;
	termset kb, goal;
	evidence e;
};

void initmem();
term* evaluate(term* p, subst* s); 			// evaluates a term given a subst
bool unify(term* s, subst* ssub, term* d, subst** dsub, bool f); // unification
void prove(session* ss);				// backchaining
bool equals(term* x, term* y);				// deep-compare terms
void printterm(const term& p);				// print a term
void prints(const subst& s);				// print a subst
void printl(const termset& l);
void printg(const ground& g);			// print a ground struct
void printe(const evidence& e);			// print evidence
void printr(const rule& r);				// print rule
void printp(const proof& p);				// print a proof element
void printrs(const ruleset& rs);			// print a ruleset
void printq(const queue& q);				// print a proof queue
bool is_implication(int p);				// identifies an implication resource
void trim(char *s);					// trim strings from both ends using isspace

const uint MEM = 1024 * 256;
const uint max_terms = MEM;
const uint max_termsets = MEM;
const uint max_rules = MEM;
const uint max_proofs = MEM;
extern term* terms;
extern termset* termsets;
extern rule* rules;
extern proof* proofs;
extern uint nterms, ntermsets, nrules, nproofs;
}
