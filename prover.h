#ifndef __PROVER_H__
#define __PROVER_H__

/*
    Created on: Apr 28, 2015
        Author: Ohad Asor
*/
#include <chrono>
#include <deque>
#include <queue>
#include <climits>
#include "rdf.h"
#include "misc.h"
#include <future>
#include <functional>
#include <forward_list>
//#include <boost/interprocess/containers/map.hpp>
//#include <boost/interprocess/containers/set.hpp>
//#include <boost/interprocess/containers/vector.hpp>

#define ISVAR(term) ((term.p < 0))

#ifdef PROF
#define PROFILE(x) x
#else
#define PROFILE(x)
#endif

struct term;
class prover;
typedef const term* termid;
struct term {
	term();
	term(nodeid _p, termid _s, termid _o);
	nodeid p;
	termid s, o;
	pobj json(const prover&) const;
};
typedef std::map<nodeid, termid> subst;
class prover {
	size_t evals = 0, unifs = 0;
public:
	typedef u64 ruleid;
	typedef std::vector<termid> termset;
	class ruleset {
	public:
		typedef std::vector<termset> btype;
	private:
		termset _head;
		btype  _body;
		size_t m = 0;
	public:
		prover* p;
		ruleset(prover* _p) : p(_p) {}
		ruleid add(termid t, const termset& ts);
		ruleid add(termid t);
		const termset& head() const	{ return _head; }
		const btype& body() const	{ return _body; }
		size_t size()			{ return _head.size(); }
		void mark();
		void revert();
		typedef std::vector<ruleid> rulelist;
		typedef std::map<nodeid, rulelist> r2id_t;
		string format() const;
		inline const rulelist& operator[](nodeid id) const { return r2id.at(id); }
		r2id_t r2id;
	} kb;
	const termset& head = kb.head();
	const std::vector<termset>& body = kb.body();
	prover ( qdb, bool check_consistency = true);
	prover ( ruleset* kb = 0 );
	prover ( string filename );
	prover ( const prover& p );
	termset qdb2termset(const qdb &q_);
	int  do_query(const termset& goal, subst* s = 0);
	void do_query(const qdb& goal, subst* s = 0);
	void query(const termset& goal, subst* s = 0);
	void query(const qdb& goal, subst* s = 0);
	~prover();

	typedef std::list<std::pair<ruleid, shared_ptr<subst>>> ground;
	typedef std::map<nodeid, std::list<std::pair<termid, ground>>> evidence;
	evidence e;
	std::vector<subst> substs;
	termid tmpvar();
	termid make(pnode p, termid s = 0, termid o = 0);
	termid make(nodeid p, termid s = 0, termid o = 0);
	termid make(termid) { throw std::runtime_error("called make(pnode/nodeid) with termid"); }
	termid make(termid, termid, termid) { throw std::runtime_error("called make(pnode/nodeid) with termid"); }

	struct proof {
		ruleid rul = 0;
		uint last, level = 0, src = 0;
		shared_ptr<proof> prev = 0, creator = 0;
		shared_ptr<subst> s = 0;//make_shared<subst>();
		ground g(prover*) const;
		termid btterm = 0;
		proof(){}// : s(make_shared<subst>()) {}
		proof(shared_ptr<proof> c, ruleid r, uint l = 0, shared_ptr<proof> p = 0, const subst& _s = subst(), uint _src = 0) 
			: rul(r), last(l), prev(p), s(make_shared<subst>(_s)), creator(c), src(_src) {}
		proof(shared_ptr<proof> c, const proof& p) : proof(c, p.rul, p.last, p.prev) { if (prev) level = prev->level + 1; }
	};
	struct proofcmp { bool operator()(const shared_ptr<proof>& x, const shared_ptr<proof>& y) const { return x->level < y->level || x->src < y->src || x->last < y->last; }};
	typedef std::priority_queue<shared_ptr<proof>, std::vector<shared_ptr<proof>>, proofcmp> queue_t;

	void addrules(pquad q, qdb& quads);
	std::vector<termid> get_list(termid head, proof& p);
	termid list2term(std::list<pnode>& l, const qdb& quads);
	termid list2term_simple(std::list<termid>& l);
	void get_dotstyle_list(termid, std::list<nodeid>&);

private:
	class termdb {
	public:
		typedef std::map<nodeid, termset> p2id_t;
		inline const termset& operator[](nodeid id) const { return p2id.at(id); }
		inline termid add(nodeid p, termid s, termid o) {
			auto r = new term/*make_shared<term>*/(p, s, o);
			p2id[p].push_back(r);
			return r;
		}
	private:
		p2id_t p2id;
	} _terms;
	friend ruleset;
	friend proof;
	int steps = 0;
	bool printNow;

	inline void pushev(shared_ptr<proof>);
	inline void step(shared_ptr<proof>&);
	#define EVAL(id) ((id) ? evaluate(*id) : 0)
	#define EVALS(id, s) ((id) ? evaluate(*id, s) : 0)
	termid evalvar(const term& x, const subst& s) {
		PROFILE(++evals);
		setproc(L"evalvar");
		auto evvit = s.find(x.p);
		termid r = evvit == s.end() ? 0 : evaluate(*evvit->second, s);
		TRACE(dout<<format(x) << ' ' << formats(s)<< " = " << format(r) << endl; if (r && r->p > 1e+8) throw 0);
		return r;
	}
	inline termid evaluate(const term& p) {
		PROFILE(++evals);
		setproc(L"evaluate");
		termid r;
		if (ISVAR(p)) return 0;
		if (!p.s && !p.o) return &p;
		termid a = evaluate(*p.s), b = evaluate(*p.o);
		return make(p.p, a ? a : make(p.s->p), b ? b : make(p.o->p));
	}

	inline termid evaluate(const term& p, const subst& s) {
		PROFILE(++evals);
		setproc(L"evaluate");
		termid r;
		if (ISVAR(p)) {
			auto it = s.find(p.p);
			r = it == s.end() ? 0 : evaluate(*it->second, s);
		} else if (!p.s && !p.o) r = &p;
		else {
			termid a = evaluate(*p.s, s), b = evaluate(*p.o, s);
			r = make(p.p, a ? a : make(p.s->p), b ? b : make(p.o->p));
		}
		TRACE(dout<<format(p) << ' ' << formats(s)<< " = " << format(r) << endl; if (r && r->p > 1e+8) throw 0);
		return r;
	}

	bool unify(termid _s, const subst& ssub, termid _d, subst& dsub);
	bool unify(termid _s, termid _d, subst& dsub);
	bool unify_snovar(const term& s, const subst& ssub, termid _d, subst& dsub);
	bool unify_dnovar(termid _s, const subst& ssub, const term& _d, subst& dsub);
	bool unify_dnovar(termid _s, const term& d, subst& dsub);
	bool unify_sdnovar(const term& s, const subst& ssub, const term& d, subst& dsub);
	bool unify_sdnovar(const term& s, const term& d, subst& dsub);
	bool unify_snovar_dvar(const term& s, const subst& ssub, const term& d, subst& dsub);
	bool unify_dnovar_ep(termid _s, const subst& ssub, const term& d, const subst& dsub);
	bool unify_snovar_dvar_ep(const term& s, const subst& ssub, const term& d, const subst& dsub);
	bool unify_ep(termid _s, const subst& ssub, const term& d, const subst& dsub);
	bool unify_sdnovar_ep(const term& s, const subst& ssub, const term& d, const subst& dsub);

	inline bool euler_path(shared_ptr<proof>&);
	int builtin(termid, shared_ptr<proof>, queue_t&);
	termid quad2term(const quad& p, const qdb& quads);
	termid list_next(termid t, proof&);
	termid list_first(termid t, proof&);
	bool islist(termid);
	bool consistency(const qdb& quads);

	// formatters
	string format(termid id, bool json = false);
	string formatkb(bool json = false);
	void printg(const ground& g);
	void printg(shared_ptr<ground> g) { printg(*g); }
	void printe();
	string format(const termset& l, bool json = false);
	void prints(const subst& s);
	void prints(shared_ptr<subst> s) { prints(*s); }
	string format(nodeid) { throw std::runtime_error("called format(termid) with nodeid"); }
	string format(nodeid, bool) { throw std::runtime_error("called format(termid) with nodeid"); }
	string format(const term&, bool json = false);
	string formatr(ruleid r, bool json = false);
	string formatg(const ground& g, bool json = false);
	void printp(shared_ptr<proof> p);
	string formatp(shared_ptr<proof> p);
	string formats(const subst& s, bool json = false);
	string formats(shared_ptr<subst>& s, bool json = false) { return s ? formats(*s, json) : string(); }
	void printterm_substs(termid id, const subst& s);
	void printl_substs(const termset& l, const subst& s);
	void printr_substs(ruleid r, const subst& s);
	void jprinte();
	pobj json(const termset& ts) const;
	pobj json(const subst& ts) const;
	pobj json(const ground& g) const;
	pobj json(ruleid rl) const;
	pobj ejson() const;
	string fsubsts(const ground& g);
	queue_t queue, gnd;
// used by unify
	termid v;
	bool r, ns;
};
#endif
