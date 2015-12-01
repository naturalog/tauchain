#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <iostream>
#include <ostream>
#include "misc.h"
#include <utility>
#include "json_object.h"
#include "prover.h"
#include <iterator>
#include <forward_list>
#include <boost/algorithm/string.hpp>
#include <dlfcn.h>
#ifdef with_marpa
#include "marpa_tau.h"
#include <fstream>
#endif



#define queuepush(x) { auto y = x; if (lastp) lastp->next = y; lastp = y; }

using namespace boost::algorithm;
int _indent = 0;

term::term() : p(0), s(0), o(0) {}

bool prover::euler_path(shared_ptr<proof> _p) {
	setproc("euler_path");
	auto& ep = _p;
	proof& p = *_p;
	termid t = heads[p.rule];
	if (!t) return false;
	const term& rt = *t;
	if (!p.s.empty()) {
		while ((ep = ep->prev))
			if (ep->rule == p.rule && unify_ep(heads[ep->rule], ep->s, rt, p.s))
				{ TRACE(dout<<"Euler path detected"<<endl); return true; }
	} else while ((ep = ep->prev))
		if (ep->rule == p.rule && unify_ep(heads[ep->rule], ep->s, rt))
			{ TRACE(dout<<"Euler path detected"<<endl); return true; }
	return false;
}

termid prover::tmpvar() {
	static int last = 1;
	return make(mkiri(pstr(string("?__v")+_tostr(last++))),0,0);
}

termid prover::list_next(termid cons, proof& p) {
	if (!cons) return 0;
	setproc("list_next");
	termset ts;
	ts.push_back(make(rdfrest, cons, tmpvar()));
	do_query( ts, &p.s);
	if (e.find(rdfrest) == e.end()) return 0;
	termid r = 0;
	for (auto x : e[rdfrest])
		if (x.first->s == cons) {
			r = x.first->o;
			break;
		}
	TRACE(dout <<"current cons: " << format(cons)<< " next cons: " << format(r) << std::endl);
	return r;
}

termid prover::list_first(termid cons, proof& p) {
	if (!cons || cons->p == rdfnil) return 0;
	setproc("list_first");
	termset ts;
	ts.push_back(make(rdffirst, cons, tmpvar()));
	do_query( ts, &p.s);
	if (e.find(rdffirst) == e.end()) return 0;
	termid r = 0;
	for (auto x : e[rdffirst])
		if (x.first->s == cons) {
			r = x.first->o;
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
		if (dt == *XSD_BOOLEAN) p = (lower(v) == "true");
		else if (dt == *XSD_DOUBLE) {
			double d;
			d = std::stod(v);
			memcpy(&p, &d, 8);
		} else if (dt == *XSD_INTEGER)
			p = std::stol(v);
	}
	return p;
}

std::vector<termid> prover::get_list(termid head, proof* _p) {
	setproc("get_list");
	assert(_p);
	proof& p = *_p;
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
	auto s = id->s;
	if (!s) return;
	list.push_back(s->p);
	get_dotstyle_list(id->o, list);
	return;
}


//Think of these two as part of the same thing. Get the subject from each
//term and put them into a list. List terms are chained together by placing
//the value in the subject, a dot in the predicate, and the next list term
//in the object.
void prover::get_dotstyle_list(termid id, std::vector<termid> &list) {
	auto s = id->s;
	if (!s) return;
	
	list.push_back(s);
	get_dotstyle_list(id->o, list);
	return;
}

std::vector<termid> prover::get_dotstyle_list(termid id) {
	std::vector<termid> r;
	get_dotstyle_list(id, r);
	return r;
}



void* testfunc(void* p) {
	derr <<std::endl<< "***** Test func calleued ****** " << p << std::endl;
	return (void*)(pstr("testfunc_result")->c_str());
//	return 0;
}



int prover::rdfs_builtin(const term& t, const term *t0, const term *t1) {
	//TODO: correctly, so that subqery proof trace not opaque?
	int r = -1;
	//rdfs:Resource(?x)
	if ((t.p == A || t.p == rdftype) && t0 && t1 && t1->p == rdfsResource)
		r = 1;
	else if (t.p == rdfssubClassOf && t1->p == rdfsResource) {
		r = 0;
		// #{?C a rdfs:Class} => {?C rdfs:subClassOf rdfs:Resource}.
		{
			prover copy(*this);
			auto ps = copy.askt(t0, rdftype, make(rdfsClass, 0, 0));
			if (ps.size())
				return 1;
		}
	}
	else if (t.p == rdfssubClassOf) {
		r = 0;
		//#{?B rdfs:subClassOf ?C. ?A rdfs:subClassOf ?B} => {?A rdfs:subClassOf ?C}.
		{
			prover copy(*this);
			auto bs = copy.askt(copy.tmpvar(), rdfssubClassOf, t1);
			for (auto b: bs) {
				prover copy(*this);
				auto xs = copy.askt(t0, rdfssubClassOf, b);
				if (xs.size())
					return 1;
			}
		}
		//#{?X a rdfs:Datatype} => {?X rdfs:subClassOf rdfs:Literal}.
		if (t1->p == rdfsLiteral) {
			{
				prover copy(*this);
				auto ps = copy.askt(t0, rdftype, make(rdfsDatatype, 0, 0));
				if (ps.size())
					return 1;
			}
		}
	}
	else if (t.p == A || t.p == rdftype) {
		r = 0;
		// {?P @has rdfs:domain ?C. ?S ?P ?O} => {?S a ?C}.
		{
			prover copy(*this);
			//TRACE(dout << "{?P @has rdfs:domain ?C. ?S ?P ?O} => {?S a ?C}." << std::endl;)
			auto ps = copy.askt(copy.tmpvar(), rdfsdomain, t1);
			for (termid p: ps) {
				dout << "\n\nYAYdomain!!\n\n" << std::endl;
				auto xx = copy.askt(t0, p->p, copy.tmpvar());
				if (xx.size() > 0) {
					dout << "\n\nYay even more\n\n" << std::endl;
					return 1;
				}
			}
		}
		//{?P @has rdfs:range ?C. ?S ?P ?O} => {?O a ?C}.
		{
			prover copy(*this);
			auto ps = copy.askt(copy.tmpvar(), rdfsrange, make(t1->p, 0, 0));
			dout << "yays:" << ps.size() << std::endl;
			for (termid p: ps) {
				dout << "p:" << p << std::endl;
				dout << "\n\nYAYrange!!\n\n" << std::endl;
				auto xx = copy.askt(copy.tmpvar(), p->p, make(t0->p, 0, 0));
				if (xx.size() > 0) {
					dout << "\n\nYay even more\n\n" << std::endl;
					return 1;
				}
			}
		}
		//#{?A rdfs:subClassOf ?B. ?S a ?A} => {?S a ?B}.
		{
			prover copy(*this);
			auto as = copy.askt(copy.tmpvar(), rdfssubClassOf, t1);
			for (termid a: as) {
				auto xx = copy.askt(t0, rdftype, a);
				if (xx.size() > 0) {
					dout << "\n\nYay even more\n\n" << std::endl;
					return 1;
				}
			}
		}


	}
	else if (t.p == rdfssubPropertyOf) {
		r = 0;
		//#{?X a rdfs:ContainerMembershipProperty} => {?X rdfs:subPropertyOf rdfs:member}.
		if (t1 && t1->p == rdfsmember) {
			prover copy(*this);
			auto ps = copy.askt(copy.tmpvar(), rdftype, make(rdfsContainerMembershipProperty, 0, 0));
			if (ps.size())
				return 1;
		}
		//#{?Q rdfs:subPropertyOf ?R. ?P rdfs:subPropertyOf ?Q} => {?P rdfs:subPropertyOf ?R}.
		{
			prover copy(*this);
			auto qs = copy.askt(copy.tmpvar(), rdfssubPropertyOf, t1);
			for (termid q: qs) {
				auto ps = copy.askt(t0, rdfssubPropertyOf, q);
				if (ps.size())
					return 1;
			}
		}
	}
	//#{?P @has rdfs:subPropertyOf ?R. ?S ?P ?O} => {?S ?R ?O}.
	{
		prover copy(*this);
		auto ps = copy.askt(copy.tmpvar(), rdfssubPropertyOf, make(t.p, 0, 0));
		for (termid p: ps) {
			auto xs = copy.askt(t0, p->p, t1);
			if (ps.size())
				return 1;
		}
	}
	return r;
}



int prover::builtin(termid id, shared_ptr<proof> p) {
	setproc("builtin");
	const term& t = *id;
	int r = -1;
	termid i0 = t.s ? EVALPS(t.s, p->s) : 0;
	termid i1 = t.o ? EVALPS(t.o, p->s) : 0;
	term r1, r2;
	const term *t0 = i0 ? &(r1=*(i0=EVALPS(t.s, p->s))) : 0;
	const term* t1 = i1 ? &(r2=*(i1=EVALPS(t.o, p->s))) : 0;
	TRACE(	dout<<"called with term " << format(id); 
		if (t0) dout << " subject = " << format(i0);
		if (t1) dout << " object = " << format(i1);
		dout << endl);

	//?

	if (t.p == GND) r = 1;

		//??

	else if (t.p == logequalTo)
		r = t0 && t1 && t0->p == t1->p ? 1 : 0;
	else if (t.p == lognotEqualTo)
		r = t0 && t1 && t0->p != t1->p ? 1 : 0;

		//internalized lists

	else if (t.p == rdffirst && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->s, p->s, t.o, p->s) ? 1 : -1;
		// returning -1 here because plz also look into the kb,
		// dont assume that if this builtin didnt succeed, the kb cant contain such fact
	else if (t.p == rdfrest && t0 && t0->p == Dot && (t0->s || t0->o))
		r = unify(t0->o, p->s, t.o, p->s) ? 1 : -1;

		//FFI

/*	else if (t.p == _dlopen) {
		if (get(t.o).p > 0) throw std::runtime_error("dlopen must be called with variable object.");
		std::vector<termid> params = get_list(t.s, *p);
		if (params.size() >= 2) {
			void* handle;
			try {
				string f = predstr(params[0]);
				if (f == "0") handle = dlopen(0, std::stol(predstr(params[1])));
				else handle = dlopen(ws(f).c_str(), std::stol(predstr(params[1])));
				pnode n = mkliteral(tostr((uint64_t)handle), XSD_INTEGER, 0);
				subs[p->s][get(t.o).p] = make(dict.set(n), 0, 0);
				r = 1;
			} catch (std::exception ex) { derr << indent() << ex.what() <<std::endl; }
			catch (...) { derr << indent() << "Unknown exception during dlopen" << std::endl; }
		}
	}
	else if (t.p == _dlerror) {
		if (get(t.o).p > 0) throw std::runtime_error("dlerror must be called with variable object.");
		auto err = dlerror();
		pnode n = mkliteral(err ? pstr(err) : pstr("NUL"), 0, 0);
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
				catch (...) { derr << indent() << "Unknown exception during dlopen" << std::endl; }
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

	////RDFS
		//first some old stuff
	/*
	else if (t.p == rdftype || t.p == A) { // {?P @has rdfs:domain ?C. ?S ?P ?O} => {?S a ?C}.
		termset ts(2);
		termid p = tmpvar();
		termid o = tmpvar();
		ts[0] = make(rdfsdomain, p, t.o);
		ts[1] = make(p, t.s, o);
		//queue.push(make_shared<proof>(nullptr, kb.add(make(A, t.s, t.o), ts), 0, p, subs(), 0, true));
	}
	else if ((
		 t.p == A // parser kludge
		 || t.p == rdftype || t.p == rdfssubClassOf) && t.s && t.o) {
		//termset ts(2,0,*alloc);
		termset ts(2);
		termid va = tmpvar();
		ts[0] = make ( rdfssubClassOf, va, t.o );
		ts[1] = make ( A, t.s, va );
		queuepush(make_shared<proof>(nullptr, kb.add(make ( A, t.s, t.o ), ts), 0, p, subs()));
	}*/
		// check the outputs of your parsers when working on this
		// check the definitions of the uri constants, some are right, some are just "xxx:yyy"
		// see we rely on the == A kludge...

	//else if ((r = rdfs_builtin(t, t0, t1)) != -1)
	//{}

	#ifdef with_marpa
		/*
	else if (t.p == marpa_parser_iri && t.s && t.o)
	// ?X is a parser created from grammar
	{
		if (t0->p > 0) throw std::runtime_error("must be called with variable subject.");
		void* handle = marpa_parser(this, t1->p, p);
		pnode n = mkliteral(tostr((uint64_t)handle), XSD_INTEGER, 0);
		p->s[t0->p] = make(dict.set(n), 0, 0);
		r = 1;
	}
	else if (t.p == file_contents_iri) {
		if (t0->p > 0) throw std::runtime_error("file_contents must be called with variable subject.");
		string fn = *dict[t0->p].value;
		std::string fnn = ws(fn);
	    std::ifstream f(fnn);
	    if (f.is_open())
		{
			p->s[t0->p] = make(mkliteral(pstr(load_file(f)), 0, 0));
			r = 1;
		}
	}
	else if (t.p == marpa_parse_iri) {
	// ?X is a parse of (input with parser)
		if (t0->p > 0) throw std::runtime_error("marpa_parse must be called with variable subject.");
		termid xxx = t1->s;
		termid xxx2 = t1->o;
		string input = *dict[xxx->p].value;
		string marpa = *dict[xxx2->p].value;
		termid result = marpa_parse((void*)std::stol(marpa), input);
		p->s[t0->p] = result;
		r = 1;
	}*/
	#endif
	if (r == 1) {
		shared_ptr<proof> r = make_shared<proof>(p, *p);
		r->btterm = EVALPS(id, p->s);
		++r->term_idx;
		queuepush(r);
	}
	return r;
}

void prover::pushev(shared_ptr<proof> p) {
	termid t;
	for (auto r : bodies[p->rule]) {
		if (!(t = (EVALS(r, p->s)))) continue;

		//dout << "XXX"; prints(p->s);
		for (auto x: p->s)
			subs_workardound[x.first].push_back(x.second);

		e[t->p].emplace_back(t, p->g(this));
		//if (level > 10) dout << "proved: " << format(t) << endl;
	}
}

shared_ptr<prover::proof> prover::step(shared_ptr<proof> _p) {
	if (!_p) return 0;
	setproc("step");
	if ((steps != 0) && (steps % 1000000 == 0)) (dout << "step: " << steps << endl);
	++steps;
	if (euler_path(_p)) return _p->next;
	const proof& frame = *_p;
	TRACE(dout<<"popped frame: " << formatp(_p) << endl);
	auto body = bodies[frame.rule];
	// if we still have some terms in rule body to process
	if (frame.term_idx != body.size()) {
		termid t = body[frame.term_idx];
		if (builtin(t, _p) != -1) return frame.next;
		{
			if ((rit = kb.r2id.find(t->p)) == kb.r2id.end()) return frame.next;
			step_in(kb.r2id[t->p], _p, t);
		}
	}
	else if (!frame.prev) gnd.push_back(_p);
	else {
		proof& ppr = *frame.prev;
		shared_ptr<proof> r = make_shared<proof>(_p, ppr);
		ruleid rl = frame.rule;
		r->s = ppr.s;
		unify(heads[rl], frame.s, bodies[r->rule][r->term_idx], r->s);
		++r->term_idx;
		step(r);
	}
	return frame.next;
}

void prover::step_in(ruleset::rulelist &candidates, shared_ptr<proof> _p, termid t)
{
	proof& frame = *_p;
	if (!frame.s.empty()) {
		for (auto rule : candidates) {
			if (unify(t, frame.s, heads[rule], termsub))
				queuepush(make_shared<proof>(_p, rule, 0, _p, termsub));
			termsub.clear();

		}
	}
	else for (auto rule : candidates) {
		if (unify(t, heads[rule], termsub))
			queuepush(make_shared<proof>(_p, rule, 0, _p, termsub));
			termsub.clear();

		}
}

prover::ground prover::proof::g(prover* p) const {
	if (!creator) return ground();
	ground r = creator->g(p);
	if (btterm)
		r.emplace_back(p->kb.add(btterm, termset()), subs());
	else if (creator->term_idx != p->bodies[creator->rule].size()) {
		if (p->bodies[rule].empty())
			r.emplace_back(rule, subs());
	} else if (!p->bodies[creator->rule].empty())
		r.emplace_back(creator->rule, creator->s);
	return r;	
}

termid prover::list2term_simple(std::list<termid>& l) {
	setproc("list2term_simple");
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
	setproc("list2term");
	termid t;
	if (l.empty()) t = make(Dot, 0, 0);
	else {
		pnode x = l.front();
		l.pop_front();

		//Try to find the item's value in quads.second. If it's
		//not there, then the item is not a list, and we'll make
		//a normal (0,node,0) term from the node. This will be the
		//subject of a term with Dot as the predicate and the term
		//for the next node as the object (or make(Dot,0,0) if there
		//is no next node). If the item's value was in quads.second,
		//then it's referencing a list, and we need to build up that
		//list and the term for this list will be the subject and the
		//same process for the object.
		auto it = quads.second.find(*x->value);
		//item is not a list
		if (it == quads.second.end())
			t = make(Dot, make(dict.set(x), 0, 0), list2term(l, quads));
		//item is a list
		else {
			auto ll = it->second;
			t = make(Dot, list2term(ll, quads), list2term(l, quads));
		}
		//Could do it like:
		//termid first;
		//termid rest = list2term(l,quads);
		//auto it = quads.second.find(*x->value);
		//if(it == quads.second.end()){
		//	first = make(dict.set(x),0,0);
		//}else{
		//	auto ll = it->second;
		//	first = list2term(ll,quads);
		//}
		//t = make(Dot,first,rest);
	}
	TRACE(dout << format(t) << endl);
	return t;
}

//Make terms from the subject node S, and object node O, which will be either
//lists or triples of the form: (0,S,0) and (0,O,0), respectively, and then make 
//a term for the predicate node P of the form ((0,S,0),P,(0,O,0)). make() will run
//checks on the (p,s,o) triples sent to it and then run prover::termdb::add()
//on these triples to actually create term objects and add them to the termdb.
termid prover::quad2term(const quad& p, const qdb& quads) {
	setproc("quad2term");
	TRACE(dout<<"called with: "<<p.tostring()<<endl);

	termid t, s, o;

	//Make sure the predicate is not rdffirst or rdfrest. Why?
	#ifndef with_marpa
	if (dict[p.pred] == rdffirst || dict[p.pred] == rdfrest) return 0;
	#endif

	//Make the (0,S,0) or list for the subject.
	auto it = quads.second.find(*p.subj->value);
	if (it != quads.second.end()) {
		auto l = it->second;
		s = list2term(l, quads);
	}
	else
		s = make(p.subj, 0, 0);

	//Make the (0,O,0) or list for the object.
	if ((it = quads.second.find(*p.object->value)) != quads.second.end()) {
		auto l = it->second;
		o = list2term(l, quads);
	}
	else
		o = make(p.object, 0, 0);

	//Make the ((0,S,0),P,(0,O,0)) term representing the full triple.
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


//This is just preprocessing to translate a qdb to a termset.
void prover::addrules(pquad q, qdb& quads) {
	setproc("addrules");
	TRACE(dout<<q->tostring()<<endl);

	//Get the values of the subject, predicate and object for this quad.
	const string &s = *q->subj->value, &p = *q->pred->value, &o = *q->object->value;
	termid t;
	TRACE(dout<<"called with " << q->tostring()<<endl);

	//if this is a rule, i.e. the predicate is "=>":
	if (p == implication) {

		//why would it be missing?
		//if the head graph is missing in the kb, add an empty one?
		if (quads.first.find(o) == quads.first.end()) 
			quads.first[o] = mk_qlist();

		//If there's a body present, then make a rule with this body
		//for each quad in the head.
		if(quads.first.find ( s ) != quads.first.end()){ 
	
			//build up the body into a termset.
			//termset ts represent the body, i.e. the subject of the
			//implication.
			termset ts = termset();

			//For each quad in the subject, make sure it's not
			//rdffirst or rdfrest, and if so then attempt to generate
			//a term from it using quad2term(). If this succeeds then
			//add this to our termset.
			for ( pquad z : *quads.first.at( s ) )
				if ((dict[z->pred] != rdffirst && 
					dict[z->pred] != rdfrest) &&
					(t = quad2term(*z, quads)))
					ts.push_back( t );
		
			//Explode the head into separate quads and make a separate
			//rule for each of them with the same body 'ts', using kb.add().
			for ( pquad y : *quads.first.at ( o ) ) {
				//Make a term from our current quad in the head, and add
				//this this term to heads and our termset ts to bodies.
				//ruleset::add()
				if ((t = quad2term(*y, quads))) 
					kb.add(t, ts);
			}
		}

	} 
	//adding a triple like <bnode> ..#implies <bnode> 
	else //for every rule added, although correct, 
	//creates a lot of noise, so we dont until we will have to
	if ((t = quad2term(*q, quads)))  //whats this test about?
		kb.add(t, termset()); // remarking the 'else' is essential for consistency checker
}



//Input: an already-filled-out qdb representing our KB, and a flag specifying
//whether we should check the KB for consistency or not.

//Check to see if we have "@default" and "false" contexts, and if not then
//make them. For every quad in the default context, <<addrules(quad,qkb)>>
prover::prover ( qdb qkb, bool check_consistency ) : kb(this) {
	//If we can't find the default graph, then make an empty default graph.
	auto it = qkb.first.find(str_default);
	if (it == qkb.first.end()){
		 //throw std::runtime_error("Error: @default graph is empty.");
		qkb.first["@default"] = mk_qlist();
		it = qkb.first.find(str_default);
	}
	//Why do we make empty "false" context in every kb?
	if (qkb.first.find("false") == qkb.first.end()) qkb.first["false"] = mk_qlist();

	//This is just preprocessing to translate a qdb to a termset.	
	for ( pquad quad : *it->second ) addrules(quad, qkb);

	//If the consistency flag was set true, then check the KB for consistency,
	//and if it's inconsistent, then throw an error.
	if (check_consistency && !consistency(qkb)) throw std::runtime_error("Error: inconsistent kb");
}




bool prover::consistency(const qdb& quads) {
	setproc("consistency");
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
		string s = *dict[y.first->s->p].value;
		if (s == "GND") continue;
		TRACE(dout<<"Trying to prove false context: " << s << endl);
		qdb qq;
		qq.first[""] = quads.first.at(s);
		q.do_query(qq);
		if (q.e.size()) {
			derr << "Inconsistency found: " << q.format(y.first) << " is provable as true and false."<<endl;
			c = false;
		}
	}
	return c;
}

prover::termset prover::qdb2termset(const qdb &q_) {
	termset goal = termset();
	termid t;
	for (auto q : merge(q_))
		if (dict[q->pred] != rdffirst && //wat
			dict[q->pred] != rdfrest &&
			(t = quad2term(*q, q_)))
			goal.push_back(t);
	return goal;
}


void prover::query(const qdb& q_, subs * s) {
	const termset t = qdb2termset(q_);
	query(t, s);
}

void prover::do_query(const qdb& q_, subs * s) {
	termset t = qdb2termset(q_);
	do_query(t, s);
}

void prover::query(const termset& goal, subs * s) {
	TRACE(dout << KRED << "Rules:\n" << formatkb() << endl << KGRN << "Query: " << format(goal) << KNRM << std::endl);
	auto duration = do_query(goal, s);
	TRACE(dout << KYEL << "Evidence:" << endl);
	printe();/* << ejson()->toString()*/ dout << KNRM;
	TRACE(dout << "elapsed: " << duration << "ms steps: " << steps << " unifs: " << unifs << " evals: " << evals << endl);
}

void prover::unittest() {
	pnode x = mkiri(pstr("x"));
	pnode a = mkiri(pstr("?a"));
	qdb &kb = *new qdb, &q = *new qdb;
	kb.first[str_default] = mk_qlist();
	q.first[str_default] = mk_qlist();
	kb.first[str_default]->push_back(make_shared<quad>(x, x, x));
	q.first[str_default]->push_back(make_shared<quad>(a, x, x));
	prover &p = *new prover(kb, false);
//	subs s1, s2;
//	termid xx = p.make(x, p.make(x,0,0), p.make(x,0,0));
//	termid aa = p.make(a, p.make(x,0,0), p.make(x,0,0));
//	for (uint n = 0; n < 2; ++n) {
//		p.unify(xx , s1, aa, s2);
//		dout <<"s1: "<< s1.format() << "s2: "<< s2.format() << endl;
//	}
//	exit(0);
	p.query(q);
	delete &kb;
	delete &q;
	delete &p;
}

int prover::do_query(const termid goal, subs * s)
{
	termset query;
	query.emplace_back(goal);
	return do_query(query, s);
}

int prover::do_query(const termset& goal, subs * s) {
	setproc("do_query");
	shared_ptr<proof> p = make_shared<proof>(nullptr, kb.add(0, goal)), q;
	if (s) p->s = *s;

	TRACE(dout << KGRN << "Query: " << format(goal) << KNRM << std::endl);
	{
		setproc("rules");
		TRACE(dout << KRED << "Rules:\n" << formatkb() << endl << KGRN << "Query: " << format(goal) << KNRM << std::endl);
	}

#ifdef TIMER
	using namespace std;
	using namespace std::chrono;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
#endif
	lastp = p;
	while ((p = step(p)));
#ifdef TIMER
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>( t2 - t1 ).count();
#else
	int duration = 0;
#endif

	for (auto x : gnd) pushev(x);
	TRACE(dout << KMAG << "Evidence:" << endl;printe();/* << ejson()->toString()*/ dout << KNRM);
	return duration/1000.;
}

term::term(nodeid _p, termid _s, termid _o) : p(_p), s(_s), o(_o) {}






//Think of these two make functions as part of the same thing, the
//first is just the entry point if you send a pnode for the pred,
//the second for if you send a nodeid for pred. The first one just
//gets the nodeid for the node and calls the second one.
termid prover::make(pnode p, termid s, termid o) { 
	return make(dict.set(*p), s, o); 
}


//Make a term from the (p,s,o) and add it to prover's termdb _terms,
//returning the termid for this term.
termid prover::make(nodeid p, termid s, termid o) {
//Make sure we have a node.
//redundant?
#ifdef DEBUG
	if (!p) throw 0;
#endif
//	if ( (_terms.terms.capacity() - _terms.terms.size() ) < _terms.terms.size() )
//		_terms.terms.reserve(2 * _terms.terms.size());

//They're either both 0 or both not zero. !s != !o rather than s != o
//so that they will evaluate to booleans first and then do boolean
//comparison rather than termid comparison.
	if (!s != !o) throw 0;

//Add the term to prover's termdb _terms.
	return _terms.add(p, s, o);
}






prover::ruleid prover::ruleset::add(termid t, const termset& ts) {
	setproc("ruleset::add");
	ruleid r =  _head.size();
	_head.push_back(t);
	_body.push_back(ts);
	r2id[t ? t->p : 0].push_back(r);
	return r;
}

prover::ruleid prover::ruleset::add(termid t) {
	termset ts = termset();
	return add(t, ts);
}

void prover::clear() {
	subs_workardound.clear();
	gnd.clear();
	e.clear();
}

bool prover::ask(nodeid s, nodeid p, nodeid o) {
	termid sn = make(s, 0, 0);
	termid on = make(o, 0, 0);
	return ask(sn, p, on);
}

bool prover::ask(termid s, nodeid p, termid o) {
	assert(s);assert(p);assert(o);
	setproc("ask");

	clear();

	termid question = make(p, s, o);
	termset query;
	query.emplace_back(question);

	subs dummy;
	do_query(query, &dummy);

	bool r = e.size();
	return r;
}

/*query, return termids*/
prover::termids prover::askt(termid s, nodeid p, termid o, size_t stop_at) {
	prover::termids r;
	assert(s);assert(p);assert(o);
	setproc("ask");

	clear();

	std::vector<termid> vars;
	if (s->p < 0) vars.push_back(s);
	if (o->p < 0) vars.push_back(o);
	//ISVAR
	assert(vars.size() < 2);//i guess you could want to run a query with both s and o variables, and get the results in one array, but hmm

	termid question = make(p, s, o);
	termset query;
	query.emplace_back(question);

	subs dummy;
	do_query(query, &dummy);
/*
	for (auto ei  : e)
	{
		for (auto x: ei.second)
		{
			for (auto g: x.second)
			{

				subs env = g.second;
*/


				for (auto var:vars) {
					//dout << var << " " << var->p << " " << subs_workardound.size() ;
					if (subs_workardound.find(var->p) != subs_workardound.end()) {
						for (auto v:subs_workardound[var->p])
						{
							TRACE(dout << "match: " << v << std::endl;)
							r.push_back(v);
							if (stop_at && (r.size() >= stop_at)) {
								goto end;
							}
						}
					}
				}
	end:
		return r;
}

prover::nodeids prover::askn(termid s, nodeid p, termid o, size_t stop_at) {
	auto r = askt(s, p, o, stop_at);
	prover::nodeids rr;
	for (auto rrr:r)
		rr.push_back(rrr->p);
	return rr;
}

/*query for subjects*/
prover::nodeids prover::askns(nodeid p, nodeid o, size_t stop_at) {
    assert(p && o);
    auto ot = make(o);
    assert (ot);
    termid s_var = tmpvar();
	assert(s_var);
	return askn(s_var, p, ot, stop_at);
}

/*query for objects*/
prover::nodeids prover::askno(nodeid s, nodeid p, size_t stop_at) {
    assert(s && p);
    auto st = make(s);
    assert (st);
    termid o_var = tmpvar();
	assert(o_var);
	return askn(st, p, o_var, stop_at);
}

/*query for one object*/
nodeid prover::askn1o(nodeid s, nodeid p) {
    return force_one_n(askno(s, p, 1));
}

/*query for one subject*/
nodeid prover::askn1s(nodeid p, nodeid o) {
	return force_one_n(askns(p, o, 1));
}

/*query for one object term*/
termid prover::askt1o(nodeid s, nodeid p) {
    assert(s);
    assert(p);
    termid xxs = make(s);
	termid o_var = tmpvar();
    assert (xxs);
    assert(o_var);
    return force_one_t(askt(xxs, p, o_var, 1));
}

nodeid prover::force_one_n(nodeids r) {
    /*#ifdef debug
     * if (r.size() > 1)
    {
        std::stringstream ss;
        ss << "well, this is weird, more than one match:";
        for (auto xx: r)
            ss << xx << " ";
        throw runtime_error(ss.str());
    }
     #endif*/
    if (r.size() == 0)
        return 0;
    else
		return r[0];
}

termid prover::force_one_t(termids r) {
    if (r.size() == 0)
        return 0;
    else
		return r[0];
}

/*get_list wrapper useful in marpa*/
prover::nodeids prover::get_list(nodeid head)
{
	proof hack(nullptr, 0);
    auto r = get_list(make(head), &hack);
    nodeids rr;
    for (auto rrr: r)
        rr.push_back(rrr->p);
    return rr;
}
