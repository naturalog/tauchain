//#define READLINE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifdef READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include <iostream>
#include <ostream>
#include "misc.h"
#include <utility>
#include "object.h"
#include "prover.h"
#include <iterator>
#include <forward_list>
#include <boost/algorithm/string.hpp>

using namespace boost::algorithm;

int logequalTo, lognotEqualTo, rdffirst, rdfrest, A, rdfsResource, rdfList, Dot, GND, rdfsType, rdfssubClassOf;
int _indent = 0;


#ifdef DEBUG
#define setproc(x) _setproc __setproc(x)
#else
#define setproc(x)
#endif

bool prover::hasvar(termid id) {
	const term p = get(id);
	setproc(L"hasvar");
	if (p.p < 0) return true;
	if (!p.s && !p.o) return false;
	return hasvar(p.s) || hasvar(p.o);
}

prover::termid prover::evaluate(termid id, const subst& s) {
	setproc(L"evaluate");
	termid r;
	const term p = get(id);
	if (p.p < 0) {
		auto it = s.find(p.p);
		r = it == s.end() ? 0 : evaluate(it->second, s);
	} else if (!p.s && !p.o)
		r = id;
	else {
		termid a = evaluate(p.s, s);
		termid b = evaluate(p.o, s);
		r = make(p.p, a ? a : make(get(p.s).p), b ? b : make(get(p.o).p));
	}
	TRACE(printterm_substs(id, s); dout << " = "; if (!r) dout << "(null)"; else dout << format(r); dout << std::endl);
	return r;
}

bool prover::unify(termid _s, const subst& ssub, termid _d, subst& dsub, bool f) {
	setproc(L"unify");
	termid v;
	const term s = get(_s);
	const term d = get(_d);
	bool r, ns = false;
	if (s.p < 0) 
		r = (v = evaluate(_s, ssub)) ? unify(v, ssub, _d, dsub, f) : true;
	else if (d.p < 0) {
		if ((v = evaluate(_d, dsub)))
			r = unify(_s, ssub, v, dsub, f);
		else {
			if (f) {
				dsub[d.p] = evaluate(_s, ssub);
				ns = true;
			}
			r = true;
		}
	} else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o))
		r = false;
	else
		r = !s.s || (unify(s.s, ssub, d.s, dsub, f) && unify(s.o, ssub, d.o, dsub, f));
	TRACE(dout << "Trying to unify ";
		printterm_substs(_s, ssub);
		dout<<" with ";
		printterm_substs(_d, dsub);
		dout<<"... ";
		if (r) {
			dout << "passed";
			if (ns) dout << " with new substitution: " << dstr(d.p) << " / " << format(dsub[d.p]);
		} else dout << "failed";
		dout << std::endl);
	return r;
}

bool prover::euler_path(proof* p, termid t) {
	proof* ep = p;
	while ((ep = ep->prev))
		if (!kb.head()[ep->rul] == !t &&
			unify(kb.head()[ep->rul], ep->s, t, p->s, false))
			break;
	if (ep) { TRACE(dout<<"Euler path detected\n"); }
	return ep;
}

int prover::builtin(termid id, proof* p) {
	setproc(L"builtin");
	const term t = get(id);
	int r = -1;
	termid i0 = t.s ? evaluate(t.s, p->s) : 0;
	termid i1 = t.o ? evaluate(t.o, p->s) : 0;
	const term *t0 = i0 ? &get(i0=evaluate(t.s, p->s)) : 0;
	const term* t1 = i1 ? &get(i1=evaluate(t.o, p->s)) : 0;
	if (t.p == GND) r = 1;
	else if (t.p == logequalTo)
		r = t0 && t1 && t0->p == t1->p ? 1 : 0;
	else if (t.p == lognotEqualTo)
		r = t0 && t1 && t0->p != t1->p ? 1 : 0;
	else if (t.p == rdffirst && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->s, p->s, t.o, p->s, true) ? 1 : 0;
	else if (t.p == rdfrest && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->o, p->s, t.o, p->s, true) ? 1 : 0;
	else if (t.p == A || t.p == rdfsType || t.p == rdfssubClassOf) {
		if (!t0) t0 = &get(i0=t.s);
		if (!t1) t1 = &get(i1=t.o);
		termid h = make ( A,           i0, i1 );
		termset ts;
		ts.push_back ( make ( rdfssubClassOf, va, i1 ) );
		ts.push_back ( make ( A,              i0, va ) );
		proof* f = new proof;
		f->rul = kb.add(h, ts);
		f->prev = p;
		f->last = 0;
		f->g = p->g;
		if (euler_path(f, h))
			return -1;
//		q.push_back(f);
//		pool.enqueue([this,f]{step(f);});
		TRACE(dout<<"builtin created frame:"<<std::endl;printp(f));
		step(f);
		r = -1;
	}
	if (r == 1) {
		proof* r = new proof;
		*r = *p;
		uint rl = kb.add(evaluate(id, p->s), termset());
		TRACE(dout << "builtin added rule: " << format(rl) << " by evaluating "; printterm_substs(id, p->s); dout << std::endl);
		r->g = p->g;
		r->g.emplace_back(rl, subst());
		++r->last;
//		q.push_back(r);
//		pool.enqueue([this,r]{step(r);});
		step(r);
	}

	return r;
}

bool prover::maybe_unify(const term s, const term d) {
	return (s.p < 0 || d.p < 0) ? true : (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) ? false :
		!s.s || (maybe_unify(terms[s.s-1], terms[d.s-1]) && maybe_unify(terms[s.o-1], terms[d.o-1]));
}

std::set<uint> prover::match(termid _e) {
	if (!_e) return std::set<uint>();
	setproc(L"match");
	std::set<uint> m;
	termid h;
	const term e = terms[_e-1];
	for (uint n = 0; n < kb.size(); ++n)
		if (((h=kb.head()[n])) && maybe_unify(e, get(h)))
			m.insert(n);
	return m;
}

void prover::step(proof* p) {
	setproc(L"step");
	TRACE(dout<<"popped frame:\n";printp(p));
	if (p->last != kb.body()[p->rul].size() && !euler_path(p, kb.head()[p->rul])) {
		termid t = kb.body()[p->rul][p->last];
		if (!t) throw 0;
		TRACE(dout<<"Tracking back from " << format(t) << std::endl);
		if (builtin(t, p) != -1) return;
		for (auto rl : match(evaluate(t, p->s))) {
			subst s;
			if (!unify(t, p->s, kb.head()[rl], s, true)) continue;
			proof* r = new proof(rl, 0, p, s, p->g);
			if (kb.body()[rl].empty()) r->g.emplace_back(rl, subst());
			step(r);
		}
	}
	else if (!p->prev) {
		for (auto r = kb.body()[p->rul].begin(); r != kb.body()[p->rul].end(); ++r) {
			termid t = evaluate(*r, p->s);
			if (!t || hasvar(t)) continue;
			TRACE(dout << "pushing evidence: " << format(t) << std::endl);
			e[get(t).p].emplace_back(t, p->g);
		}
	} else {
		ground g = p->g;
		if (!kb.body()[p->rul].empty())
			g.emplace_back(p->rul, p->s);
		proof* r = new proof(*p->prev);
		r->g = g;
		unify(kb.head()[p->rul], p->s, kb.body()[r->rul][r->last], r->s, true);
		++r->last;
		step(r);
	}
	delete p;
}

prover::termid prover::mkterm(const wchar_t* p, const wchar_t* s, const wchar_t* o, const quad& ) {
	termid ps = s ? make(dict.set(s), 0, 0/*, q.subj*/) : 0;
	termid po = o ? make(dict.set(o), 0, 0/*, q.object*/) : 0;
	return make(dict.set(string(p)), ps, po/*, q.pred*/);
}

prover::termid prover::mkterm(string s, string p, string o, const quad& q) {
	return mkterm(p.c_str(), s.c_str(), o.c_str(), q);
}

prover::termid prover::quad2term(const quad& p) {
	return mkterm(p.subj->value, p.pred->value, p.object->value, p);
}

void prover::addrules(pquad q, const qdb& kb) {
	const string &s = q->subj->value, &p = q->pred->value, &o = q->object->value;
	if ( p != implication || kb.find ( o ) == kb.end() ) 
		this->kb.add(quad2term(*q), termset());
	else for ( jsonld::pquad y : *kb.at ( o ) ) {
		termset ts;
		if ( kb.find ( s ) != kb.end() )
			for ( jsonld::pquad z : *kb.at ( s ) )
				ts.push_back( quad2term( *z ) );
		this->kb.add(quad2term(*y), ts);
	}
}

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}

prover::prover ( qdb qkb, qlist query ) : prover() {
	for ( jsonld::pquad quad : *qkb.at(L"@default")) 
		addrules(quad, qkb);
	for ( auto q : query )
		goal.push_back( quad2term( *q ) );
	va = make(L"?A");
	proof* p = new proof;
	p->rul = kb.add(0, goal);
	p->last = 0;
	p->prev = 0;
	TRACE(dout << KRED << "Facts:\n" << formatkb() << KGRN << "Query: " << format(goal) << KNRM << std::endl);
	step(p);
//	pool.enqueue([this,p]{step(p);});
	TRACE(dout << KWHT << "Evidence:" << std::endl; 
		printe(); dout << KNRM);
}

prover::term::term(int _p, termid _s, termid _o, int props) : p(_p), s(_s), o(_o), properties(props) {}

const prover::term& prover::get(termid id) {
	if (!id) throw 0;
	return terms[id - 1]; 
}

prover::termid prover::make(string p, termid s, termid o, int props) { 
	return make(dict.set(p), s, o, props); 
}

prover::termid prover::make(int p, termid s, termid o, int props) {
	if (!p) throw 0;
	terms.emplace_back(p,s,o,props);
	return terms.size();
}

uint prover::ruleset::add(termid t, const termset& ts) {
	_head.push_back(t);
	_body.push_back(ts);
	return _head.size()-1;
}

#ifdef OPENCL
#define __CL_ENABLE_EXCEPTIONS
 
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

std::string clprog = ""
"bool maybe_unify(termid _s, termid _d, const term* t) {"
"	local const term s = t[_s-1], d = t[_d-1];"
"	if (s.p < 0 || d.p < 0) return true;"
"	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;"
"	return !s.s || (maybe_unify(s.s, d.s) && maybe_unify(s.o, d.o));"
"}"
"kernel void match(termid e, const termid* t, uint sz, global termid* res) {"
"	uint n = get_global_id(0);"
"	prefetch(t[n]);"
"	local "
"	if (t[n] && maybe_unify(e, t[n]))"
"		res[pos++] = n;"
"}";
#endif
prover::prover() {
#ifdef OPENCL
	initcl();
#endif
}

#ifdef OPENCL
void prover::initcl() {
	cl_int err = CL_SUCCESS;
	try {
		cl::Platform::get(&platforms);
		if (platforms.size() == 0) {
			derr << "Platform size: 0" << std::endl;
			exit(-1);
		}
		cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};
		context = cl::Context (CL_DEVICE_TYPE_CPU, properties); 
		devices = context.getInfo<CL_CONTEXT_DEVICES>();
		prog = cl::Program(clprog);
		cl::Kernel kernel(prog, "match", &err);
		cl::Event event;
		cq = cl::CommandQueue (context, devices[0], 0, &err);
		cq.enqueueNDRangeKernel( kernel, cl::NullRange, cl::NDRange(4,4), cl::NullRange, NULL, &event); 
		event.wait();
	} catch (cl::Error err) { derr << err.what() << ':' << err.err() << std::endl; }
}
#endif 
