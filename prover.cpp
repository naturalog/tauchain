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
#include <dlfcn.h>

using namespace boost::algorithm;
int _indent = 0;

etype littype(string s);
string littype(etype s);

const uint max_terms = 1024 * 1024;

bool prover::hasvar(termid id) {
	const term p = get(id);
	setproc(L"hasvar");
	if (p.p < 0) return true;
	if (!p.s && !p.o) return false;
	return hasvar(p.s) || hasvar(p.o);
}

prover::termid prover::evaluate(termid id, const subst& s) {
	if (!id) return 0;
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
//	} else if (islist(_s)) {
//		TRACE( dout << "List unification "; printterm_substs(_s, ssub); dout<<" with "; printterm_substs(_d, dsub); dout <<endl);
//		if (s.p != d.p || !islist(_d)) r = false;
//		else r = unify(s.o, ssub, d.o, dsub, f);
	}
	else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o))
		r = false;
	else
		r = !s.s || (unify(s.s, ssub, d.s, dsub, f) && unify(s.o, ssub, d.o, dsub, f));
	TRACE(
		dout << "Trying to unify ";
		printterm_substs(_s, ssub);
		dout<<" with ";
		printterm_substs(_d, dsub);
		dout<<" : ";
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

prover::termid prover::tmpvar() {
	static int last = 1;
	return make(mkiri(pstr(string(L"?__v")+_tostr(last++))),0,0);
}

prover::termid prover::list_next(prover::termid cons, proof& p) {
	if (!cons) return 0;
	setproc(L"list_next");
	termset ts;
	ts.push_back(make(rdfrest, cons, tmpvar()));
	(*this)( ts , &p.s);
	if (e.find(rdfrest) == e.end()) return 0;
	termid r = 0;
	for (auto x : e[rdfrest])
		if (get(x.first).s == cons) {
			r = get(x.first).o;
			break;
		}
	TRACE(dout <<"current cons: " << format(cons)<< " next cons: " << format(r) << std::endl);
	return r;
}

prover::termid prover::list_first(prover::termid cons, proof& p) {
	if (!cons || get(cons).p == rdfnil) return 0;
	setproc(L"list_first");
	termset ts;
	ts.push_back(make(rdffirst, cons, tmpvar()));
	(*this)( ts , &p.s);
	if (e.find(rdffirst) == e.end()) return 0;
	termid r = 0;
	for (auto x : e[rdffirst])
		if (get(x.first).s == cons) {
			r = get(x.first).o;
			break;
		}
	TRACE(dout<<"current cons: " << format(cons) << " next cons: " << format(r) << std::endl);
	return r;
}

uint64_t dlparam(const node& n) {
	uint64_t p;
	if (!n.datatype || !n.datatype->size() || *n.datatype == *XSD_STRING) {
		p = (uint64_t)n.value->c_str();
	} else {
		const string &dt = *n.datatype, &v = *n.value;
		if (dt == *XSD_BOOLEAN) p = (lower(v) == L"true");
		else if (dt == *XSD_DOUBLE) {
			double d;
			d = std::stod(v);
			memcpy(&p, &d, 8);
		} else if (dt == *XSD_INTEGER /*|| dt == *XSD_PTR*/)
			p = std::stol(v);
	}
	return p;
}

std::vector<prover::termid> prover::get_list(prover::termid head, proof& p) {
	setproc(L"get_list");
//	evidence e1 = e;
//	e.clear();
	termid t = list_first(head, p);
	std::vector<termid> r;
	TRACE(dout<<"get_list with "<<format(head));
	while (t) {
		r.push_back(t);
		head = list_next(head, p);
		t = list_first(head, p);
	}
//	e = e1;
	TRACE(dout<<" returned " << r.size() << " items: "; for (auto n : r) dout<<format(n)<<' '; dout << std::endl);
	return r;
}

void* testfunc(void* p) {
	derr <<std::endl<< "***** Test func called ****** " << p << std::endl;
	return (void*)(pstr("testfunc_result")->c_str());
//	return 0;
}

string prover::predstr(prover::termid t) { return *dict[get(t).p].value; }
string prover::preddt(prover::termid t) { return *dict[get(t).p].datatype; }

int prover::builtin(termid id, proof* p, std::deque<proof*>& queue) {
	setproc(L"builtin");
	const term t = get(id);
	int r = -1;
	termid i0 = t.s ? evaluate(t.s, p->s) : 0;
	termid i1 = t.o ? evaluate(t.o, p->s) : 0;
	term r1, r2;
	const term *t0 = i0 ? &(r1=get(i0=evaluate(t.s, p->s))) : 0;
	const term* t1 = i1 ? &(r2=get(i1=evaluate(t.o, p->s))) : 0;
	TRACE(	dout<<"called with term " << format(id); 
		if (t0) dout << " subject = " << format(i0);
		if (t1) dout << " object = " << format(i0);
		dout << endl);
	if (t.p == GND) r = 1;
	else if (t.p == logequalTo)
		r = t0 && t1 && t0->p == t1->p ? 1 : 0;
	else if (t.p == lognotEqualTo)
		r = t0 && t1 && t0->p != t1->p ? 1 : 0;
//	else if (t.p == rdffirst && t0 && startsWith(*dict[t0->p].value,L"_:list") && (!t0->s == !t0->o))
//		r = ((!t0->s && !t0->o) || unify(t0->s, p->s, t.o, p->s, true)) ? 1 : 0;
//	else if (t.p == rdfrest && t0 && startsWith(*dict[t0->p].value,L"_:list") && (!t0->s == !t0->o))
//		r = ((!t0->s && !t0->o) || unify(t0->o, p->s, t.o, p->s, true)) ? 1 : 0;
	else if (t.p == rdffirst && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->s, p->s, t.o, p->s, true) ? 1 : 0;
	else if (t.p == rdfrest && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->o, p->s, t.o, p->s, true) ? 1 : 0;
	else if (t.p == _dlopen) {
		if (get(t.o).p > 0) throw std::runtime_error("dlopen must be called with variable object.");
		std::vector<termid> params = get_list(t.s, *p);
		if (params.size() >= 2) {
			void* handle;
			try {
				string f = predstr(params[0]);
				if (f == L"0") handle = dlopen(0, std::stol(predstr(params[1])));
				else handle = dlopen(ws(f).c_str(), std::stol(predstr(params[1])));
			} catch (std::exception ex) { derr << indent() << ex.what() <<std::endl; }
			catch (...) { derr << indent() << L"Unknown exception during dlopen" << std::endl; }
			pnode n = mkliteral(tostr((uint64_t)handle), XSD_INTEGER, 0);
			p->s[get(t.o).p] = make(dict.set(n), 0, 0);
			r = 1;
		}
	}
	else if (t.p == _dlerror) {
		if (get(t.o).p > 0) throw std::runtime_error("dlerror must be called with variable object.");
		auto err = dlerror();
		pnode n = mkliteral(err ? pstr(err) : pstr(L"NULL"), 0, 0);
		p->s[get(t.o).p] = make(dict.set(n), 0, 0);
		r = 1;
	}
	else if (t.p == _dlsym) {
//		if (t1) throw std::runtime_error("dlsym must be called with variable object.");
		if (!t1) {
			std::vector<termid> params = get_list(t.s, *p);
			void* handle;
			if (params.size()) {
				try {
					handle = dlsym((void*)std::stol(predstr(params[0])), ws(predstr(params[1])).c_str());
				} catch (std::exception ex) { derr << indent() << ex.what() <<std::endl; }
				catch (...) { derr << indent() << L"Unknown exception during dlopen" << std::endl; }
				pnode n = mkliteral(tostr((uint64_t)handle), XSD_INTEGER, 0);
				p->s[t1->p] = make(dict.set(n), 0, 0);
				r = 1;
			}
		}
	}
	else if (t.p == _dlclose) {
//		if (t1->p > 0) throw std::runtime_error("dlclose must be called with variable object.");
		pnode n = mkliteral(tostr(ws(dlerror())), 0, 0);
		p->s[t1->p] = make(dict.set(mkliteral(tostr(dlclose((void*)std::stol(*dict[t.p].value))), XSD_INTEGER, 0)), 0, 0);
		r = 1;
	}
	else if (t.p == _invoke) {
		typedef void*(*fptr)(void*);
		auto params = get_list(t.s, *p);
		if (params.size() != 2) return -1;
		if (preddt(params[0]) != *XSD_INTEGER) return -1;
		fptr func = (fptr)std::stol(predstr(params[0]));
		void* res;
		if (params.size() == 1) {
			res = (*func)((void*)dlparam(dict[*get_list(params[1],*p).begin()]));
			pnode n = mkliteral(tostr((uint64_t)res), XSD_INTEGER, 0);
			p->s[get(t.o).p] = make(dict.set(n), 0, 0);
		}
		r = 1;
	}
	else if ((t.p == A || t.p == rdfsType || t.p == rdfssubClassOf) && t.s && t.o) {
		termset ts(2);
		termid va = tmpvar();
		ts[0] = make ( rdfssubClassOf, va, t.o );
		ts[1] = make ( A, t.s, va );
		queue.push_front(new proof( kb.add(make ( A, t.s, t.o ), ts, this), 0, p, subst(), p->g)/*, queue, false*/);
	}
	if (r == 1) {
		proof* r = new proof;
		*r = *p;
		r->g = p->g;
		r->g.emplace_back(kb.add(evaluate(id, p->s), termset(), this), subst());
		++r->last;
		step(r, queue);
	}

	return r;
}

bool prover::maybe_unify(const term s, const term d) {
//	return true;
//	std::deque<const term*>
	setproc(L"maybe_unify");
	bool r = (s.p < 0 || d.p < 0) ? true : (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) ? false :
		(!s.s || (maybe_unify(get(s.s), get(d.s)) && maybe_unify(get(s.o), get(d.o))));
	TRACE(dout<<format(s) << ' ' <<format(d) << (r?L"match":L"mismatch") << endl);
	return r; 
}

std::set<uint> prover::match(termid _e) {
	if (!_e) return std::set<uint>();
	setproc(L"match");
	std::set<uint> m;
	termid h;
	const term e = get(_e);
	for (uint n = 0; n < kb.size(); ++n)
		if (((h=kb.head()[n]))&&(h!=_e)&& maybe_unify(e, get(h)))
			m.insert(n);
//	TRACE(dout<<format(_e) << L" matches: "; for (auto x : m) dout << format(kb.head()[x]) << L' '; dout << endl);
	return m;
}

void prover::step(proof* p, std::deque<proof*>& queue, bool) {
	setproc(L"step");
	++steps;
	TRACE(dout<<"popped frame:\n";printp(p));
	if (p->last != kb.body()[p->rul].size()) {
		if (euler_path(p, kb.head()[p->rul])) return;
		termid t = kb.body()[p->rul][p->last];
		if (!t) throw 0;
		TRACE(dout<<"Tracking back from " << format(t) << std::endl);
		if (builtin(t, p, queue) != -1) return;
		for (auto rl : match(evaluate(t, p->s))) {
			subst s;
			if (!unify(t, p->s, kb.head()[rl], s, true)) continue;
			proof* r = new proof(rl, 0, p, s, p->g);
			if (kb.body()[rl].empty()) r->g.emplace_back(rl, subst());
			queue.push_front(r);
		}
	}
	else if (!p->prev) {
		for (auto r = kb.body()[p->rul].begin(); r != kb.body()[p->rul].end(); ++r) {
			substs.push_back(p->s);
			termid t = evaluate(*r, p->s);
			if (!t || hasvar(t)) continue;
			TRACE(dout << "pushing evidence: " << format(t) << std::endl);
			e[get(t).p].emplace(t, p->g);
		}
	} else {
		ground g = p->g;
		if (!kb.body()[p->rul].empty())
			g.emplace_back(p->rul, p->s);
		proof* r = new proof(*p->prev);
		r->g = g;
		unify(kb.head()[p->rul], p->s, kb.body()[r->rul][r->last], r->s, true);
		++r->last;
		step(r, queue);
	}
	TRACE(dout<<"Deleting frame: " << std::endl; printp(p));
//	if (del) delete p;
}

prover::termid prover::list2term(std::list<pnode>& l) {
	setproc(L"list2term");
	termid t;
	if (l.empty()) t = make(Dot, 0, 0);
	else {
		pnode x = l.front();
		l.pop_front();
//		TRACE(dout << x->tostring() << endl);
		auto it = quads.second.find(*x->value);
		if (it == quads.second.end()) t = make(Dot, make(dict.set(x), 0, 0), list2term(l));
		else {
			auto ll = it->second;
			t = make(Dot, list2term(ll), list2term(l));
		}
	}
	TRACE(dout << format(t) << endl);
	return t;
}

prover::termid prover::quad2term(const quad& p) {
	setproc(L"quad2term");
	TRACE(dout<<L"called with: "<<p.tostring()<<endl);
	termid t, s, o;
	if (dict[p.pred] == rdffirst || dict[p.pred] == rdfrest) return 0;
	auto it = quads.second.find(*p.subj->value);
	if (it != quads.second.end()) {
		auto l = it->second;
		s = list2term(l);
	}
	else
		s = make(p.subj, 0, 0);
	if ((it = quads.second.find(*p.object->value)) != quads.second.end()) {
		auto l = it->second;
		o = list2term(l);
	}
	else
		o = make(p.object, 0, 0);
	t = make(p.pred, s, o);
	TRACE(dout<<"quad: " << p.tostring() << " term: " << format(t) << endl);
	return t;
}

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q.first ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}

void prover::operator()(qlist query, const subst* s) {
	termset goal;
	termid t;
	for ( auto q : query ) 
		if (dict[q->pred] != rdffirst && dict[q->pred] != rdfrest)
			if ((t = quad2term( *q ))) goal.push_back( t );
	return (*this)(goal, s);
}

prover::prover(prover::ruleset* _kb) : kb(_kb ? *_kb : *new ruleset) {
	kbowner = !_kb;
	_terms.reserve(max_terms);
	CL(initcl());
}

prover::~prover() { }

//bool prover::islist(pquad q) { return startsWith(*dict[get(t).p].value,L"_:list"); }

void prover::addrules(pquad q) {
	setproc(L"addrules");
	TRACE(dout<<q->tostring()<<endl);
	const string &s = *q->subj->value, &p = *q->pred->value, &o = *q->object->value;
	if (dict[q->pred] == rdffirst || dict[q->pred] == rdfrest) return;
	termid t;
	if ( p != implication/* || quads.first.find ( o ) == quads.first.end()*/ )
		if ((t = quad2term(*q))) 
			kb.add(t, termset(), this);
	if (p == implication)
		for ( pquad y : *quads.first.at ( o ) ) {
			termset ts;
			if ( quads.first.find ( s ) != quads.first.end() )
				for ( pquad z : *quads.first.at(s) )
					if (dict[z->pred] != rdffirst && dict[z->pred] != rdfrest)
						if ((t = quad2term(*z))) 
							ts.push_back( t );
			if ((t = quad2term(*y))) kb.add(t, ts, this);
		}
}
/*
qdb lists(const qdb& kb) {
	std::map<string, std::map<node, std::map<node, pquad>>> qs;
	std::map<string, std::list<pnode>> ls;
	auto isls = [](pnode n){ return n->_type == node::BNODE && startsWith(*n->value,L"_:list"); };
	auto first = [&](node n, string g){ return qs[g][dict[rdffirst]][n]->object; };
	auto rest = [&](node n, string g){ return qs[g][dict[rdfrest]][n]->object; };
	for (auto x : kb)
		for (auto q : *x.second)
			qs[x.first][*q->pred][*q->subj] = q;
	for (auto x : kb)
		for (auto q : *x.second)
			if (isls(q->subj)) {
				ls[x.first].push_back(q->subj);
				pquad _q;
				for (_q != rdfnil) {
					ls[x.first].push_back(first(*q->subj, x.first));
					_q = rest(*q->subj, x.first);
					
				}
			}
}
*/
prover::prover ( qdb qkb ) : quads(qkb) {
	auto it = qkb.first.find(L"@default");
	if (it == qkb.first.end()) throw std::runtime_error("Error: @default graph is empty.");
	for ( pquad quad : *it->second ) addrules(quad);
}
#include <chrono>
void prover::operator()(termset& goal, const subst* s) {
	proof* p = new proof;
	std::deque<proof*> queue;
	p->rul = kb.add(0, goal, this);
	p->last = 0;
	p->prev = 0;
	if (s) p->s = *s;
	TRACE(dout << KRED << "Rules:\n" << formatkb() << KGRN << "Query: " << format(goal) << KNRM << std::endl);
	queue.push_front(p);
	using namespace std;
	using namespace std::chrono;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	do {
		proof* q = queue.back();
		queue.pop_back();
		step(q, queue);
	} while (!queue.empty());
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>( t2 - t1 ).count();
	TRACE(dout << KWHT << "Evidence:" << endl;printe();/* << ejson()->toString()*/ dout << KNRM);
	dout << "elapsed: " << (duration / 1000.) << "ms steps: " << steps << endl;
//	return results();
}

prover::term::term(resid _p, termid _s, termid _o) : p(_p), s(_s), o(_o) {}

const prover::term& prover::get(termid id) const {
	if (!id || id > (termid)_terms.size()) throw std::runtime_error("invalid term id passed to prover::get");
	return _terms[id - 1]; 
}

prover::termid prover::make(pnode p, termid s, termid o) { 
	return make(dict.set(*p), s, o); 
}

prover::termid prover::make(resid p, termid s, termid o) {
#ifdef DEBUG
	if (!p) throw 0;
#endif
	_terms.emplace_back(p, s, o);
	return _terms.size();
}

uint prover::ruleset::add(termid t, const termset& ts, prover* p) {
	_head.push_back(t);
	_body.push_back(ts);
	TRACE(if (!ts.size() && !p->get(t).s) throw 0);
	return _head.size()-1;
}
/*
void* node::toptr() const {
	if (!datatype || *datatype == *XSD_STRING) return (void*)value->c_str();
	string s = *datatype;
	int* r = new int;
	if (s == *XSD_BOOLEAN) return ? L"true" : L"false";
	if (s == *XSD_DOUBLE) return &(*r = (int)std::stod(*value));
	if (s == *XSD_INTEGER) return &(*r = (int)std::stol(*value));
	if (s == *XSD_FLOAT) return &(*r = (int)std::stof(*value));
	delete r;
	return (void*)value->c_str();
*/
//	return 0;
//if (s == *XSD_DECIMAL) 
//if (s == *XSD_ANYURI) 
//	if (s == *XSD_STRING)
//	throw wruntime_error(L"Unidentified literal type" + s);
//

/*
pstring littype(etype s) {
	if (s == BOOLEAN) return XSD_BOOLEAN;
	if (s == DOUBLE) return XSD_DOUBLE;
	if (s == INT) return XSD_INTEGER;
	if (s == FLOAT) return XSD_FLOAT;
	if (s == DECIMAL) return XSD_DECIMAL;
	if (s == URISTR) return XSD_ANYURI;
	if (s == STR) return XSD_STRING;
}
*/
bool prover::term::isstr() const { node n = dict[p]; return n._type == node::LITERAL && n.datatype == XSD_STRING; }
prover::term::term() : p(0), s(0), o(0) {}
prover::term::term(const prover::term& t) : p(t.p), s(t.s), o(t.o) {}
prover::term& prover::term::operator=(const prover::term& t) { p = t.p; s = t.s; o = t.o; return *this; }

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

pobj prover::json(const termset& ts) const {
	polist_obj l = mk_olist_obj(); 
	for (termid t : ts) l->LIST()->push_back(get(t).json(*this));
	return l;
}
pobj prover::json(const subst& s) const {
	psomap_obj o = mk_somap_obj();
	for (auto x : s) (*o->MAP())[dstr(x.first)] = get(x.second).json(*this);
	return o;
}
pobj prover::json(ruleid t) const {
	pobj m = mk_somap_obj();
	(*m->MAP())[L"head"] = get(kb.head()[t]).json(*this);
	(*m->MAP())[L"body"] = json(kb.body()[t]);
	return m;
};
pobj prover::json(const ground& g) const {
	pobj l = mk_olist_obj();
	for (auto x : g) {
		psomap_obj o = mk_somap_obj();
		(*o->MAP())[L"src"] = json(x.first);
		(*o->MAP())[L"env"] = json(x.second);
		l->LIST()->push_back(o);
	}
	return l;
}
pobj prover::ejson() const {
//	typedef map<resid, set<std::pair<termid, ground>>> evidence;
	pobj o = mk_somap_obj();
	for (auto x : e) {
		polist_obj l = mk_olist_obj();
		for (auto y : x.second) {
			psomap_obj t = mk_somap_obj(), t1;
			(*t->MAP())[L"head"] = get(y.first).json(*this);
			(*t->MAP())[L"body"] = t1 = mk_somap_obj();
			(*t1->MAP())[L"pred"] = mk_str_obj(L"GND");
			(*t1->MAP())[L"args"] = json(y.second);
			l->LIST()->push_back(t);
		}
		(*o->MAP())[dstr(x.first)] = l;
	}
	return o;
}
//evidence[t.pred].push({head:t, body:[{pred:'GND', args:c.ground}]})
//g.push({src:rl, env:{}})
//e[node]: [ { (rule) "head":term, "body": { "pred":"GND", args: [ {"src":(rule), "env":(subst)  ] }, {...}  ]
//	typedef boost::container::list<std::pair<ruleid, subst>> ground;
