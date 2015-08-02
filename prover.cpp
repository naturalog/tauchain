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
#define ISVAR(term) ((term.p < 0))

prover::term::term() : p(0), s(0), o(0) {}

termid prover::evaluate(const term& p, termid id) {
	if (!id) return 0;
	setproc(L"evaluate");
	termid r;
	if (ISVAR(p)) return 0;
	if (!p.s && !p.o) return id;
	termid a = evaluate(p.s), b = evaluate(p.o);
	return make(p.p, a ? a : make(get(p.s).p), b ? b : make(get(p.o).p));
}

termid prover::evaluate(const term& p, termid id, const subst& s) {
	if (!id) return 0;
	setproc(L"evaluate");
	termid r;
	if (ISVAR(p)) {
		auto it = s.find(p.p);
		r = it == s.end() ? 0 : evaluate(it->second, s);
	} else if (!p.s && !p.o)
		r = id;
	else {
		termid a = evaluate(p.s, s), b = evaluate(p.o, s);
		r = make(p.p, a ? a : make(get(p.s).p), b ? b : make(get(p.o).p));
	}
	TRACE(dout<<format(id) << ' ' << formats(s)<< " = " << format(r) << endl);
	return r;
}

bool prover::unify(const prover::term& s, termid _s, const subst& ssub, const prover::term& d, termid _d, subst& dsub, bool f) {
	setproc(L"unify");
	termid v;
	bool r, ns = false;
	if (ISVAR(s)) r = (v = evaluate(s, _s, ssub)) ? unify(v, ssub, d, _d, dsub, f) : true;
	else if (ISVAR(d)) {
		if ((v = evaluate(d, _d, dsub))) r = unify(s, _s, ssub, v, dsub, f);
		else {
			if (f) {
				dsub[d.p] = evaluate(s, _s, ssub);
				ns = true;
			}
			r = true;
		}
	}
	else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) r = false;
	else if (!s.s) r = true;
	else if ((r = unify(s.s, ssub, d.s, dsub, f)))
		r = unify(s.o, ssub, d.o, dsub, f);
	if (f) {
		TRACE(dout << "Trying to unify " << format(_s) << " sub: " << formats(ssub)
			  << " with " << format(_d) << " sub: " << formats(dsub) << " : ";
		if (r) {
			dout << "passed";
			if (ns) dout << " with new substitution: " << dstr(d.p) << " / " << format(dsub[d.p]);
		} else dout << "failed";
		dout << endl);
	}
	return r;
}

bool prover::unify(const prover::term& s, termid _s, const prover::term& d, termid _d, subst& dsub, bool f) {
	setproc(L"unify");
	termid v;
	bool r, ns = false;
	if (ISVAR(s)) r = true;
	else if (ISVAR(d)) {
		if ((v = evaluate(d, _d, dsub))) r = unify(s, _s, v, dsub, f);
		else {
			if (f) {
				dsub[d.p] = evaluate(s, _s);
				ns = true;
			}
			r = true;
		}
	}
	else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) r = false;
	else if (!s.s) r = true;
	else if ((r = unify(s.s, d.s, dsub, f)))
		r = unify(s.o, d.o, dsub, f);
	if (f) {
		TRACE(dout << "Trying to unify " << format(_s) << " sub: " 
			  << " with " << format(_d) << " sub: " << formats(dsub) << " : ";
		if (r) {
			dout << "passed";
			if (ns) dout << " with new substitution: " << dstr(d.p) << " / " << format(dsub[d.p]);
		} else dout << "failed";
		dout << endl);
	}
	return r;
}

bool prover::euler_path(shared_ptr<proof>& _p) {
	setproc(L"euler_path");
	auto ep = _p;
	proof& p = *_p;
	termid t = head[p.rul];
	while ((ep = ep->prev))
		if (ep->rul == p.rul && unify(head[ep->rul], *ep->s, t, *p.s, false))
			{ TRACE(dout<<"Euler path detected\n"); return true; }
	return ep != 0;
}

termid prover::tmpvar() {
	static int last = 1;
	return make(mkiri(pstr(string(L"?__v")+_tostr(last++))),0,0);
}


termid prover::list_next(termid cons, proof& p) {
	if (!cons) return 0;
	setproc(L"list_next");
	termset ts;
	ts.push_back(make(rdfrest, cons, tmpvar()));
	query( ts, &*p.s);
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

termid prover::list_first(termid cons, proof& p) {
	if (!cons || get(cons).p == rdfnil) return 0;
	setproc(L"list_first");
	termset ts;
	ts.push_back(make(rdffirst, cons, tmpvar()));
	query( ts , &*p.s);
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
	uint64_t p = 0;
	if (!n.datatype || !n.datatype->size() || *n.datatype == *XSD_STRING) {
		p = (uint64_t)n.value->c_str();
	} else {
		const string &dt = *n.datatype, &v = *n.value;
		if (dt == *XSD_BOOLEAN) p = (lower(v) == L"true");
		else if (dt == *XSD_DOUBLE) {
			double d;
			d = std::stod(v);
			memcpy(&p, &d, 8);
		} else if (dt == *XSD_INTEGER)//|| dt == *XSD_PTR)
			p = std::stol(v);
	}
	return p;
}

std::vector<termid> prover::get_list(termid head, proof& p) {
	setproc(L"get_list");
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

void prover::get_dotstyle_list(termid id, std::list<nodeid> &list) {
	auto s = get(id).s;
	if (!s) return;
	list.push_back(get(s).p);
	get_dotstyle_list(get(id).o, list);
	return;
}

void* testfunc(void* p) {
	derr <<std::endl<< "***** Test func called ****** " << p << std::endl;
	return (void*)(pstr("testfunc_result")->c_str());
//	return 0;
}

int prover::builtin(termid id, shared_ptr<proof> p, queue_t& queue) {
	setproc(L"builtin");
	const term t = get(id);
	int r = -1;
	termid i0 = t.s ? evaluate(t.s, *p->s) : 0;
	termid i1 = t.o ? evaluate(t.o, *p->s) : 0;
	term r1, r2;
	const term *t0 = i0 ? &(r1=get(i0=evaluate(t.s, *p->s))) : 0;
	const term* t1 = i1 ? &(r2=get(i1=evaluate(t.o, *p->s))) : 0;
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
		r = unify(t0->s, *p->s, t.o, *p->s, true) ? 1 : -1;
	else if (t.p == rdfrest && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->o, *p->s, t.o, *p->s, true) ? 1 : -1;
/*	else if (t.p == _dlopen) {
		if (get(t.o).p > 0) throw std::runtime_error("dlopen must be called with variable object.");
		std::vector<termid> params = get_list(t.s, *p);
		if (params.size() >= 2) {
			void* handle;
			try {
				string f = predstr(params[0]);
				if (f == L"0") handle = dlopen(0, std::stol(predstr(params[1])));
				else handle = dlopen(ws(f).c_str(), std::stol(predstr(params[1])));
				pnode n = mkliteral(tostr((uint64_t)handle), XSD_INTEGER, 0);
				subs[p->s][get(t.o).p] = make(dict.set(n), 0, 0);
				r = 1;
			} catch (std::exception ex) { derr << indent() << ex.what() <<std::endl; }
			catch (...) { derr << indent() << L"Unknown exception during dlopen" << std::endl; }
		}
	}
	else if (t.p == _dlerror) {
		if (get(t.o).p > 0) throw std::runtime_error("dlerror must be called with variable object.");
		auto err = dlerror();
		pnode n = mkliteral(err ? pstr(err) : pstr(L"NULL"), 0, 0);
		subs[p->s][get(t.o).p] = make(dict.set(n), 0, 0);
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
					pnode n = mkliteral(tostr((uint64_t)handle), XSD_INTEGER, 0);
					p->s[t1->p] = make(dict.set(n), 0, 0);
					r = 1;
				} catch (std::exception ex) { derr << indent() << ex.what() <<std::endl; }
				catch (...) { derr << indent() << L"Unknown exception during dlopen" << std::endl; }
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
	}*/
	else if ((t.p == A || t.p == rdfsType || t.p == rdfssubClassOf) && t.s && t.o) {
		//termset ts(2,0,*alloc);
		termset ts(2);
		termid va = tmpvar();
		ts[0] = make ( rdfssubClassOf, va, t.o );
		ts[1] = make ( A, t.s, va );
		queue.push_front(std::async([&](){return make_shared<proof>(kb.add(make ( A, t.s, t.o ), ts), 0, p, subst()/*, p->g*/);}));
	}
	#ifdef with_marpa
	else if (t.p == marpa_parser_iri)// && !t.s && t.o) //fixme
	/* ?X is a parser created from grammar */
	{
		void* handle = marpa_parser(this, get(t.o).p, p);
		pnode n = mkliteral(tostr((uint64_t)handle), XSD_INTEGER, 0);
		(*p->s)[get(t.s).p] = make(dict.set(n), 0, 0);
		r = 1;
		dout << "ppp";
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
		dout << "pppp";
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
	if (r == 1) 
		queue.push_back(std::async([p, id, this](){
			shared_ptr<proof> r = make_shared<proof>();
			*r = *p;
			r->btterm = evaluate(id, *p->s);
			++r->last;
			return r;
		}));
	return r;
}

void prover::pushev(shared_ptr<proof> p) {
	termid t;
	for (auto r : body[p->rul]) {
		MARPA(substs.push_back(*p->s));
		if (!(t = (p->s ? evaluate(r, *p->s) : evaluate(r)))) continue;
		e[get(t).p].emplace_back(t, p->g(this));
//		dout << "proved: " << format(t) << endl;
	}
}

//void prover::printq(queue_t& q){
//	int n = 0;
//	for (auto p : q) {
//		int pqid = -1;
//		if (p->prev) pqid = (p->prev)->qid;
//		dout << n++ << ") qid: " << p->qid << ", ind: " << p->last << ", pqid: " << pqid << ", env: " << (/*p->s->empty() ? string(L"undefined") :*/ formats(p->s)) << endl;
//	}
//}

void prover::step(shared_ptr<proof>& _p, queue_t& queue) {
	setproc(L"step");
	++steps;
	proof& p = *_p;
	auto rul = body[p.rul];
	if (p.last != rul.size()) {
		if (euler_path(_p)) return;
		termid t = rul[p.last];
		MARPA(if (builtin(t, _p, queue) != -1) return);
		auto it = kb.r2id.find(get(t).p);
		if (it == kb.r2id.end()) return;
		subst s;
		for (auto rl : it->second) {
			auto& ss = _p->s;
			if (ss ? unify(t, *_p->s, head[rl], s, true) : unify(t, head[rl], s, true))
				queue.push_front(std::async([rl, this, _p, s]() {
					shared_ptr<proof> r = make_shared<proof>(rl, 0, _p, s);
					r->creator = _p;
					return r;
				}));
			s.clear();
		}
	}
	else if (!p.prev) { pushev(_p); }
	else {
		shared_ptr<proof> r = make_shared<proof>(*p.prev);
		ruleid rl = p.rul;
		r->creator = _p;
		auto& ss = p.prev->s;
		if (ss) r->s = make_shared<subst>(*ss);
		else r->s = make_shared<subst>();
		if (p.s) unify(head[rl], *p.s, body[r->rul][r->last], *r->s, true);
		else unify(head[rl], body[r->rul][r->last], *r->s, true);
		++r->last;
		step(r, queue);
	}
}

prover::ground prover::proof::g(prover* p) const {
	if (!creator) return ground();
	ground r = creator->g(p);
	if (btterm) r.emplace_back(p->kb.add(btterm, termset()), make_shared<subst>());
	else if (creator->last != p->body[creator->rul].size()) {
		if (p->body[rul].empty()) r.emplace_back(rul, (shared_ptr<subst>)0);
	} else if (!p->body[creator->rul].empty()) r.emplace_back(creator->rul, creator->s);
	return r;	
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
	#ifndef with_marpa
	if (dict[p.pred] == rdffirst || dict[p.pred] == rdfrest) return 0;
	#endif
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
	p.query(g);
	auto ee = p.e;
	for (auto x : ee) for (auto y : x.second) {
		prover q(*this);
		g.clear();
		string s = *dict[p.get(p.get(y.first).s).p].value;
		if (s == L"GND") continue;
		TRACE(dout<<L"Trying to prove false context: " << s << endl);
		qdb qq;
		qq.first[L""] = quads.first.at(s);
		q.query(qq);
		if (q.e.size()) {
			derr << L"Inconsistency found: " << q.format(y.first) << L" is provable as true and false."<<endl;
			c = false;
		}
	}
	return c;
}

void prover::query(const qdb& q_, subst* s) {
	termset goal = termset();
	termid t;
	for ( auto q : merge(q_) )
		if (	dict[q->pred] != rdffirst &&
			dict[q->pred] != rdfrest &&
			(t = quad2term( *q, q_ )) )
			goal.push_back( t );
	query(goal, s);
}

#include <chrono>
void prover::query(termset& goal, subst* s) {
//	setproc(L"prover()");
	queue_t queue, gnd;
	queue.push_front(std::async([&](){
	shared_ptr<proof> p = make_shared<proof>();
	p->rul = kb.add(0, goal);
	p->last = 0;
	p->prev = 0;
	if (s) p->s = make_shared<subst>(*s);
	return p;
	}));
	
	#ifdef with_marpa
	TRACE(dout << KGRN << "Query: " << format(goal) << KNRM << std::endl);

	#else
	
	TRACE(dout << KRED << L"Rules:\n" << formatkb()<<endl<< KGRN << "Query: " << format(goal) << KNRM << std::endl);
//	TRACE(dout << KRED << L"Rules:\n" << kb.format()<<endl<< KGRN << "Query: " << format(goal) << KNRM << std::endl);
	#endif	
	shared_ptr<proof> q;
	using namespace std;
	using namespace std::chrono;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	do {
		q = queue.back().get();
		queue.pop_back();
//		printq(queue);
		step(q, queue);
		//if (steps % 10000 == 0) (dout << "step: " << steps << endl);
	} while (!queue.empty() && steps < 2e+7);
//	for (auto x : gnd) pushev(x);
	
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>( t2 - t1 ).count();
	dout << KYEL << "Evidence:" << endl;printe();/* << ejson()->toString()*/ dout << KNRM;
	dout << "elapsed: " << (duration / 1000.) << "ms steps: " << steps << endl;
}

prover::term::term(nodeid _p, termid _s, termid _o) : p(_p), s(_s), o(_o) {}

const prover::term& prover::get(termid id) const {
#ifdef DEBUG
	if (!id || id > (termid)_terms.size())
		throw std::runtime_error("invalid term id passed to prover::get");
#endif
	return _terms[id - 1]; 
}

termid prover::make(pnode p, termid s, termid o) { 
	return make(dict.set(*p), s, o); 
}

termid prover::make(nodeid p, termid s, termid o) {
#ifdef DEBUG
	if (!p) throw 0;
#endif
	if (_terms.terms.capacity() - _terms.terms.size() < 10)
		_terms.terms.reserve(5 * _terms.terms.size() / 6);
	return _terms.add(p, s, o);
}

prover::ruleid prover::ruleset::add(termid t, const termset& ts) {
	setproc(L"ruleset::add");
	ruleid r =  _head.size();
	_head.push_back(t);
	_body.push_back(ts);
	r2id[t ? p->get(t).p : 0].push_back(r);
	return r;
}

prover::ruleid prover::ruleset::add(termid t) {
	termset ts = termset();
	return add(t, ts);
}

