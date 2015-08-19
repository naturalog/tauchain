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
typedef std::map<resid, termid> subs;
/*
template<typename T>
struct list {
	T item;
	list<T>* next;
};

typedef list<const wchar_t*> res;

struct res {
	size_t sz;
	union {
		const wchar_t* value;
		res* list;
	};
};

struct res {
	const wchar_t* const* values;
	size_t sz;
	res() : values(0), sz(0) {}
	bool isvar() { return first == L'?'; }
	bool islist() { return first == '.'; }
	void init(pnode n) { value = n->value.c_str(); }
	void init(std::list<pnode>& l, const qdb& quads) {
		first = L'.';
		if (l.empty()) return 0;
		res* r = list = new res[l.size()];
		for (auto x : l) {
			auto it = quads.second.find(*x->value);
			if (it == quads.second.end()) r->init(x);
			else r->init(it->second, quads);
			++r;
		}
	}

	~res() {
		if (list) delete[] list;
		list = 0;
	}
};
*/
struct term {
	term();
	resid p;//, s, o;
	termid s, o;
	std::function<termid(const subs&)> evaluate, dosub;
	std::function<bool(const subs&, termid, subs&)> unify;
	std::function<bool(const subs&, const term&, const subs&)> unify_ep;
	term(resid _p, termid _s = 0, termid _o = 0);
#ifdef JSON
	pobj json(const prover&) const;
#endif
};

//struct subcmp { bool operator()( const std::pair<resid, termid>& x, const std::pair<resid, termid>& y) const { return x.first < y.first; } };
//typedef std::set<std::pair<resid, termid>, subcmp> subs;
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
		typedef std::map<resid, rulelist> r2id_t;
		string format() const;
		inline const rulelist& operator[](resid id) const { return r2id.at(id); }
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
	int  do_query(const termset& goal, subs* s = 0);
	void do_query(const qdb& goal, subs * s = 0);
	void query(const termset& goal, subs * s = 0);
	void query(const qdb& goal, subs * s = 0);
	~prover();

	typedef std::list<std::pair<ruleid, shared_ptr<subs>>> ground;
	typedef std::map<resid, std::list<std::pair<termid, ground>>> evidence;
	evidence e;
	std::vector<subs> subss;
	termid tmpvar();
	static termid make(pnode p, termid s = 0, termid o = 0);
	static termid make(resid p, termid s = 0, termid o = 0);
//	static termid make(resid p, resid s, resid o);
	static termid make(termid) { throw std::runtime_error("called make(pnode/resid) with termid"); }
	static termid make(termid, termid, termid) { throw std::runtime_error("called make(pnode/resid) with termid"); }

	struct proof {
		ruleid rule = 0;
		uint term_idx, level = 0;
		shared_ptr<proof> prev = 0, creator = 0, next = 0;
		shared_ptr<subs> s = 0;//make_shared<subs>();
//		subs s;
		ground g(prover*) const;
		termid btterm = 0;
//		uint src = 0;
		/*bool predvar = false;*/
//		proof(){}// : s(make_shared<subs>()) {}
		proof(shared_ptr<proof> c, ruleid r, uint l = 0, shared_ptr<proof> p = 0, const subs&  _s = subs())
			: rule(r), term_idx(l), prev(p), creator(c), s(make_shared<subs>(_s)) { }
		proof(shared_ptr<proof> c, const proof& p) 
			: proof(c, p.rule, p.term_idx, p.prev) { if (prev) level = prev->level + 1; }
	};
//	struct proofcmp { bool operator()(const shared_ptr<proof>& x, const shared_ptr<proof>& y) const { return x->level < y->level || x->src < y->src || x->term_idx < y->term_idx; }};
//	typedef std::priority_queue<shared_ptr<proof>, std::vector<shared_ptr<proof>>, proofcmp> queue_t;

	void addrules(pquad q, qdb& quads);
	std::vector<termid> get_list(termid head, proof*);
	termid list2term(std::list<pnode>& l, const qdb& quads);
	termid list2term_simple(std::list<termid>& l);
	void get_dotstyle_list(termid, std::list<resid>&);

	typedef std::vector <resid> resids;
	typedef std::vector <termid> termids;

	resids get_list(resid head);
	termids askts(termid var, termid s, pnode p, termid o, int stop_at=0);
	resids askns(termid var, termid s, pnode p, termid o, int stop_at=0);
	resids ask4ss(pnode p, pnode o, int stop_at = 0);
	resids ask4os(pnode s, pnode p, int stop_at = 0);
	resid ask1o(pnode s, pnode p);
	resid ask1s(pnode p, pnode o);
	termid ask1ot(pnode s, pnode p);
	resid force_one_n(resids r);
	termid force_one_t(termids r);

private:
	class termdb {
	public:
		typedef std::map<resid, termset> p2id_t;
		inline const termset& operator[](resid id) const { return p2id.at(id); }
		bool equals(termid x, termid y) {
//			return x->p == y->p && 
//				x->s == y->s && 
//				x->o == y->o;
			if (!x || !y) return !x == !y;
			if (x->p == y->p) return equals(x->s, y->s) && equals(x->o, y->o);
			return false;
		}
		inline termid add(resid p, termid s, termid o) {
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
	};
	static termdb _terms;
	friend ruleset;
	friend proof;
	int steps = 0;
	bool printNow;

	inline void pushev(shared_ptr<proof>);
	inline shared_ptr<proof> step(shared_ptr<proof>);
	inline void step_in(size_t &src, ruleset::rulelist &candidates, shared_ptr<proof> _p, termid t);
	subs::const_iterator evvit;
	termid evaluate(const term& p, const subs& s) { return p.evaluate(s); }
	termid evaluate(const term& p, const shared_ptr<subs> s) {
		static subs dummy;
		return evaluate(p, s ? *s : dummy);
	}
	termid evaluate(termid t, const shared_ptr<subs> s) {
		return t ? evaluate(*t, s) : 0;
	}
	bool unify(termid _s, const subs& ssub, termid _d, subs& dsub);
	bool unify(termid _s, const shared_ptr<subs> ssub, termid _d, const shared_ptr<subs> dsub) {
		static subs dummy1, dummy2;
		bool r = unify(_s, ssub ? *ssub : dummy1, _d, dsub ? *dsub : dummy2);
		dummy1.clear(); dummy2.clear();
		return r;
	}
	bool unify_ep(termid _s, const subs& ssub, const term& d, const subs& dsub);
	bool unify_ep(termid _s, const shared_ptr<subs> ssub, const term& d, const shared_ptr<subs> dsub) {
		static subs dummy;
		bool r = unify_ep(_s, ssub ? *ssub : dummy, d, dsub ? *dsub : dummy);
		dummy.clear();
		return r;
	}

	inline bool euler_path(shared_ptr<proof>);
	int builtin(termid, shared_ptr<proof>);
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
	static string format(resid r) { return dstr(r); } // { throw std::runtime_error("called format(termid) with resid"); }
	static string format(resid r, bool) { return dstr(r); }// { throw std::runtime_error("called format(termid) with resid"); }
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
