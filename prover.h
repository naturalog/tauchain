#ifndef __PROVER_H__
#define __PROVER_H__

/*
    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

#include <deque>
#include <climits>
#include "rdf.h"
#include "misc.h"
#include <future>
#include <functional>
#include <forward_list>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/vector.hpp>

typedef u64 termid;
typedef std::map<nodeid, termid> subst;

class prover {
public:
	class term {
	public:
		term();
		term(nodeid _p, termid _s, termid _o);
		nodeid p;
		termid s, o;
		pobj json(const prover&) const;
	};
	typedef u64 ruleid;
	typedef boost::container::vector<termid> termset;
	class ruleset {
	public:
		typedef boost::container::vector<termset> btype;
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
	const boost::container::vector<termset>& body = kb.body();
	prover ( qdb, bool check_consistency = true);
	prover ( ruleset* kb = 0 );
	prover ( string filename );
	prover ( const prover& p );
	void query(termset& goal, subst* s = 0);
	void query(const qdb& goal, subst* s = 0);
	const term& get(termid) const;
	const term& get(nodeid) const { throw std::runtime_error("called get(termid) with nodeid"); }
	~prover();

	typedef std::list<std::pair<ruleid, shared_ptr<subst>>> ground;
	typedef std::map<nodeid, std::list<std::pair<termid, ground>>> evidence;
	evidence e;
	std::vector<subst> substs;
	termid tmpvar();
	void printg(const ground& g);
	void printg(shared_ptr<ground> g) { printg(*g); }
	void printe();
	termid make(pnode p, termid s = 0, termid o = 0);
	termid make(nodeid p, termid s = 0, termid o = 0);
	termid make(termid) { throw std::runtime_error("called make(pnode/nodeid) with termid"); }
	termid make(termid, termid, termid) { throw std::runtime_error("called make(pnode/nodeid) with termid"); }
	string format(const termset& l, bool json = false);
	void prints(const subst& s);
	void prints(shared_ptr<subst> s) { prints(*s); }

	struct proof {
		ruleid rul = 0;
		uint last;
		shared_ptr<proof> prev = 0, creator = 0;
		shared_ptr<subst> s = 0;//make_shared<subst>();
		ground g(prover*) const;
		termid btterm = 0;
		proof(){}// : s(make_shared<subst>()) {}
		proof(ruleid r, uint l = 0, shared_ptr<proof> p = 0, const subst& _s = subst()) 
			: rul(r), last(l), prev(p), s(make_shared<subst>(_s)){}
		proof(const proof& p) : proof(p.rul, p.last, p.prev){}
	};

	typedef std::deque<shared_ptr<proof>> queue_t;

	void addrules(pquad q, qdb& quads);
	std::vector<termid> get_list(termid head, proof& p);
	termid list2term(std::list<pnode>& l, const qdb& quads);
	termid list2term_simple(std::list<termid>& l);
	string format(termid id, bool json = false);
	void get_dotstyle_list(termid, std::list<nodeid>&);
	string formatkb(bool json = false);

private:
	class termdb {
		typedef boost::container::vector<term> terms_t;
	public:
		terms_t terms;
		typedef std::map<nodeid, termset> p2id_t;
		size_t size() const { return terms.size(); }
		inline const term& operator[](termid id) const { return terms.at(id); }
		inline const termset& operator[](nodeid id) const { return p2id.at(id); }
		inline termid add(nodeid p, termid s, termid o) { terms.emplace_back(p, s, o); termid r = size(); p2id[p].push_back(r); return r; }
	private:
		p2id_t p2id;
	} _terms;
	friend ruleset;
	friend proof;
	int steps = 0;
	bool printNow;

	inline void pushev(shared_ptr<proof>);
	inline void step(shared_ptr<proof>&, queue_t&);
	termid evaluate(termid id, const subst& s) { return id ? evaluate(get(id), id, s) : 0; }
	inline termid evaluate(termid id) { return id ? evaluate(get(id), id) : 0; }
	termid evaluate(const term&, termid id);
	termid evaluate(const term&, termid id, const subst& s);
//	inline termid evaluate(termid id, shared_ptr<subst>& s) {
//		static subst emp;
//		return s ? evaluate(id, *s) : evaluate(id, emp);
//	}
	inline bool unify(termid _s, const subst& ssub, termid _d, subst& dsub, bool f) { return !_s ? _s == _d : unify(get(_s), _s, ssub, get(_d), _d, dsub, f); }
	inline bool unify(const term& s, termid _s, const subst& ssub, termid _d, subst& dsub, bool f) { return _d ? unify(s, _s, ssub, get(_d), _d, dsub, f) : 0; }
	inline bool unify(termid _s, const subst& ssub, const term& d, termid _d, subst& dsub, bool f) { return _s ? unify(get(_s), _s, ssub, d, _d, dsub, f) : 0; }
	bool unify(const term& s, termid _s, const subst& ssub, const term& d, termid _d, subst& dsub, bool f);
	inline bool unify(termid _s, termid _d, subst& dsub, bool f) { return !_s ? _s == _d : unify(get(_s), _s, get(_d), _d, dsub, f); }
	inline bool unify(const term& s, termid _s, termid _d, subst& dsub, bool f) { return _d ? unify(s, _s, get(_d), _d, dsub, f) : 0; }
	inline bool unify(termid _s, const term& d, termid _d, subst& dsub, bool f) { return _s ? unify(get(_s), _s, d, _d, dsub, f) : 0; }
	bool unify(const term& s, termid _s, const term& d, termid _d, subst& dsub, bool f);
//	inline bool unify(termid _s, const subst& ssub, termid _d, subst& dsub, bool f);
//	inline bool unify(const term& s, termid _s, const subst& ssub, termid _d, subst& dsub, bool f);
//	inline bool unify(termid _s, const subst& ssub, const term& d, termid _d, subst& dsub, bool f);
//	bool unify2(termid _s, const subst& ssub, termid _d, subst& dsub, bool f);
	//inline bool unify(termid _s, shared_ptr<subst>& ssub, termid _d, shared_ptr<subst>& dsub, bool f, bool printNow) { return unify(_s, *ssub, _d, *dsub, f, printNow); }
	inline bool euler_path(shared_ptr<proof>&);
	int builtin(termid, shared_ptr<proof>, queue_t&);
	termid quad2term(const quad& p, const qdb& quads);
	termid list_next(termid t, proof&);
	termid list_first(termid t, proof&);
	bool islist(termid);
	bool consistency(const qdb& quads);

	// formatters
	string format(nodeid) { throw std::runtime_error("called format(termid) with nodeid"); }
	string format(nodeid, bool) { throw std::runtime_error("called format(termid) with nodeid"); }
	string format(term t, bool json = false);
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
};
#endif
