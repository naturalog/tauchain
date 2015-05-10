#include <list>
#include <deque>
#include <map>
#include <forward_list>
using namespace std;

namespace prover {
struct term {
	int p;
	term *s, *o;
	term() : p(0), s(0), o(0) {}
};

typedef /*forward_*/list<term*> termset;

/*
struct termset {
	term* p;
	termset* next;
	termset() : p(0), next(0) {}
};
*/
struct rule {
	term* p;
	termset body;
	rule() : p(0), body(0) {}
};
/*
struct ruleset {
	rule* r;
	ruleset* next;
};*/
typedef map<int, list<rule*>> ruleset;
typedef map<int, term*> subst;
typedef list<pair<rule*, subst>> ground;
/*
struct subst {
	int p;
	term* pr;
	subst* next;
};
*/
/*
struct ground {
	rule* r;
	subst* s;
	ground* next;
};

struct evidence_item {
	term* p;
	ground* g;
	evidence_item* next;
};
*/
typedef map<int, list<pair<term*, ground>>> evidence;

/*
struct evidence {
	int p;
	evidence_item* item;
	evidence* next;
};
*/
struct proof {
	rule* rul;
	termset::iterator last;
	proof* prev;
	subst s;
	ground g;
	proof() : rul(0), last(0), prev(0) {}
};
typedef deque<proof*> queue;

/*
struct queue {
	proof* p;
	queue* prev;
	queue* next;
};

struct dict {
	char* s;
	int n;
	dict* next;
};
*/
struct session {
	ruleset rkb;
	termset kb, goal;
	evidence e;
};

void initmem();
//void pushg(ground** g, rule* r, subst* s);		// push rule and subst pair into ground
//void pushe(evidence** _e, term* t, ground* g); 		// push evidence item into evidence
//subst* clone(subst* s); 				// deep-copy a subst
//void pushq(queue**, proof* p); 				// push a proof to the back of a proofs queue
//void unshift(queue** _q, proof* p); 			// push a proof to the front of the queue
//void setsub(subst** s, int p, term* pr); 		// pushes a substitution
term* evaluate(term* p, subst* s); 			// evaluates a term given a subst
bool unify(term* s, subst* ssub, term* d, subst** dsub, bool f); // unification
void prove(session* ss);				// backchaining
bool equals(term* x, term* y);				// deep-compare terms
//void pushp(termset** l, term& p);			// push a term to a termset
//void pushr(ruleset** _rs, term* s, term* o);		// push subject and object of s=>o into a ruleset
void printterm(const term& p);				// print a term
//term* term(int s, int p, int o);			// create an s/p/o term
void prints(const subst& s);				// print a subst
void printl(const termset& l);
void printg(const ground& g);			// print a ground struct
void printe(const evidence& e);			// print evidence
//void printei(const evidence_item& ei);		// print single evidence step
void printr(const rule& r);				// print rule
//void printss(const session& s);				// print session memory
void printp(const proof& p);				// print a proof element
void printrs(const ruleset& rs);			// print a ruleset
void printq(const queue& q);				// print a proof queue
//ruleset* findruleset(ruleset* rs, int p);		// find in a ruleset according to term
//int pushw( const char* s);			// push a string into a dictionary and get its int id
//const char* dgetw( int n);			// decrypt string from dictionary given id
//bool dgetwn( const char* s, int* n);		// returns the id of a given string in a dict
//void readr(ruleset** _r);				// read a rule from the user and stores it in a ruleset
//void menu(session* ss);					// run menu
//void printcmd(const char* line, session*);		// handle print commands
bool is_implication(int p);				// identifies an implication resource
//void pushr(ruleset** _rs, rule* r);
void trim(char *s);					// trim strings from both ends using isspace

extern uint nterms;
extern term* terms;
extern uint ntermsets;
extern termset* termsets;
extern uint nrules;
extern rule* rules;
//extern uint nsubsts;
//extern subst* substs;
//extern uint nrulesets;
//extern ruleset* rulesets;
//extern uint ngrounds;
//extern ground* grounds;
extern uint nproofs;
extern proof* proofs;
//extern uint nqueues;
//extern queue* queues;
extern uint nevidence_items;
//extern evidence_item* evidence_items;
//extern uint nevidences;
//extern evidence* evidences;
//extern uint ndicts;
//extern dict* dicts;
}
