#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
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
#ifdef with_marpa
#include "marpa_tau.h"
#include <fstream>
#endif

#ifdef with_marpa
#define MARPA(x) x
#else
#define MARPA(x)
#endif

using namespace boost::algorithm;
int _indent = 0;

bool prover::hasvar(termid id) {
	const term p = get(id);
	setproc(L"hasvar");
	if (p.p < 0) return true;
	if (!p.s && !p.o) return false;
	return hasvar(p.s) || hasvar(p.o);
}

termid prover::evaluate(termid id, const subst& s) {
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
	if (!_d != !_s) return false;
	const term s = get(_s), d = get(_d);
	bool r, ns = false;
	if (s.p < 0) r = (v = evaluate(_s, ssub)) ? unify(v, ssub, _d, dsub, f) : true;
	else if (d.p < 0) {
		if ((v = evaluate(_d, dsub))) r = unify(_s, ssub, v, dsub, f);
		else {
			if (f) {
				dsub[d.p] = evaluate(_s, ssub);
				ns = true;
			}
			r = true;
		}
	}
	else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) r = false;
	else r = !s.s || (unify(s.s, ssub, d.s, dsub, f) && unify(s.o, ssub, d.o, dsub, f));
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

bool prover::euler_path(shared_ptr<proof> p) {
	auto ep = p;
	termid t = kb.head()[p->rul];
	uint l = 0;
	while ((ep = ep->prev))
		if (ep->rul == p->rul && unify(kb.head()[ep->rul], ep->s, t, p->s, false))
			{ TRACE(dout<<"Euler path detected\n"); return true; } else ++l;
	TRACE(dout<<"depth: " << l << endl)
	
//	for (auto ep : queue)
//		if (ep != p && (ep->rul == t || unify(kb.head()[ep->rul], ep->s, t, p->s, false)))
//			{ TRACE(dout<<"Euler path detected\n"); return true; }
	return false;
}

termid prover::tmpvar() {
	static int last = 1;
	return make(mkiri(pstr(string(L"?__v")+_tostr(last++))),0,0);
}
void prover::get_dotstyle_list(termid id, std::list<resid> &list) {
	auto s = get(id).s;
	if (!s) return;
	list.push_back(get(s).p);
	get_dotstyle_list(get(id).o, list);
	return;
}

void* testfunc(void* p) {
	derr <<std::endl<< "***** Test func called ****** " << p << std::endl;
	return (void*)(pstr("testfunc_result")->c_str());
}

string prover::predstr(termid t) { return *dict[get(t).p].value; }
string prover::preddt(termid t) { return *dict[get(t).p].datatype; }

int prover::builtin(termid id, shared_ptr<proof> p, queue_t& queue) {
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
		if (t1) dout << " object = " << format(i1);
		dout << endl);
	if (t.p == GND) r = 1;
	else if (t.p == logequalTo)
		r = t0 && t1 && t0->p == t1->p ? 1 : 0;
	else if (t.p == lognotEqualTo)
		r = t0 && t1 && t0->p != t1->p ? 1 : 0;
	else if (t.p == rdffirst && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->s, p->s, t.o, p->s, true) ? 1 : 0;
	else if (t.p == rdfrest && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->o, p->s, t.o, p->s, true) ? 1 : 0;
	else if ((t.p == A || t.p == rdfsType || t.p == rdfssubClassOf) && t.s && t.o) {
		//termset ts(2,0,*alloc);
		termset ts(2);
		termid va = tmpvar();
		ts[0] = make ( rdfssubClassOf, va, t.o );
		ts[1] = make ( A, t.s, va );
		queue.push_front(make_shared<proof>(kb.add(make ( A, t.s, t.o ), ts), 0, p, subst(), p->g));
	}
	#ifdef with_marpa
	else if (t.p == marpa_parser_iri)// && !t.s && t.o) //fixme
	/* ?X is a parser created from grammar */
	{
		void* handle = marpa_parser(this, get(t.o).p, p);
		pnode n = mkliteral(tostr((uint64_t)handle), XSD_INTEGER, 0);
		(*p->s)[get(t.s).p] = make(dict.set(n), 0, 0);
		r = 1;
	}
	else if (t.p == file_contents_iri) {
		if (get(t.s).p > 0) throw std::runtime_error("file_contents must be called with variable subject.");
		string fn = *dict[get(t.o).p].value;
		std::string fnn = ws(fn);
	    std::ifstream f(fnn);
	    if (f.is_open())
		{
			(*p->s)[get(t.s).p] = make(mkliteral(pstr(load_file(f)), 0, 0));
			r = 1;
		}
	}
	else if (t.p == marpa_parse_iri) {
	/* ?X is a parse of (input with parser) */
		if (get(t.s).p > 0) throw std::runtime_error("marpa_parse must be called with variable subject.");
		term xx = get(i1);
		term xxx = get(xx.s);
		string input = *dict[xxx.p].value;
		string marpa = *dict[get(get(get(i1).o).s).p].value;
		termid result = marpa_parse((void*)std::stol(marpa), input);
		dout << L"result2: " << format(result) << std::endl;
		(*p->s)[get(t.s).p] = result;
		r = 1;
	}
	#endif
	if (r == 1) {
		shared_ptr<proof> r = make_shared<proof>();
		*r = *p;
		r->g = p->g;
		r->g.emplace_back(kb.add(evaluate(id, p->s), termset()), subst());
		++r->last;
		queue.push_back(r);
	}

	return r;
}

void prover::step(std::shared_ptr<proof> p, queue_t& queue, bool) {
	setproc(L"step");
	++steps;
	TRACE(dout<<"popped frame:\n";printp(p));
	if (p->last != kb.body()[p->rul].size()) {
		if (euler_path(p)) return;
		termid t = kb.body()[p->rul][p->last];
		TRACE(dout<<"Tracking back from " << format(t) << std::endl);
		#ifdef with_marpa
		if (builtin(t, p, queue) != -1) return;
		#endif
		for (auto rl : kb[get(t).p]) {
			subst s;
			if (unify(t, p->s, kb.head()[rl], s, true)) {
				shared_ptr<proof> r = make_shared<proof>(rl, 0, p, s, p->g);
				if (kb.body()[rl].empty()) r->g.emplace_back(rl, subst());
				queue.push_front(r);
			}
		}
	}
	else if (!p->prev) {
		termid t;
		for (auto r : kb.body()[p->rul]) {
			MARPA(substs.push_back(*p->s));
			if (!(t = evaluate(r, p->s))) continue;
			e[get(t).p].emplace(t, p->g);
			dout << "proved: " << format(t) << endl;
			for (auto x = queue.begin(); x != queue.end(); ++x) {
				auto y = *x;
				while ((y = y->prev))
					if (&*y == &*p) {
						queue.erase(x);
						dout<<L"<= <="<<endl;
					}
			}
		}
	} else {
		shared_ptr<proof> r = make_shared<proof>(*p->prev);
		r->g = p->g;
		r->s = subst(p->prev->s);
		if (!kb.body()[p->rul].empty()) r->g.emplace_back(p->rul, p->s);
		unify(kb.head()[p->rul], p->s, kb.body()[r->rul][r->last], r->s, true);
		++r->last;
		queue.push_back(r);
	}
	TRACE(dout<<"Deleting frame: " << std::endl; printp(p));
}

termid prover::list2term_simple(std::list<termid>& l) {
	setproc(L"list2term_simple");
	termid t;
	if (l.empty())
		t = make(Dot, 0, 0);
	else {
		termid x = l.front();
		l.pop_front();
		t = make(Dot, x, list2term_simple(l));
	}
	TRACE(dout << format(t) << endl);
	return t;
}

termid prover::list2term(std::list<pnode>& l, const qdb& quads) {
	setproc(L"list2term");
	termid t;
	if (l.empty()) t = make(Dot, 0, 0);
	else {
		pnode x = l.front();
		l.pop_front();
//		TRACE(dout << x->tostring() << endl);
		auto it = quads.second.find(*x->value);
		//item is not a list
		if (it == quads.second.end())
			t = make(Dot, make(dict.set(x), 0, 0), list2term(l, quads));
		//item is a list
		else {
			auto ll = it->second;
			t = make(Dot, list2term(ll, quads), list2term(l, quads));
		}
	}
	TRACE(dout << format(t) << endl);
	return t;
}

termid prover::quad2term(const quad& p, const qdb& quads) {
	setproc(L"quad2term");
	TRACE(dout<<L"called with: "<<p.tostring()<<endl);
	termid t, s, o;
	auto it = quads.second.find(*p.subj->value);
	if (it != quads.second.end()) {
		auto l = it->second;
		s = list2term(l, quads);
	}
	else
		s = make(p.subj, 0, 0);
	if ((it = quads.second.find(*p.object->value)) != quads.second.end()) {
		auto l = it->second;
		o = list2term(l, quads);
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
/*


*/
void prover::operator()(const qdb& query, subst* s) {
	termset goal = termset();
	termid t;
	for ( auto q : merge(query) ) 
		if (	dict[q->pred] != rdffirst && 
			dict[q->pred] != rdfrest &&
			(t = quad2term( *q, query )) )
			goal.push_back( t );
	return (*this)(goal, s);
}
/*



*/
prover::~prover() { }
prover::prover(const prover& q) : kb(q.kb), _terms(q._terms) { kb.p = this; } 

void prover::addrules(pquad q, qdb& quads) {
	setproc(L"addrules");
	TRACE(dout<<q->tostring()<<endl);
	const string &s = *q->subj->value, &p = *q->pred->value, &o = *q->object->value;
	termid t;
	TRACE(dout<<"called with " << q->tostring()<<endl);
	if (p == implication) {
		if (quads.first.find(o) == quads.first.end()) quads.first[o] = mk_qlist();
		for ( pquad y : *quads.first.at ( o ) ) {
			if ( quads.first.find ( s ) == quads.first.end() ) continue;
			termset ts = termset();
			for ( pquad z : *quads.first.at( s ) )
				if ((dict[z->pred] != rdffirst && 
					dict[z->pred] != rdfrest) &&
					(t = quad2term(*z, quads)))
					ts.push_back( t );
			if ((t = quad2term(*y, quads))) kb.add(t, ts);
		}
	} else if ((t = quad2term(*q, quads))) kb.add(t, termset());
}

prover::prover ( qdb qkb, bool check_consistency ) : kb(this) {
	auto it = qkb.first.find(L"@default");
	if (it == qkb.first.end()) throw std::runtime_error("Error: @default graph is empty.");
	if (qkb.first.find(L"false") == qkb.first.end()) qkb.first[L"false"] = make_shared<qlist>();
	for ( pquad quad : *it->second ) addrules(quad, qkb);
	if (check_consistency && !consistency(qkb)) throw std::runtime_error("Error: inconsistent kb");
}

bool prover::consistency(const qdb& quads) {
	setproc(L"consistency");
	bool c = true;
	prover p(*this);
	termid t = p.make(mkiri(pimplication), p.tmpvar(), p.make(False, 0, 0));
	termset g = termset();
	g.push_back(t);
	p(g);
	auto ee = p.e;
	for (auto x : ee) for (auto y : x.second) {
		prover q(*this);
		g.clear();
		string s = *dict[p.get(p.get(y.first).s).p].value;
		if (s == L"GND") continue;
		TRACE(dout<<L"Trying to prove false context: " << s << endl);
		qdb qq;
		qq.first[L""] = quads.first.at(s);
		q(qq);
		if (q.e.size()) {
			derr << L"Inconsistency found: " << q.format(y.first) << L" is provable as true and false."<<endl;
			c = false;
		}
	}
	return c;
}





/*


*/
#include <chrono>
void prover::operator()(termset& goal, subst* s) {
	queue_t queue;
	shared_ptr<proof> p = make_shared<proof>();
	p->rul = kb.add(0, goal);
	p->last = 0;
	p->prev = 0;
	if (s) p->s = *s;
	#ifdef with_marpa
	TRACE(dout << KGRN << "Query: " << format(goal) << KNRM << std::endl);
	#else
	TRACE(dout << KRED << L"Rules:\n" << formatkb()<<endl<< KGRN << "Query: " << format(goal) << KNRM << std::endl);
	TRACE(dout << KRED << L"Rules:\n" << kb.format()<<endl<< KGRN << "Query: " << format(goal) << KNRM << std::endl);
	#endif
	queue.push_front(p);
	std::shared_ptr<proof> q;
	using namespace std;
	using namespace std::chrono;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	do {
		q = queue.back();
		queue.pop_back();
		step(q, queue);
		if (steps % 1000 == 0) (dout << "step: " << steps << endl);
	} while (!queue.empty() && steps < 2e+7);
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>( t2 - t1 ).count();
	TRACE(dout << KWHT << "Evidence:" << endl;printe();/* << ejson()->toString()*/ dout << KNRM);
	#ifndef with_marpa
	/*TRACE*/(dout << "elapsed: " << (duration / 1000.) << "ms steps: " << steps << endl);
	#endif//hate this part, because naturalog has this like ..umm..the main function of tau, printing out the rules at the top and time and everything, while i call this from marpa over and over to retrieve facts about the grammar:) ah i see :) yes i mean you can just look at it and see its too mixed up//yeah..well..thats just some food for thought for you.. hehe believe me i've got my eye on this :)ah cool
}
/*


*/
//btw


prover::term::term(resid _p, termid _s, termid _o) : p(_p), s(_s), o(_o) {}

const prover::term& prover::get(termid id) const {
	TRACE(if (!id || id > (termid)_terms.size())
		throw std::runtime_error("invalid term id passed to prover::get"));
	return _terms[id - 1]; 
}

termid prover::make(pnode p, termid s, termid o) { 
	return make(dict.set(*p), s, o); 
}

termid prover::make(resid p, termid s, termid o) {
#ifdef DEBUG
	if (!p) throw 0;
#endif
	return _terms.add(p, s, o);
}

prover::ruleid prover::ruleset::add(termid t, const termset& ts) {
	setproc(L"ruleset::add");
	ruleid r =  _head.size();
	_head.push_back(t);
	_body.push_back(ts);
	r2id[t ? p->get(t).p : 0].push_back(r);
	TRACE(dout<<r<<tab<<p->formatr(r)<<endl);
	return r;
}

prover::ruleid prover::ruleset::add(termid t) {
	termset ts = termset();
	return add(t, ts);
}


prover::term::term() : p(0), s(0), o(0) {}

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
}

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


string prover::ruleset::format() const {
	setproc(L"ruleset::format");
	std::wstringstream ss;
	ss << L'['<<endl;
	for (auto it = r2id.begin(); it != r2id.end();) {
		ss <<tab<< L'{' << endl <<tab<<tab<<L'\"'<<(it->first ? *dict[it->first].value : L"")<<L"\":[";
		for (auto iit = it->second.begin(); iit != it->second.end();) {
			ss << p->formatr(*iit, true);
			if (++iit != it->second.end()) ss << L',';
			ss << endl;
		}
		ss << L"]}";
		if (++it != r2id.end()) ss << L',';
	}
	ss << L']';
	return ss.str();
}
