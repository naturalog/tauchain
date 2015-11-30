#ifndef __PROVER_H__
#define __PROVER_H__

/*
    Created on: Apr 28, 2015
        Author: Ohad Asor
*/
#ifdef TIMER
#include <chrono>
#include <future>
#endif
#include <deque>
#include <queue>
#include <climits>
#include "rdf.h"
#include "misc.h"
#include <functional>
#include <forward_list>
//#include <malloc.h>
#include <assert.h>
//#include <sys/mman.h>
//#include <errno.h>
//#include <unistd.h>
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
#ifdef JSON
	pobj json(const prover&) const;
#endif
};

//struct subcmp { bool operator()( const std::pair<nodeid, termid>& x, const std::pair<nodeid, termid>& y) const { return x.first < y.first; } };
//typedef std::set<std::pair<nodeid, termid>, subcmp> subs;
typedef std::map<nodeid, termid> subs;
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
	const termset&heads = kb.head();
	const std::vector<termset>&bodies = kb.body();
	prover ( qdb, bool check_consistency = true);
	prover ( ruleset* kb = 0 );
	prover ( string filename );
	prover ( const prover& p );
	termset qdb2termset(const qdb &q_);
	int do_query(const termid goal, subs* s = 0);
	int  do_query(const termset& goal, subs* s = 0);
	void do_query(const qdb& goal, subs * s = 0);
	void query(const termset& goal, subs * s = 0);
	void query(const qdb& goal, subs * s = 0);
	~prover();

	typedef std::list<std::pair<ruleid, subs>> ground;
	typedef std::map<nodeid, std::list<std::pair<termid, ground>>> evidence;
	evidence e;
	termid tmpvar();
	termid make(pnode p, termid s = 0, termid o = 0);
	termid make(nodeid p, termid s = 0, termid o = 0);
	termid make(termid) { throw std::runtime_error("called make(pnode/nodeid) with termid"); }
	termid make(termid, termid, termid) { throw std::runtime_error("called make(pnode/nodeid) with termid"); }

	struct proof {
		ruleid rule = 0;
		uint term_idx, level = 0;
		shared_ptr<proof> prev = 0, creator = 0, next = 0;
		subs s;
		ground g(prover*) const;
		termid btterm = 0;
		proof(shared_ptr<proof> c, ruleid r, uint l = 0, shared_ptr<proof> p = 0, const subs&  _s = subs())
			: rule(r), term_idx(l), prev(p), creator(c), s(_s) { }
		proof(shared_ptr<proof> c, const proof& p)
			: proof(c, p.rule, p.term_idx, p.prev) { if (prev) level = prev->level + 1; }
	};
//	struct proofcmp { bool operator()(const shared_ptr<proof>& x, const shared_ptr<proof>& y) const { return x->level < y->level || x->src < y->src || x->term_idx < y->term_idx; }};
//	typedef std::priority_queue<shared_ptr<proof>, std::vector<shared_ptr<proof>>, proofcmp> queue_t;

	void addrules(pquad q, qdb& quads);
	std::vector<termid> get_list(termid head, proof*);
	termid list2term(std::list<pnode>& l, const qdb& quads);
	termid list2term_simple(std::list<termid>& l);

	void get_dotstyle_list(termid, std::list<nodeid>&);
	void get_dotstyle_list(termid id, std::vector<termid> &list);
	std::vector<termid> get_dotstyle_list(termid id);

	typedef std::vector <nodeid> nodeids;
	typedef std::vector <termid> termids;

	nodeids get_list(nodeid head);
	bool ask(termid s, nodeid p, termid o);
	bool ask(nodeid s, nodeid p, nodeid o);
	termids askt(termid s, nodeid p, termid o, size_t stop_at=0);
	nodeids askn(termid s, nodeid p, termid o, size_t stop_at=0);
	nodeids askns(nodeid p, nodeid o, size_t stop_at = 0);
	nodeids askno(nodeid s, nodeid p, size_t stop_at = 0);
	nodeid askn1o(nodeid s, nodeid p);
	nodeid askn1s(nodeid p, nodeid o);
	termid askt1o(nodeid s, nodeid p);
	nodeid force_one_n(nodeids r);
	termid force_one_t(termids r);

	std::map<nodeid, std::vector<termid>> subs_workardound;

	void clear();

	class termdb {
	public:
		typedef std::map<nodeid, termset> p2id_t;
		inline const termset& operator[](nodeid id) const { return p2id.at(id); }
		bool equals(termid x, termid y) {
			if (!x || !y) return !x == !y;
			if (x->p == y->p) return equals(x->s, y->s) && equals(x->o, y->o);
			return false;
		}

		//Check if the term defined by this (p,s,o) is already in the
		//p2id_t. If so, just return the one that's already there. Otherwise
		//make the term for this (p,s,o) and add that to p2id[p].
		//**Can we just use t instead of making r?
		inline termid add(nodeid p, termid s, termid o) {
			auto& pp = p2id[p];
			term t(p, s, o);
			for (auto x : pp) if (equals(x, &t)) return x;
//			dout <<"watch *(int*)"<< r << endl;
//			unlock();
//			termid r = &(data[sz++] = t);
//			lock();
			termid r = new term(p, s, o);
			pp.push_back(r);
			return r;
		}
//		termdb(){lock();}
	private:
//		size_t sz = 0;
//		const size_t pages = 4;
//		term* data = (term*)memalign(pagesize, pagesize * pages);
		p2id_t p2id;
//	        void lock() {
//	                if (-1 == mprotect(data, pagesize * pages, PROT_READ) ) {
//	                        dout<<(errno==EACCES?"EACCES ":errno==EINVAL?"EINVAL ":errno==ENOMEM?"ENOMEM ":"");
//	                        throw std::runtime_error("mprotect() failed");
//	                }
//        	}
//	        void unlock() { mprotect(data, pagesize * pages, PROT_READ | PROT_WRITE); }
	} _terms;
	friend ruleset;
	friend proof;

private:

	int steps = 0;
	bool printNow;

	inline void pushev(shared_ptr<proof>);
	inline shared_ptr<proof> step(shared_ptr<proof>);
	inline void step_in(ruleset::rulelist &candidates, shared_ptr<proof> _p, termid t);
	subs::const_iterator evvit;
	#define EVAL(id) ((id) ? evaluate(*id) : 0)
	#define EVALS(id, s) ((id) ? evaluate(*id, s) : 0)
	#define EVALPS(id, s) ((s.empty()) ? (EVALS(id, s)) : ((EVAL(id))))
	termid evalvar(const term& x, const subs& s) {
		PROFILE(++evals);
		setproc(L"evalvar");
		termid r = ((evvit = s.find(x.p)) == s.end()) ? 0 : evaluate(*evvit->second, s);
		TRACE(dout<<format(x) << ' ' << formats(s)<< " = " << format(r) << endl; if (r && r->p > 1e+8) throw 0);
		return r;
	}
	inline termid evaluate(const term& p) {
		PROFILE(++evals);
		setproc(L"evaluate");
		if (ISVAR(p)) return 0;
		if (!p.s && !p.o) return &p;
		termid a = evaluate(*p.s), b = evaluate(*p.o);
		return make(p.p, a ? a : make(p.s->p), b ? b : make(p.o->p));
	}

	inline termid evaluate(const term& p, const subs&  s) {
		PROFILE(++evals);
		setproc(L"evaluate");
//		dout<<"eval:"<<format(p) << ' ' << formats(s) << endl;
		termid r;
		if (ISVAR(p)) r = ((evvit = s.find(p.p)) == s.end()) ? 0 : evaluate(*evvit->second, s);
		else if (!p.s && !p.o) r = &p;
		else if (!p.s != !p.o) throw 0;
		else {
			termid a = evaluate(*p.s, s), b = evaluate(*p.o, s);
			r = make(p.p, a ? a : make(p.s->p), b ? b : make(p.o->p));
		}
		TRACE(dout<<format(p) << ' ' << formats(s)<< " = " << format(r) << endl);
		return r;
	}

	bool unify(termid _s, const subs& ssub, termid _d, subs& dsub);
//	bool unify(termid _s, const subs& ssub, termid _d);
//	bool unify(termid _s, termid _d);
	bool unify(termid _s, termid _d, subs& dsub);
	bool unify_snovar(const term& s, const subs& ssub, termid _d, subs& dsub);
	bool unify_dnovar(termid _s, const subs& ssub, const term& _d, subs& dsub);
	bool unify_dnovar(termid _s, const term& d, subs& dsub);
	bool unify_sdnovar(const term& s, const subs& ssub, const term& d, subs& dsub);
	bool unify_sdnovar(const term& s, const term& d, subs& dsub);
	bool unify_snovar_dvar(const term& s, const subs& ssub, const term& d, subs& dsub);
	bool unify_dnovar_ep(termid _s, const subs& ssub, const term& d, const subs& dsub);
	bool unify_snovar_dvar_ep(const term& s, const subs& ssub, const term& d, const subs& dsub);
	bool unify_ep(termid _s, const subs& ssub, const term& d, const subs& dsub);
	bool unify_ep(termid _s, const subs& ssub, const term& d);
	bool unify_ep(termid _s, const term& d, const subs& dsub);
	bool unify_ep(termid _s, const term& d);
	bool unify_snovar_ep(const term& s, const subs& ssub, const term& d);
	bool unify_sdnovar_ep(const term& s, const subs& ssub, const term& d);
	bool unify_sdnovar_ep(const term& s, const term& d, const subs& dsub);
	bool unify_sdnovar_ep(const term& s, const subs& ssub, const term& d, const subs& dsub);
	bool unify(termid _s, const shared_ptr<subs>& ssub, termid _d, subs& dsub) {
		return ssub ? unify(_s, *ssub, _d, dsub) : unify(_s, _d, dsub);
	}
	bool unify(termid _s, const shared_ptr<subs>& ssub, termid _d, shared_ptr<subs>& dsub) {
		return ssub ? unify(_s, *ssub, _d, *dsub) : unify(_s, _d, *dsub);
	}
	bool unify_ep(termid _s, const shared_ptr<subs>& ssub, const term& _d, const shared_ptr<subs>& dsub) {
		return ssub ? unify_ep(_s, *ssub, _d, dsub) : unify_ep(_s, _d, dsub);
	}
	bool unify_ep(termid _s, const subs& ssub, const term& _d, const shared_ptr<subs>& dsub) {
		return dsub ? unify_ep(_s, ssub, _d, *dsub) : unify_ep(_s, ssub, _d);
	}
	bool unify_ep(termid _s, const term& _d, const shared_ptr<subs>& dsub) {
		return dsub ? unify_ep(_s, _d, *dsub) : unify_ep(_s, _d);
	}

	inline bool euler_path(shared_ptr<proof>);
	int builtin(termid, shared_ptr<proof>);
	int rdfs_builtin(const term& t, const term *t0, const term *t1);
	termid quad2term(const quad& p, const qdb& quads);
	termid list_next(termid t, proof&);
	termid list_first(termid t, proof&);
	bool islist(termid);
	bool consistency(const qdb& quads);

	// formatters
public:
	static string format(termid id, bool json = false);
	static string format(const term&, bool json = false);
	string formatkb(bool json = false);
	void printg(const ground& g);
	void printg(shared_ptr<ground> g) { printg(*g); }
	void printe();
	string format(const termset& l, bool json = false);
	void prints(const subs&  s);
	void prints(shared_ptr<subs> s) { prints(*s); }
	string format(nodeid) { throw std::runtime_error("called format(termid) with nodeid"); }
	string format(nodeid, bool) { throw std::runtime_error("called format(termid) with nodeid"); }
	string formatr(ruleid r, bool json = false);
	string formatg(const ground& g, bool json = false);
	void printp(shared_ptr<proof> p);
	string formatp(shared_ptr<proof> p);
	string formats(const subs&  s, bool json = false);// { return s.format(json); }
	string formats(shared_ptr<subs>& s, bool json = false) { return s ? formats(*s, json) : string(); }
	void printterm_subs(termid id, const subs&  s);
	void printl_subs(const termset& l, const subs&  s);
	void printr_subs(ruleid r, const subs&  s);
#ifdef JSON
	void jprinte();
	pobj json(const termset& ts) const;
	pobj json(const subs&  ts) const;
	pobj json(const ground& g) const;
	pobj json(ruleid rl) const;
	pobj ejson() const;
#endif
	string fsubs(const ground& g);
	//queue_t queue, gnd;
	std::list<shared_ptr<proof>> gnd;
	static void unittest();
	subs termsub;
	ruleset::r2id_t::const_iterator rit;
	shared_ptr<proof> lastp = 0;
};


#endif
