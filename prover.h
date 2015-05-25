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
typedef uint termid;

namespace prover {
struct session;
}

class reasoner {
public:
	bool prove ( qdb kb, qlist query );
	bool test_reasoner();
};

namespace prover {
bool equals(const class term* x, const class term* y);
void printterm(const class term& p);
class term {
//	static std::map<int, std::map<termid, std::map<termid, termid>>> terms; // pso
	static std::vector<term> terms;
public:
	term(int _p, termid _s, termid _o, pnode n) : p(_p), s(_s), o(_o), node(n) {}
	const int p;
	const termid s, o;
	pnode node;
	static const term& get(termid id) { return terms[id - 1]; }
	static termid make(string p, termid s = 0, termid o = 0, pnode node = 0) { return make(dict.set(p), s, o, node); }
	static termid make(int p, termid s = 0, termid o = 0, pnode node = 0) {
		if (!p) throw 0;
		terms.emplace_back(p,s,o,node);
		return terms.size();
	}
};
typedef std::vector<termid> termset;
string format(const class rule& r);

class rule {
	termset _body;
public:
	termid p = 0;
	const termset& body() const { return _body; }
	void body(const termset& b) { _body = b; }
	void body(termid t) { _body.push_back(t); }
	bool operator==(const rule& x) const { return p == x.p && body() == x.body(); }
	rule(){}
	rule(termid _p) : p(_p) {}
};
/*
struct cmp { bool operator()(const rule* x, const rule* y) {
	if (!x != !y) return !x;
	if (!x) return false;
	return format(*x) < format(*y);} 
};
*/
//typedef std::map<int, std::set<const rule* /*, cmp*/>> ruleset;
//typedef std::set<const rule*> ruleset;
struct ruleset {
	std::vector<termid> head;
	std::vector<termset> body;
};

typedef std::map<int, termid> subst;
typedef std::list<std::pair<int, subst>> ground;
typedef std::map<int, std::list<std::pair<termid, ground>>> evidence;

struct proof {
	uint rul, last;
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
void printterm(termid p);

const uint MEM = 1024 * 8 * 256;
const uint max_terms = MEM;
const uint max_termsets = MEM;
const uint max_rules = MEM;
const uint max_proofs = MEM;
extern rule* rules;
extern proof* proofs;
extern uint nterms, ntermsets, nrules, nproofs;
}
