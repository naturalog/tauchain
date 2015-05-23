/*
    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

#include <deque>
#include <climits>
#include "rdf.h"
#include "misc.h"
#include "parsers.h"
#include <functional>

//using namespace std;
using namespace jsonld;

const uint K1 = 1024, M1 = K1 * K1;

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
bool equals(const class term* x, const class term* y);
class term {
	static std::set<const term*> terms;
	term()						: p(0), s(0), o(0) {}
	term(int _p, const term* _s, const term* _o)	: p(_p), s(_s), o(_o) {}
//	term(const term& x)				: p(x.p), s(x.s), o(x.o) {}
public:
	const int p;
	const term *s, *o;
	static const term* make(int p, const term* s = 0, const term* o = 0) {
		if (!p) throw 0;
		for (auto x : terms) 
			if (x->p == p && equals(s, x->s) && equals(o, x->o))
				return x;
		term* t = new term(p,s,o);
		terms.insert(t); 
		return t;
	}
//	term(const string& _p)				: term(dict.set(_p)) {}
//	term(const string& _p, term& _s, term& _o)	: term(dict.set(_p), _s, _o) {}
	operator bool() const { return p; }
	term& operator=(const term& x) { const_cast<int&>(p) = x.p; s = x.s; o = x.o;  return *this; }
	bool operator!=(const term& x) const { return !((*this) == x); }
	bool operator==(const term& x) const {
		if (p != x.p) return false;
		if (s) { if (!x.s || *s != *x.s) return false; } else if (x.s) return false;
		if (o) { if (!x.o || *o != *x.o) return false; } else if (x.o) return false;
		return true;
	}
};

typedef std::set<const term*> termset;

class rule {
	termset _body;
public:
	const term* p = 0;
	const termset& body() const { return _body; }
	void body(const termset& b) { _body = b; }
	void body(const term* t) { _body.insert(t); }
	bool operator==(const rule& x) const { return p == x.p && body() == x.body(); }
//	rule& operator=(const rule& x) { p = x.p; body = x.body; return *this; }
//	rule(const rule& x) : p(x.p), body(x.body) {}
	rule(){}
	rule(const term* _p) : p(_p) {}
};

typedef std::map<int, std::list<const rule*>> ruleset;
typedef std::map<int, const term*> subst;
typedef std::list<std::pair<const rule*, subst>> ground;
typedef std::map<int, std::list<std::pair<const term*, ground>>> evidence;

struct proof {
	const rule* rul;
	termset::const_iterator last;
	proof* prev;
	std::shared_ptr<subst> s;
	ground g;
	std::function<int(struct session&)> callback;
	proof() : rul(0), prev(0), s(std::make_shared<subst>()) {}
};

typedef std::deque<proof*> queue;

struct session {
	ruleset kb;
	termset goal;
	evidence e;
	queue q;
	proof* p;
};

void initmem();
//term* evaluate(term* p, subst* s); 			// evaluates a term given a subst
//bool unify(term* s, subst* ssub, term* d, subst** dsub, bool f); // unification
void prove(session& ss);				// backchaining
//bool equals(term* x, term* y);				// deep-compare terms
void printterm(const term& p);				// print a term
void prints(const subst& s);				// print a subst
void printl(const termset& l);
void printg(const ground& g);			// print a ground struct
void printe(const evidence& e);			// print evidence
void printr(const rule& r);				// print rule
void printrs(const ruleset& rs);			// print a ruleset
void printq(const queue& q);				// print a proof queue
void printp(proof* p);
//bool is_implication(int p);				// identifies an implication resource
//void trim(char *s);					// trim strings from both ends using isspace

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
