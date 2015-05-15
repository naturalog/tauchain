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
#include <boost/algorithm/string.hpp>

using namespace boost::algorithm;
bidict& gdict = dict;

int logequalTo, lognotEqualTo, rdffirst, rdfrest, A, rdfsResource, rdfList, Dot, GND, rdfsType, rdfssubClassOf;

namespace prover {

term* terms = 0;
termset* termsets = 0;
rule* rules = 0;
proof* proofs = 0;
uint nterms, ntermsets, nrules, nproofs;

void printps(term* p, subst* s); // print a term with a subst

void initmem() {
	static bool once = false;
	if (!once) {
		terms = new term[max_terms];
		termsets = new termset[max_termsets];
		rules = new rule[max_rules];
		proofs = new proof[max_proofs];
		once = true;
	}
//	memset(terms, 0, sizeof(term) * max_terms);
//	memset(termsets, 0, sizeof(termset) * max_termsets);
//	memset(rules, 0, sizeof(rule) * max_rules);
//	memset(proofs, 0, sizeof(proof) * max_proofs);
	nterms = ntermsets = nrules = nproofs = 0;
}

term* evaluate(term* p, subst& s) {
	if (!p) return 0;
	if (p->p < 0) {
		auto it = s.find(p->p);
		return it == s.end() ? 0 : evaluate(s[p->p], s);
	}
	if (!p->s && !p->o)
		return p;
	term *a = evaluate(p->s, s), *r = &terms[nterms++];
	r->p = p->p;
	if (a)
		r->s = a;
	else {
		r->s = &terms[nterms++];
		r->s->p = p->s->p;
		r->s->s = r->s->o = 0;
	}
	a = evaluate(p->o, s);
	if (a)
		r->o = a;
	else {
		r->o = &terms[nterms++];
		r->o->p = p->o->p;
		r->o->s = r->o->o = 0;
	}
	return r;
}

void printps(term& p, subst& s) {
	printterm(p);
	dout<<" [ ";
	prints(s);
	dout<<" ] ";
}

bool unify(term* s, subst& ssub, term* d, subst& dsub, bool f) {
	term* v;
	if (!s && !d) 
		return true;
	if (s->p < 0) 
		return (v = evaluate(s, ssub)) ? unify(v, ssub, d, dsub, f) : true;
	if (d->p < 0) {
		if ((v = evaluate(d, dsub)))
			return unify(s, ssub, v, dsub, f);
		else {
			if (f)
				dsub[d->p] = evaluate(s, ssub);
			return true;
		}
	}
	if (!(s->p == d->p && !s->s == !d->s && !s->o == !d->o))
		return false;
	return unify(s->s, ssub, d->s, dsub, f) && unify(s->o, ssub, d->o, dsub, f);
}

bool euler_path(proof* p) {
	proof* ep = p;
	while ((ep = ep->prev))
		if (ep->rul == p->rul &&
			unify(ep->rul->p, *ep->s, p->rul->p, *p->s, false))
			break;
	if (ep) {
		TRACE(dout<<"Euler path detected\n");
		return true;
	}
	return false;
}

int builtin(term& t, proof& p, session* ss) {
	if (t.p == GND) return 1;
	term* t0 = evaluate(t.s, *p.s);
	term* t1 = evaluate(t.o, *p.s);
	if (t.p == logequalTo)
		return t0 && t1 && t0->p == t1->p ? 1 : 0;
	if (t.p == lognotEqualTo)
		return t0 && t1 && t0->p != t1->p ? 1 : 0;
	if (t.p == rdffirst && t0 && t0->p == Dot && (t0->s || t0->o))
		return unify(t0->s, *p.s, t.o, *p.s, true) ? 1 : 0;
	if (t.p == rdfrest && t0 && t0->p == Dot && (t0->s || t0->o))
		return unify(t0->o, *p.s, t.o, *p.s, true) ? 1 : 0;
	if (t.p == A || t.p == rdfsType) {
//		if (t1 && ((t1->p == rdfList && t0 && t0->p == Dot) || t1->p == rdfsResource))
//			return 1;
//		if (!t.o) 
//			return -1;
		if (!t0) t0 = t.s;
		if (!t1) t1 = t.o;
		// {?A rdfs:subClassOf t1. ?t0 a ?A} => {t0 a t1}.
		// {?A rdfs:subClassOf ?B. ?S a ?A} => {?S a ?B}.
		session s2;
		s2.rkb = ss->rkb;
		term vA(L"?A");
		term q1(rdfssubClassOf, vA, *t1);
		term q2(A, *t0, vA);
		s2.goal.insert(&q1);
		s2.goal.insert(&q2);
		prove(&s2);
		return s2.e.empty() ? -1 : 1;
/*		term q1(rdfssubClassOf, vA, *t1);
		s2.goal.insert(&q1);
		prove(&s2);
		auto it = s2.e.find(rdfssubClassOf);
		if (it == s2.e.end()) return -1;
//		std::list<std::pair<term*, ground>> answers = it->second;
		for (auto tg : it->second) {
			int subclasser = tg.first->s->p;
			TRACE(dout << "subclasser: " << dict[subclasser] << std::endl);
			session s3;
			s3.rkb = ss->rkb;
			term sc(subclasser);
			term q2(A, *t0, sc);
			s3.goal.insert(&q2);
			prove(&s3);
			if (!s3.e.empty())
				return 1;
		}*/
	}
	#ifdef marpa
	if (t.p == marpa_parsed) {
	#endif

	return -1;
}

void prove(session* ss) {
	termset& goal = ss->goal;
	ruleset& cases = ss->rkb;
	queue qu;
	rule* rg = &rules[nrules++];
	proof* p = &proofs[nproofs++];
	rg->p = 0;
	rg->body = goal;
	p->rul = rg;
	p->last = rg->body.begin();
	p->prev = 0;
	qu.push_back(p);
	TRACE(dout<<"\nprove() called with facts:\n";
		printrs(cases);
//		dout<<"\nand query:\n";
//		printl(goal);
		dout << std::endl);
	do {
		p = qu.back();
		qu.pop_back();
//		TRACE(dout<<"popped frame:\n";printp(p));
		if (p->last != p->rul->body.end()) {
			term* t = *p->last;
			TRACE(dout<<"Tracking back from ";
				printterm(*t);
				dout << std::endl);
			int b = builtin(*t, *p, ss);
			if (b == 1) {
				proof* r = &proofs[nproofs++];
				rule* rl = &rules[nrules++];
				*r = *p;
				rl->p = evaluate(t, *p->s);
				r->g = p->g;
				r->g.emplace_back(rl, make_shared<subst>());
				++r->last;
				qu.push_back(r);
//				TRACE(dout<<"pushing frame:\n";printp(r));
		            	continue;
		        }
		    	else if (!b) 
				continue;

			auto it = cases.find(t->p);
			if (it == cases.end())
				continue;
			for (auto x : cases) {
//			if (x.first >= 0 && x.first != t->p)
//				continue;
			std::list<rule*>& rs = x.second;
//			list<rule*>& rs = it->second;
			for (rule* rl : rs) {
				subst s;
				TRACE(dout<<"\tTrying to unify ";
					printps(*t, *p->s);
					dout<<" and ";
					printps(*rl->p, s);
					dout<<"... ");
				if (unify(t, *p->s, rl->p, s, true)) {
					TRACE(dout<<"\tunification succeeded";
						if (s.size()) {
							dout<<" with new substitution: ";
							prints(s);
						}
						dout << std::endl);
					if (euler_path(p))
						continue;
					proof* r = &proofs[nproofs++];
					r->rul = rl;
					r->last = r->rul->body.begin();
					r->prev = p;
					r->s = make_shared<subst>(s);
					r->g = p->g;
					if (rl->body.empty())
						r->g.emplace_back(rl, make_shared<subst>());
					qu.push_front(r);
//					TRACE(dout<<"pushing frame:\n";printp(r));
				} else {
					TRACE(dout<<"\tunification failed\n");
				}
			}
		}}
		else if (!p->prev) {
			for (auto r = p->rul->body.begin(); r != p->rul->body.end(); ++r) {
				term* t = evaluate(*r, *p->s);
				ss->e[t->p].emplace_back(t, p->g);
			}
			TRACE(dout<<"no prev frame. queue:\n");
			TRACE(printq(qu));
		} else {
			ground g = p->g;
			proof* r = &proofs[nproofs++];
			if (!p->rul->body.empty())
				g.emplace_back(p->rul, p->s);
			*r = *p->prev;
			r->g = g;
			r->s = make_shared<subst>(*p->prev->s);
			unify(p->rul->p, *p->s, *r->last, *r->s, true);
			++r->last;
			qu.push_back(r);
//			TRACE(dout<<"pushing frame:\n";printp(r));
			TRACE(dout<<L"finished a frame. queue size:" << qu.size() << std::endl);
//			TRACE(printq(qu));
			continue;
		}
	} while (!qu.empty());
	dout << "==========================" << std::endl;
	dout << KRED;
	dout<<"Facts:\n";
	printrs(cases);
	dout << KYEL;
	dout << "Query:\n";
	printl(goal);
	dout << KNRM;
	dout << "\nEvidence:\n";
	printe(ss->e);
//	dout << "==========================" << std::endl;
}

bool equals(term* x, term* y) {
	return (!x == !y) && x && equals(x->s, y->s) && equals(x->o, y->o);
}

string format(const term& p) {
	std::wstringstream ss;
	if (p.s)
		ss << format (*p.s);
	ss << L' ' << dstr(p.p) << L' ';
	if (p.o)
		ss << format (*p.o);
	return ss.str();
}

void printterm(const term& p) {
	if (p.s)
		printterm(*p.s);
	dout << L' ' << dstr(p.p) << L' ';
	if (p.o)
		printterm(*p.o);
}

void printp(proof* p) {
	if (!p)
		return;
	dout << L"\trule:\t";
	printr(*p->rul);
//	dout << L"\n\tindex:\t" << std::distance(p->rul->body.begin(), p->last) << std::end;
	if (p->prev)
		dout << L"\n\tprev:\t" << p->prev << L"\n\tsubst:\t";
	else
		dout << L"\n\tprev:\t(null)\n\tsubst:\t";
	if (p->s) prints(*p->s);
	dout << L"\n\tground:\t";
	printg(p->g);
	dout << L"\n";
}

void printrs(const ruleset& rs) {
	for (auto x : rs)
		for (auto y : x.second) {
			printr(*y);
			dout << std::endl;
		}
}

void printq(const queue& q) {
	for (auto x : q) {
		printp(x);
		dout << std::endl;
	}
}

void prints(const subst& s) {
//	dout << '[';
	for (auto x : s) {
		dout << L"        " << dstr(x.first) << L" / ";
		printterm(*x.second);
		dout << std::endl;
	}
//	dout << "] ";
}

string format(const termset& l) {
	std::wstringstream ss;
	auto x = l.begin();
	while (x != l.end()) {
		ss << format (**x);
		if (++x != l.end())
			ss << L',';
	}
	return ss.str();
}

void printl(const termset& l) {
	auto x = l.begin();
	while (x != l.end()) {
		printterm(**x);
		if (++x != l.end())
			dout << L',';
	}
}

void printr(const rule& r) {
	if(!r.body.empty())
	{
		printl(r.body);
		dout << L" => ";
	}
	if (r.p)
		printterm(*r.p);
	if(r.body.empty())
		dout << L".";
}

void printterm_substs(const term& p, const subst& s) {
	if (p.s)
		printterm_substs(*p.s, s);
	dout << L' ' << dstr(p.p);
	if(s.find(p.p) != s.end())
	{
		dout << L'(';
		printterm(*s.at(p.p));
		dout << L')';
	}
	dout << L' ';
	if (p.o)
		printterm_substs(*p.o, s);
}

void printl_substs(const termset& l, const subst& s) {
	auto x = l.begin();
	while (x != l.end()) {
		printterm_substs(**x, s);
		if (++x != l.end())
			dout << L',';
	}
}

void printr_substs(const rule& r, const subst& s) {
	printl_substs(r.body, s);
	dout << L" => ";
	printterm_substs(*r.p, s);
}

string format(const rule& r) {
	std::wstringstream ss;
	if (!r.body.empty())
		ss << format(r.body) << L" => ";
	ss << format(*r.p);
	return ss.str();
}

void printg(const ground& g) {
	for (auto x : g) {
		dout << L"    ";
		printr_substs(*x.first, *x.second);
		if (!x.second || x.second->empty()) {
			dout << std::endl;
			continue;
		}
	}
//	dout<<'.';
}

void printe(const evidence& e) {
	for (auto y : e)
		for (auto x : y.second) {
			printterm(*x.first);
//			if ( x.second.size() == 1 && x.second.front().second.empty()) 
//				dout << L"." << std::endl;
//			else
//			{
				dout << L":" << std::endl;
				printg(x.second);
				dout << std::endl;
//			}
#ifdef IRC
			sleep(1);
#endif
		}
}
}

prover::term* mkterm(const wchar_t* p, const wchar_t* s = 0, const wchar_t* o = 0) {
	prover::term* t = &prover::terms[prover::nterms++];
	t->p = dict.set(string(p));
	if (s) t->s = mkterm(s, 0, 0);
	if (o) t->o = mkterm(o, 0, 0);
	return t;
}

prover::term* mkterm(string s, string p, string o) {
	return mkterm(p.c_str(), s.c_str(), o.c_str());
}

prover::term* quad2term(const quad& p) {
	return mkterm(p.pred->value.c_str(), p.subj->value.c_str(), p.object->value.c_str());
}

int szqdb(const qdb& x) {
	int r = 0;
	for (auto y : x)
		r += y.second->size();
	return r;
}

void reasoner::addrules(string s, string p, string o, prover::session& ss, const qdb& kb) {
	if (p[0] == L'?')
		throw 0;
	prover::rule* r = &prover::rules[prover::nrules++];
	r->p = mkterm(s, p, o );
	if ( p != implication || kb.find ( o ) == kb.end() ) 
		ss.rkb[r->p->p].push_back(r);
	else for ( jsonld::pquad y : *kb.at ( o ) ) {
		r = &prover::rules[prover::nrules++];
		prover::term* tt = quad2term( *y );
		r->p = tt;
		if ( kb.find ( s ) != kb.end() )
			for ( jsonld::pquad z : *kb.at ( s ) )
				r->body.insert( quad2term( *z ) );
		ss.rkb[r->p->p].push_back(r);
	}
}

qlist merge ( const qdb& q ) {
	qlist r;
	for ( auto x : q ) for ( auto y : *x.second ) r.push_back ( y );
	return r;
}

bool reasoner::prove ( qdb kb, qlist query ) {
	prover::session ss;
	//std::set<string> predicates;
	for ( jsonld::pquad quad : *kb.at(L"@default")) {
		const string &s = quad->subj->value, &p = quad->pred->value, &o = quad->object->value;
			addrules(s, p, o, ss, kb);
	}
	for ( auto q : query )
		ss.goal.insert( quad2term( *q ) );
	prover::prove(&ss);
	return ss.e.size();
}

bool reasoner::test_reasoner() {
	dout << L"to perform the socrates test, put the following three lines in a file say f, and run \"tau < f\":" << std::endl
		<<L"socrates a man.  ?x a man _:b0.  ?x a mortal _:b1.  _:b0 => _:b1." << std::endl<<L"fin." << std::endl<<L"?p a mortal." << std::endl;
	return 1;
}

float degrees ( float f ) {
	static float pi = acos ( -1 );
	return f * 180 / pi;
}
