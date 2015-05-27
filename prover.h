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
#ifdef OPENCL
#define CL(x) x
#else
#define CL(x)
#endif

using namespace jsonld;

class prover {
public:
	prover ( qdb kb, qlist query );
private:
	typedef int termid;
	typedef uint ruleid;
	typedef int resid; // resource id
	typedef std::vector<termid> termset;
	typedef std::map<resid, termid> subst;
	typedef std::list<std::pair<ruleid, subst>> ground;
	typedef std::map<ruleid, std::list<std::pair<termid, ground>>> evidence;

	class ruleset {
		termset _head;
		std::vector<termset> _body;
	public:
		const termset& head() const { return _head; }
		const std::vector<termset>& body() const { return _body; }
		uint add(termid t, const termset& ts);
		uint size() { return _head.size(); }
	};

	void search ();

	class term {
	public:
		term(int _p, termid _s, termid _o, int props);
		const resid p;
		const termid s, o;
		const int properties = 0; // encode lang nodetype and datatype here
	};
	std::vector<term> terms;
	const term& get(termid id);
	termid make(string p, termid s = 0, termid o = 0, int props = 0);
	termid make(int p, termid s = 0, termid o = 0, int props = 0);

	struct proof {
		uint rul, last;
		proof *prev;
		subst s;
		ground g;
		proof() : rul(0), prev(0) {}
	};
	typedef std::deque<prover::proof*> queue;

	ruleset kb;
	termset goal;
	evidence e;
	queue q;
	proof* p;

	void addrules(pquad q, const qdb& kb);

	bool hasvar(termid id);
	termid evaluate(termid id, const subst& s);
	bool unify(termid _s, const subst& ssub, termid _d, subst& dsub, bool f);
	bool euler_path(proof* p, termid t, const ruleset& kb, bool update = false);
	int builtin(termid id);
	bool maybe_unify(termid _s, termid _d);
	std::set<uint> match(termid e, const termid* t, uint sz);
	termid mkterm(const wchar_t* p, const wchar_t* s, const wchar_t* o, const quad& q);
	termid mkterm(string s, string p, string o, const quad& q);
	termid quad2term(const quad& p);

	string format(termid id);
	string format(const termset& l);
	string format(int r, const ruleset& kb);
	string format(const ruleset& rs);
	void printp(proof* p, const ruleset& kb);
	void printq(const queue& q, const ruleset& kb);
	void prints(const subst& s);
	void printterm_substs(termid id, const subst& s);
	void printl_substs(const termset& l, const subst& s);
	void printr_substs(int r, const subst& s, const ruleset& kb);
	void printr(int r, const ruleset& kb);
	void printg(const ground& g, const ruleset& kb);
	void printe(const evidence& e, const ruleset& kb);
};
