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
void printterm(const class term& p);
class term {
//	static std::set<const term*> terms;
	static std::map<int, std::map<const term*, std::map<const term*, const term*>>> terms; // pso
	term(int _p, const term* _s, const term* _o)	: p(_p), s(_s), o(_o) {}
public:
	const int p;
	const term *s, *o;
	static const term* make(string p, const term* s = 0, const term* o = 0) { return make(dict.set(p), s, o); }
	static const term* make(int p, const term* s = 0, const term* o = 0) {
		for (auto y : terms[p])
			for (auto x : y.second)
				if (x.second->p == p && equals(s, x.second->s) && equals(o, x.second->o)) {
//				dout << "term::make found existing term: "; printterm(*x); dout << " where requested term: "; printterm({p,s,o}); dout << std::endl;
					return x.second;
				}
		term* t = new term(p,s,o);
//		terms.insert(t);
		terms[p][s][o] = t;
//		dout << "term::make added new term: "; printterm(*t); dout << std::endl;
		return t;
	}
	operator bool() const { return p; }
	bool operator!=(const term& x) const { return !((*this) == x); }
	bool operator==(const term& x) const {
		if (p != x.p) return false;
		if (s) { if (!x.s || *s != *x.s) return false; } else if (x.s) return false;
		if (o) { if (!x.o || *o != *x.o) return false; } else if (x.o) return false;
		return true;
	}
};
typedef std::set<const term*> termset;
string format(const class rule& r);

class rule {
	termset _body;
public:
	const term* p = 0;
	const termset& body() const { return _body; }
	void body(const termset& b) { _body = b; }
	void body(const term* t) { _body.insert(t); }
	bool operator==(const rule& x) const { return p == x.p && body() == x.body(); }
	rule(){}
	rule(const term* _p) : p(_p) {}
};
struct cmp { bool operator()(const rule* x, const rule* y) {
	if (!x != !y) return !x;
	if (!x) return false;
	return format(*x) < format(*y);} 
};

typedef std::map<int, std::set<const rule*, cmp>> ruleset;
typedef std::map<int, const term*> subst;
typedef std::list<std::pair<const rule*, subst>> ground;
typedef std::map<int, std::list<std::pair<const term*, ground>>> evidence;

struct proof {
	const rule* rul;
	termset::const_iterator last;
	proof *prev;
	subst s;
	ground g;
	std::function<int(struct session&)> callback;
	proof() : rul(0), prev(0) {}
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
void prove(session& ss);
void printterm(const term& p);
void prints(const subst& s);
void printl(const termset& l);
void printg(const ground& g);
void printe(const evidence& e);
void printr(const rule& r);
void printrs(const ruleset& rs);
void printq(const queue& q);
void printp(proof* p);

const uint MEM = 1024 * 256;
const uint max_terms = MEM;
const uint max_termsets = MEM;
const uint max_rules = MEM;
const uint max_proofs = MEM;
extern rule* rules;
extern proof* proofs;
extern uint nterms, ntermsets, nrules, nproofs;
}
