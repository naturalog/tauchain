#ifndef __PROVER_H__
#define __PROVER_H__

/*
    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

#define __CL_ENABLE_EXCEPTIONS
#include <deque>
#include <climits>
#include "rdf.h"
#include "misc.h"
#include <functional>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/list.hpp>
#ifdef OPENCL
#define CL(x) x
extern std::string clprog;
#include <CL/cl.hpp>
#else
#define CL(x)
#endif

//enum etype { IRI, BNODE, BOOLEAN, DOUBLE, INT, FLOAT, DECIMAL, URISTR, STR };

class prover {
public:
	typedef u64 termid;

	class term {
	public:
		term();
		term(resid _p, termid _s, termid _o);
		term(const term& t);
		term& operator=(const term& t);
		resid p;
		termid s, o;
		bool isstr() const;
		pobj json(const prover&) const;
	};
	typedef u64 ruleid;
	typedef boost::container::vector<termid> termset;
	typedef boost::container::map<resid, termid> subst;
	class ruleset {
		termset _head;
		boost::container::vector<termset> _body;
		size_t m = 0;
	public:
		uint add(termid t, const termset& ts, prover*);
		const termset& head() const				{ return _head; }
		const boost::container::vector<termset>& body() const	{ return _body; }
		size_t size()						{ return _head.size(); }
		void mark();
		void revert();
	} kb;
	prover ( qdb, bool check_consistency = true);
	prover ( ruleset* kb = 0 );
	prover ( string filename );
	prover ( const prover& p );
	void operator()(termset& goal, const subst* s = 0);
	void operator()(qlist goal, const subst* s = 0);
	const term& get(termid id) const;
	const term& get(resid id) const { throw std::runtime_error("called get(termid) with resid"); }
	~prover();

	typedef boost::container::list<std::pair<ruleid, subst>> ground;
	typedef boost::container::map<resid, boost::container::set<std::pair<termid, ground>>> evidence;
	evidence e;
	std::vector<subst> substs;
	termid tmpvar();
	void printg(const ground& g);
	void printe();
	termid make(pnode p, termid s = 0, termid o = 0);
	termid make(resid p, termid s = 0, termid o = 0);
	termid make(termid p, termid s = 0, termid o = 0) { throw std::runtime_error("called make(pnode/resid) with termid"); }
	string format(const termset& l, bool json = false);
	void prints(const subst& s);

private:

#ifdef OPENCL
typedef cl_short prop_t;
typedef 
#else
typedef int prop_t;
#endif

	boost::container::vector<term> _terms;
	friend ruleset;

	struct proof {
		ruleid rul;
		uint last;
		proof *prev;
		subst s;
		ground g;
		proof() : rul(0), prev(0) {}
		proof(ruleid r, uint l = 0, proof* p = 0, const subst& _s = subst(), const ground& _g = ground() ) 
			: rul(r), last(l), prev(p), s(_s), g(_g) {}
		proof(const proof& p) : rul(p.rul), last(p.last), prev(p.prev), s(p.s), g(p.g) {}
	};
	void step (proof*, std::deque<proof*>&, bool del = true);

	void addrules(pquad q);

	bool hasvar(termid id);
	termid evaluate(termid id, const subst& s);
	bool unify(termid _s, const subst& ssub, termid _d, subst& dsub, bool f);
	bool euler_path(proof* p, termid t);
	int builtin(termid id, proof* p, std::deque<proof*>& queue);
	bool maybe_unify(const term, const term);
	std::set<uint> match(termid e);
	termid quad2term(const quad& p);

	string format(termid id, bool json = false);
	string format(resid id, bool json = false) { throw std::runtime_error("called format(termid) with resid"); }
	string format(term t, bool json = false);
	string formatr(int r, bool json = false);
	string formatkb();
	void printp(proof* p);
	string formats(const subst& s, bool json = false);
	void printterm_substs(termid id, const subst& s);
	void printl_substs(const termset& l, const subst& s);
	void printr_substs(int r, const subst& s);
	void jprinte();

#ifdef OPENCL
	void initcl();
	std::vector<cl::Platform> platforms;
	cl::Context context;
	std::vector<cl::Device> devices;
	cl::Program prog;
	cl::Kernel kernel;
	cl::Event event;
	cl::CommandQueue cq;
	uint last_rule, last_term;
	cl::Buffer clterms, clrule, clresult;
#endif
//	termid va;
	qdb quads;
	termid list_next(termid t, proof&);
	termid list_first(termid t, proof&);
	std::vector<termid> get_list(termid head, proof& p);
	bool kbowner, goalowner;
	string predstr(prover::termid t);
	string preddt(prover::termid t);
	string formatg(const ground& g, bool json = false);
	pobj json(const termset& ts) const;
	pobj json(const subst& ts) const;
	pobj json(const ground& g) const;
	pobj json(ruleid rl) const;
	pobj ejson() const;
	bool islist(termid);
	termid list2term(std::list<pnode>& l);
	int steps = 0;
	bool consistency();
};
#endif
