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
#include <malloc.h>
//#include <boost/interprocess/containers/map.hpp>
//#include <boost/interprocess/containers/set.hpp>
//#include <boost/interprocess/containers/vector.hpp>

#define ISVAR(term) ((term.p < 0))

#ifdef PROF
#define PROFILE(x) x
#else
#define PROFILE(x)
#endif

const size_t pagesize = sysconf(_SC_PAGE_SIZE);

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
#include <sys/mman.h>
#include <errno.h>
struct substs {
	//typedef std::map<nodeid, termid> data_t;
	struct sub {
		nodeid first;
		termid second;
	};
	termid get(nodeid p) const;
	void set(nodeid p, termid t);
	const sub* find(nodeid p) const;
//	const sub* begin() const;
//	const sub* end() const;
	void clear();
	bool empty() const;
	string format(bool json = false) const;
	size_t sz = 0;
	substs() {
		data = (sub*)memalign(pagesize, 4 * pagesize);
//		dout << "pagesize: " << pagesize << endl;
		lock();
}
private:
	sub *data;
	void lock() {
		if (-1 == mprotect(data, pagesize * 4, PROT_READ) ) {
			if (errno == EACCES) dout << "EACCES ";
			if (errno == EINVAL) dout << "EINVAL ";
			if (errno == ENOMEM) dout << "ENOMEM ";
			throw std::runtime_error("mproect() failed");
		}
	}
	void unlock() {
		mprotect(data, pagesize * 4, PROT_READ | PROT_WRITE);
	}
};

//typedef boost::container::map<nodeid, termid> substs;
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
	int do_query(const termid goal);
	int  do_query(const termset& goal, substs* s = 0);
	void do_query(const qdb& goal, substs * s = 0);
	void query(const termset& goal, substs * s = 0);
	void query(const qdb& goal, substs * s = 0);
	~prover();

	typedef std::list<std::pair<ruleid, shared_ptr<substs>>> ground;
	typedef std::map<nodeid, std::list<std::pair<termid, ground>>> evidence;
	evidence e;
	std::vector<substs> substss;
	termid tmpvar();
	termid make(pnode p, termid s = 0, termid o = 0);
	termid make(nodeid p, termid s = 0, termid o = 0);
	termid make(termid) { throw std::runtime_error("called make(pnode/nodeid) with termid"); }
	termid make(termid, termid, termid) { throw std::runtime_error("called make(pnode/nodeid) with termid"); }

	struct proof {
		ruleid rule = 0;
		uint term_idx, level = 0;
		shared_ptr<proof> prev = 0, creator = 0;
		shared_ptr<substs> s = make_shared<substs>();
		ground g(prover*) const;
		termid btterm = 0;
		uint src = 0;
		/*bool predvar = false;*/
		proof(){}// : s(make_shared<substs>()) {}
		proof(shared_ptr<proof> c, ruleid r, uint l = 0, shared_ptr<proof> p = 0, const substs & _s = substs(), uint _src = 0/*, bool _predvar = false*/)
			: rule(r), term_idx(l), prev(p), creator(c), s(make_shared<substs>(_s)), src(_src)/*, predvar(_predvar)*/{}
		proof(shared_ptr<proof> c, const proof& p) : proof(c, p.rule, p.term_idx, p.prev) { if (prev) level = prev->level + 1; }
	};
	struct proofcmp { bool operator()(const shared_ptr<proof>& x, const shared_ptr<proof>& y) const { return x->level < y->level || x->src < y->src || x->term_idx < y->term_idx; }};
	typedef std::priority_queue<shared_ptr<proof>, std::vector<shared_ptr<proof>>, proofcmp> queue_t;

	void addrules(pquad q, qdb& quads);
	std::vector<termid> get_list(termid head, proof& p);
	termid list2term(std::list<pnode>& l, const qdb& quads);
	termid list2term_simple(std::list<termid>& l);
	void get_dotstyle_list(termid, std::list<nodeid>&);

	typedef std::vector <nodeid> nodeids;
	typedef std::vector <termid> termids;

	nodeids get_list(nodeid head);
	termids askts(termid var, termid s, pnode p, termid o, int stop_at=0);
	nodeids askns(termid var, termid s, pnode p, termid o, int stop_at=0);
	nodeids ask4ss(pnode p, pnode o, int stop_at = 0);
	nodeids ask4os(pnode s, pnode p, int stop_at = 0);
	nodeid ask1o(pnode s, pnode p);
	nodeid ask1s(pnode p, pnode o);
	termid ask1ot(pnode s, pnode p);
	nodeid force_one_n(nodeids r);
	termid force_one_t(termids r);

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
	inline void step_in(size_t &src, ruleset::rulelist &candidates, shared_ptr<proof> &_p, termid t);
	#define EVAL(id) ((id) ? evaluate(*id) : 0)
	#define EVALS(id, s) ((id) ? evaluate(*id, s) : 0)
	termid evalvar(const term& x, const substs & s) {
		PROFILE(++evals);
		setproc(L"evalvar");
		auto evvit = s.find(x.p);
		termid r = evvit ? evaluate(*evvit->second, s) : 0;
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

	inline termid evaluate(const term& p, const substs & s) {
		PROFILE(++evals);
		setproc(L"evaluate");
		termid r;
		if (ISVAR(p)) {
			auto it = s.find(p.p);
			r = it ? evaluate(*it->second, s) : 0;
		} else if (!p.s && !p.o) r = &p;
		else {
			termid a = evaluate(*p.s, s), b = evaluate(*p.o, s);
			r = make(p.p, a ? a : make(p.s->p), b ? b : make(p.o->p));
		}
		TRACE(dout<<format(p) << ' ' << formats(s)<< " = " << format(r) << endl; if (r && r->p > 1e+8) throw 0);
		return r;
	}

	bool unify(termid _s, const substs & ssub, termid _d, substs & dsub);
	bool unify(termid _s, termid _d, substs & dsub);
	bool unify_snovar(const term& s, const substs & ssub, termid _d, substs & dsub);
	bool unify_dnovar(termid _s, const substs & ssub, const term& _d, substs & dsub);
	bool unify_dnovar(termid _s, const term& d, substs & dsub);
	bool unify_sdnovar(const term& s, const substs & ssub, const term& d, substs & dsub);
	bool unify_sdnovar(const term& s, const term& d, substs & dsub);
	bool unify_snovar_dvar(const term& s, const substs & ssub, const term& d, substs & dsub);
	bool unify_dnovar_ep(termid _s, const substs & ssub, const term& d, const substs & dsub);
	bool unify_snovar_dvar_ep(const term& s, const substs & ssub, const term& d, const substs & dsub);
	bool unify_ep(termid _s, const substs & ssub, const term& d, const substs & dsub);
	bool unify_sdnovar_ep(const term& s, const substs & ssub, const term& d, const substs & dsub);

	inline bool euler_path(shared_ptr<proof>&);
	int builtin(termid, shared_ptr<proof>, queue_t&);
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
	void prints(const substs & s);
	void prints(shared_ptr<substs> s) { prints(*s); }
	string format(nodeid) { throw std::runtime_error("called format(termid) with nodeid"); }
	string format(nodeid, bool) { throw std::runtime_error("called format(termid) with nodeid"); }
	string formatr(ruleid r, bool json = false);
	string formatg(const ground& g, bool json = false);
	void printp(shared_ptr<proof> p);
	string formatp(shared_ptr<proof> p);
	string formats(const substs & s, bool json = false) { return s.format(json); }
	string formats(shared_ptr<substs>& s, bool json = false) { return s ? formats(*s, json) : string(); }
	void printterm_substs(termid id, const substs & s);
	void printl_substs(const termset& l, const substs & s);
	void printr_substs(ruleid r, const substs & s);
	void jprinte();
	pobj json(const termset& ts) const;
	pobj json(const substs & ts) const;
	pobj json(const ground& g) const;
	pobj json(ruleid rl) const;
	pobj ejson() const;
	string fsubsts(const ground& g);
	queue_t queue, gnd;
	static void unittest(); 
};
#endif
