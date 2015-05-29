/*
    Created on: Apr 28, 2015
        Author: Ohad Asor
*/

#define __CL_ENABLE_EXCEPTIONS
#include <deque>
#include <climits>
#include "rdf.h"
#include "misc.h"
#include "parsers.h"
#include <functional>
#include "ThreadPool.h"
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/list.hpp>
#ifdef OPENCL
#define CL(x) x
extern std::string clprog;
#include <CL/cl.hpp>
#else
#define CL(x)
#endif

using namespace jsonld;

typedef uint64_t u64;
typedef int64_t i64;
typedef uint8_t u8;
typedef int8_t i8;

class prover {
public:
	prover ( qdb kb, qlist query );
private:
	prover();
#ifdef OPENCL
typedef cl_short prop_t;
typedef 
#else
typedef int prop_t;
#endif
	class ruleset;
	typedef int termid;
	typedef uint ruleid;
	typedef int resid; // resource id
	typedef boost::container::map<resid, termid> subst;
	typedef boost::container::vector<termid> termset;
	typedef boost::container::list<std::pair<ruleid, subst>> ground;
	typedef boost::container::map<ruleid, boost::container::list<std::pair<termid, ground>>> evidence;

	class ruleset {
		termset _head;
		boost::container::vector<termset> _body;
	public:
		const termset& head() const { return _head; }
		const boost::container::vector<termset>& body() const { return _body; }
		uint add(termid t, const termset& ts);
		uint size() { return _head.size(); }
	};

	class term {
	public:
		term(int _p, termid _s, termid _o, int props);
		const resid p;
		const termid s, o;
		const prop_t properties = 0; // encode lang nodetype and datatype here
	};
	boost::container::vector<term> terms;
	const term& get(termid id);
	termid make(string p, termid s = 0, termid o = 0, int props = 0);
	termid make(int p, termid s = 0, termid o = 0, int props = 0);

	struct proof {
		ruleid rul;
		uint last;
		proof *prev;
		subst s;
		ground g;
		std::list<std::thread*> waitlist;
		proof() : rul(0), prev(0) {}
		proof(ruleid r, uint l = 0, proof* p = 0, const subst& _s = subst(), const ground& _g = ground() ) 
			: rul(r), last(l), prev(p), s(_s), g(_g) {}
		proof(const proof& p) : rul(p.rul), last(p.last), prev(p.prev), s(p.s), g(p.g) {}
	};
	void step (proof*);

	ruleset kb;
	termset goal;
	evidence e;

	void addrules(pquad q, const qdb& kb);

	bool hasvar(termid id);
	termid evaluate(termid id, const subst& s);
	bool unify(termid _s, const subst& ssub, termid _d, subst& dsub, bool f);
	bool euler_path(proof* p, termid t);
	int builtin(termid id, proof*);
	bool maybe_unify(const term, const term);
	std::set<uint> match(termid e);
	termid mkterm(const wchar_t* p, const wchar_t* s, const wchar_t* o, const quad& q);
	termid mkterm(string s, string p, string o, const quad& q);
	termid quad2term(const quad& p);

	string format(termid id);
	string format(const termset& l);
	string formatr(int r);
	string formatkb();
	void printp(proof* p);
	void prints(const subst& s);
	void printterm_substs(termid id, const subst& s);
	void printl_substs(const termset& l, const subst& s);
	void printr_substs(int r, const subst& s);
	void printg(const ground& g);
	void printe();

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
	termid va;
	ThreadPool pool;
};
