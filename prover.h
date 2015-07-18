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
		prover& p;
	public:
		ruleset(prover* _p) : p(*_p) {}
		uint add(termid t, const termset& ts, prover*);
		const termset& head() const				{ return _head; }
		const boost::container::vector<termset>& body() const	{ return _body; }
		size_t size()						{ return _head.size(); }
		void mark();
		void revert();
		typedef boost::container::list<ruleid> rulelist;
		typedef boost::container::map<resid, rulelist> r2id_t;
		inline const rulelist& operator[](resid id) const { return r2id.at(id); }
		string format() const;
	private:
		r2id_t r2id;
	} kb;
	prover ( qdb, bool check_consistency = true);
	prover ( ruleset* kb = 0 );
	prover ( string filename );
	prover ( const prover& p );
	void operator()(termset& goal, const subst* s = 0);
	void operator()(qlist goal, const subst* s = 0);
	const term& get(termid) const;
	const term& get(resid) const { throw std::runtime_error("called get(termid) with resid"); }
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
	termid make(termid) { throw std::runtime_error("called make(pnode/resid) with termid"); }
	termid make(termid, termid, termid) { throw std::runtime_error("called make(pnode/resid) with termid"); }
	string format(const termset& l, bool json = false);
	void prints(const subst& s);

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
	void addrules(pquad q);
	std::vector<termid> get_list(termid head, proof& p);
	termid list2term(std::list<pnode>& l);




private:

#ifdef OPENCL
typedef cl_short prop_t;
typedef 
#else
typedef int prop_t;
#endif

	class termdb {
	public:
		typedef boost::container::list<termid> termlist;
		typedef boost::container::map<resid, termlist> p2id_t;
		size_t size() const { return terms.size(); }
		inline const term& operator[](termid id) const { return terms.at(id); }
		inline const termlist& operator[](resid id) const { return p2id.at(id); }
		inline termid add(resid p, termid s, termid o) { terms.emplace_back(p, s, o); termid r = size(); p2id[p].push_back(r); return r; }
	private:
		boost::container::vector<term> terms;
		p2id_t p2id;
	} _terms;
	friend ruleset;

	void step (proof*, std::deque<proof*>&, bool del = true);

	bool hasvar(termid id);
	termid evaluate(termid id, const subst& s);
	bool unify(termid _s, const subst& ssub, termid _d, subst& dsub, bool f);
	bool euler_path(proof* p, termid t);
	int builtin(termid id, proof* p, std::deque<proof*>& queue);
//	bool maybe_unify(const term, const term);
//	std::set<uint>
	bool match(termid e, termid h);
	termid quad2term(const quad& p);

	string format(termid id, bool json = false);
	string format(resid) { throw std::runtime_error("called format(termid) with resid"); }
	string format(resid, bool) { throw std::runtime_error("called format(termid) with resid"); }
	string format(term t, bool json = false);
	string formatr(int r, bool json = false);
	string formatkb(bool json = false);
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
	int steps = 0;
	bool consistency();
};
#endif
