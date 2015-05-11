#include <list>
#include <deque>
#include <map>
#include <forward_list>
#include <array>
using namespace std;

namespace prover {
struct term {
	int p;
	term *s, *o;
	term() : p(0), s(0), o(0) {}
};

typedef forward_list<term*> termset;

struct rule {
	term* p;
	termset body;
	rule() : p(0), body(0) {}
};

typedef map<int, list<rule*>> ruleset;
typedef map<int, term*> subst;
typedef list<pair<rule*, subst>> ground;
typedef map<int, list<pair<term*, ground>>> evidence;

struct proof {
	rule* rul;
	termset::iterator last;
	proof* prev;
	subst s;
	ground g;
	proof() : rul(0), last(0), prev(0) {}
};
typedef deque<proof*> queue;

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

const uint MEM = 1024 * 1024;
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
