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
bidict& gdict = dict;
#include "prover.h"
extern std::ostream& dout;
using namespace std;

namespace prover {
const uint MEM = 1024 * 1024;
const uint max_terms = MEM;		uint nterms = 0; 		term* terms = 0;
const uint max_termsets = MEM;		uint ntermsets = 0; 		termset* termsets = 0;
const uint max_rules = MEM;		uint nrules = 0; 		rule* rules = 0;
const uint max_proofs = MEM; 		uint nproofs = 0; 		proof* proofs = 0;

void printps(term* p, subst* s); // print a term with a subst

term* GND;

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(X)
#endif

// set alloc to false in order to only zero the memory
void initmem() {
	if (terms) delete[] terms;
	if (rules) delete[] rules;
	if (proofs) delete[] proofs;

	terms	 	= new term[max_terms];
	termsets 	= new termset[max_termsets];
	rules 		= new rule[max_rules];
	proofs 		= new proof[max_proofs];
	nterms = ntermsets = nrules = nproofs = 0;
}

term* evaluate(term* p, subst& s) {
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
	if (s->p < 0) {
		term* sval = evaluate(s, ssub);
		if (sval)
			return unify(sval, ssub, d, dsub, f);
		return true;
	}
	if (d->p < 0) {
		term* dval = evaluate(d, dsub);
		if (dval)
			return unify(s, ssub, dval, dsub, f);
		else {
			if (f)
				dsub[d->p] = evaluate(s, ssub);
			return true;
		}
	}
	if (!(s->p == d->p && !s->s == !d->s && !s->o == !d->o))
		return false;
	bool r = true;
	if (s->s)
		r &= unify(s->s, ssub, d->s, dsub, f);
	if (s->o)
		r &= unify(s->o, ssub, d->o, dsub, f);
	return r;

}

bool euler_path(proof* p) {
	proof* ep = p;
	while ((ep = ep->prev))
		if (ep->rul == p->rul &&
			unify(ep->rul->p, ep->s, p->rul->p, p->s, false))
			break;
	if (ep) {
		TRACE(dout<<"Euler path detected\n");
		return true;
	}
	return false;
}

int builtin(term*, proof*, session*) {
//	const char* s = dgetw(ss->d, t->p);
//	if (s && !strcmp(s, "GND"))	
//		return 1;
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
	TRACE(dout<<"\nprove() called with facts:");
	TRACE(printrs(cases));
	TRACE(dout<<"\nand query:");
	TRACE(printl(goal));
	TRACE(dout<<endl);
	do {
		p = qu.front();
		qu.pop_front();
		if (p->last != p->rul->body.end()) {
			term* t = *p->last;
			TRACE(dout<<"Tracking back from ");
			TRACE(printterm(*t));
			TRACE(dout<<endl);
			int b = builtin(t, p, ss);
			if (b == 1) {
				proof* r = &proofs[nproofs++];
				rule* rl = &rules[nrules++];
				*r = *p;
				rl->p = evaluate(t, p->s);
				r->g = ground(p->g);
				r->g.emplace_back(rl, subst());
				++r->last;
				qu.push_back(r);
		            	continue;
		        }
		    	else if (!b) 
				continue;

			auto it = cases.find(t->p);
			if (it == cases.end())
				continue;
			list<rule*> rs = it->second;
			for (rule* rl : rs) {
				subst s;
				TRACE(dout<<"\tTrying to unify ");
				TRACE(printps(*t, p->s));
				TRACE(dout<<" and ");
				TRACE(printps(*rl->p, s));
				TRACE(dout<<"... ");
				if (unify(t, p->s, rl->p, s, true)) {
					TRACE(dout<<"\tunification succeeded");
					TRACE(if (s.size()) {
						TRACE(dout<<" with new substitution: ");
						TRACE(prints(s));
					});
					TRACE(dout<<endl);
					if (euler_path(p))
						continue;
					proof* r = &proofs[nproofs++];
					r->rul = rl;
					r->last = r->rul->body.begin();
					r->prev = p;
					r->s = s;
					r->g = ground(p->g);
					if (rl->body.empty())
						r->g.emplace_back(rl, subst());
					qu.push_back(r);
//					TRACE(dout<<"queue:\n"));
//					TRACE(printq(qu));
				} else {
					TRACE(dout<<"\tunification failed\n");
				}
			}
		}
		else if (!p->prev) {
			for (auto r = p->rul->body.begin(); r != p->rul->body.end(); ++r) {
				term* t = evaluate(*r, p->s);
				ss->e[t->p].emplace_back(t, p->g);
			}
			TRACE(dout<<"no prev frame. queue:\n");
//			TRACE(printq(qu));
		} else {
			ground g = p->g;
			proof* r = &proofs[nproofs++];
			if (!p->rul->body.empty())
				g.emplace_back(p->rul, p->s);
			*r = *p->prev;
			r->g = g;
			r->s = p->prev->s;
			unify(p->rul->p, p->s, *r->last, r->s, true);
			++r->last;
			qu.push_front(r);
			TRACE(dout<<"finished a frame. queue:\n");
//			TRACE(printq(qu));
			continue;
		}
	} while (!qu.empty());
	dout<<"\nEvidence:";
	dout<<"========="<<endl;
	printe(ss->e);
}

bool equals(term* x, term* y) {
	return (!x == !y) && x && equals(x->s, y->s) && equals(x->o, y->o);
}

void printterm(const term& p) {
	if (p.s)
		printterm(*p.s);
	dout<<' '<<dstr(p.p)<<' ';
	if (p.o)
		printterm(*p.o);
}

void printp(proof* p) {
	if (!p)
		return;
	dout<<"\trule:\t";
	printr(*p->rul);
	dout<<"\n\tremaining:\t";
//	printl(p->last);
	if (p->prev)
		dout<<"\n\tprev:\t"<<(p->prev - proofs)/sizeof(proof)<<"\n\tsubst:\t";
	else
		dout<<"\n\tprev:\t(null)\n\tsubst:\t";
	prints(p->s);
	dout<<"\n\tground:\t";
	printg(p->g);
	dout<<"\n";
}

void printrs(const ruleset& rs) {
	for (auto x : rs)
		for (auto y : x.second) {
			printr(*y);
			dout << endl;
		}
}

void printq(const queue& q) {
	for (auto x : q) {
		printp(x);
		dout << endl;
	}
}

void prints(const subst& s) {
	dout << '[';
	for (auto x : s) {
		dout<<"["<<dstr(x.first)<<" / ";
		printterm(*x.second);
		dout << ";";
	}
	dout << ']' << endl;
}

void printl(const termset& l) {
	for (auto x : l) {
		printterm(*x);
		dout << ',';
	}
}

void printr(const rule& r) {
	printl(r.body);
	dout<<" => ";
	printterm(*r.p);
}

void printg(const ground& g) {
	for (auto x : g) {
		printr(*x.first);
		dout<<':';
		prints(x.second);
	}
	dout<<'.';
}

void printe(const evidence& e) {
	for (auto y : e)
		for (auto x : y.second) {
			printterm(*x.first);
			dout << ": ";
			printg(x.second);
			dout << endl;
		}
}
}
