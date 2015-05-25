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
public:
	bool prove ( qdb kb, qlist query );
	bool test_reasoner();
};

namespace prover {
bool equals(const class term* x, const class term* y);
void printterm(const class term& p);
class term {
//	static std::map<int, std::map<const term*, std::map<const term*, const term*>>> terms; // pso
	static std::list<term*> terms;
public:
	term(int _p, const term* _s, const term* _o, pnode n) : p(_p), s(_s), o(_o), node(n) {}
	const int p;
	const term *s, *o;
	pnode node;
	static const term* make(string p, const term* s = 0, const term* o = 0, pnode node = 0) { return make(dict.set(p), s, o, node); }
	static const term* make(int p, const term* s = 0, const term* o = 0, pnode node = 0) {
/*		auto& tt = terms[p][s];
		auto it = tt.find(o);
		if (it != tt.end()) return it->second;*/
//		for (auto y : terms[p]) {
//			auto x = y.second[s];
//			for (auto x : y.second)
//				if (x.second->p == p && !s == !x.second->s && !o == !x.second->o
//					&& (!s || equals(s, x.second->s))
//					&& (!o || equals(o, x.second->o)))
//						return x.second;
//		}
		term* t = new term(p,s,o, node);
		terms.push_front(t);
		return t;
//		terms[p][s][o] = t;
//		auto r = terms.emplace(p, s, o, node);
//		t->node = node;
//		dout << "term::make added new term: "; printterm(*t); dout << std::endl;
//		static uint n = 0;
//		if (r.second) if (++n % 10000 == 0) std::cerr << n << std::endl;
//		return &*r.first;
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

const uint MEM = 1024 * 8 * 256;
const uint max_terms = MEM;
const uint max_termsets = MEM;
const uint max_rules = MEM;
const uint max_proofs = MEM;
extern rule* rules;
extern proof* proofs;
extern uint nterms, ntermsets, nrules, nproofs;
}
