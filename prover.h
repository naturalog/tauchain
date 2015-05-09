namespace prover {
struct term {
	int p;
	term *s, *o;
};

struct termset {
	term* p;
	termset* next;
};

struct rule {
	term* p;
	termset* body;
};

struct ruleset {
	rule* r;
	ruleset* next;
};

struct subst {
	int p;
	term* pr;
	subst* next;
};

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

struct evidence {
	int p;
	evidence_item* item;
	evidence* next;
};

struct proof {
	rule* rul;
	termset* last;
	proof* prev;
	subst* s;
	ground* g;
};

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

struct session {
	queue* q;
	ruleset *rkb;
	termset *kb, *goal;
	dict* d;
	evidence* e;
};

void initmem();
void pushg(ground** g, rule* r, subst* s);		// push rule and subst pair into ground
void pushe(evidence** _e, term* t, ground* g); 		// push evidence item into evidence
subst* clone(subst* s); 				// deep-copy a subst
void pushq(queue**, proof* p); 				// push a proof to the back of a proofs queue
void unshift(queue** _q, proof* p); 			// push a proof to the front of the queue
void setsub(subst** s, int p, term* pr); 		// pushes a substitution
term* evaluate(term* p, subst* s); 			// evaluates a term given a subst
bool unify(term* s, subst* ssub, term* d, subst** dsub, bool f); // unification
void prove(session* ss);				// backchaining
bool equals(term* x, term* y);				// deep-compare terms
void pushp(termset** l, term* p);			// push a term to a termset
void pushr(ruleset** _rs, term* s, term* o);		// push subject and object of s=>o into a ruleset
void printterm(term* p, dict* d);			// print a term
//term* term(int s, int p, int o);			// create an s/p/o term
void prints(subst* s, dict* d);				// print a subst
void printl(termset* l, dict* d); 			// print a termset
void printg(ground* g, dict* d);			// print a ground struct
void printe(evidence* e, dict* d);			// print evidence
void printei(evidence_item* ei, dict* d);		// print single evidence step
void printr(rule* r, dict* d);				// print rule
void printss(session* s);				// print session memory
void printp(proof* p, dict* d);				// print a proof element
void printrs(ruleset* rs, dict* d);			// print a ruleset
void printq(queue* q, dict* d);				// print a proof queue
ruleset* findruleset(ruleset* rs, int p);		// find in a ruleset according to term
int pushw(dict** _d, const char* s);			// push a string into a dictionary and get its int id
const char* dgetw(dict* d, int n);			// decrypt string from dictionary given id
bool dgetwn(dict* d, const char* s, int* n);		// returns the id of a given string in a dict
void readr(ruleset** _r);				// read a rule from the user and stores it in a ruleset
void menu(session* ss);					// run menu
void printcmd(const char* line, session*);		// handle print commands
bool is_implication(int p);				// identifies an implication resource
void pushr(ruleset** _rs, rule* r);
void trim(char *s);					// trim strings from both ends using isspace

extern uint nterms;
extern term* terms;
extern uint ntermsets;
extern termset* termsets;
extern uint nrules;
extern rule* rules;
extern uint nsubsts;
extern subst* substs;
extern uint nrulesets;
extern ruleset* rulesets;
extern uint ngrounds;
extern ground* grounds;
extern uint nproofs;
extern proof* proofs;
extern uint nqueues;
extern queue* queues;
extern uint nevidence_items;
extern evidence_item* evidence_items;
extern uint nevidences;
extern evidence* evidences;
extern uint ndicts;
extern dict* dicts;
}
