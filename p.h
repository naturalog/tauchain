#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <string>
#include <set>

typedef std::wstring string;
typedef int resid;
struct term;
#include "containers.h"

typedef struct frame* pframe;
const wchar_t endl[3] = L"\r\n";

#ifdef DEBUG
#define TRACE(x) x
#else
#define TRACE(x)
#endif

typedef map<resid, term*, true> subs;
typedef vector<term*, true> termset;
string format(const term* t, bool body = false);
string format(const termset& t, int dep = 0);
string format(const subs& s);
void trim(string& s);
string wstrim(string s);

term* mkterm(termset& kb, termset& query);
term* mkterm(resid p);
term* mkterm(resid p, const termset& args);
typedef term* (*fevaluate)(term&);
typedef term* (*fevaluates)(term&, const subs&);
typedef bool (*funify)(term& s, term& d, subs& dsub);
typedef bool (*funify_ep)(term& s, term& d, const subs& dsub);

term* evvar(term&);
term* evnoargs(term&);
term* ev(term&);
term* evs(term&,const subs&);
term* evvars(term&,const subs&);
term* evnoargss(term&,const subs&);
bool u1(term& s, term& d, subs& dsub);
bool u2(term& s, term& d, const subs& dsub);
bool u3(term& s, term& d, subs& dsub);
bool u4(term& s, term& d, const subs& dsub);

struct term {
	term();
	term(termset& kb, termset& query);
	term(resid _p, const termset& _args = termset());

	const resid p;
	termset args;
	const size_t szargs;
	termset matches, body;

	fevaluate evaluate;
	fevaluates evaluates;
	funify unify;
	funify_ep unify_ep;

	void trymatch(termset& heads);

	struct coro {
		term& t;
		term* i;
		termset::coro match_coro, args_coro;
		subs dsub;
		bool state;
		coro(term& _t) : t(_t), match_coro(t.matches), args_coro(t.args), state(false) {}
		bool operator()() {
			switch (state) {
			case false:
				while (match_coro()) {
					{
						termset::coro match_args_coro((*match_coro.i)->args);
						while (args_coro() && match_args_coro())
							if (!(*args_coro.i)->unify(**args_coro.i, **match_args_coro.i, dsub))
								break;
					}
					if (!args_coro.state) {
						i = *match_coro.i;
						return state = true;
					} else args_coro.state = false;
			case true:
					dsub.clear1();
				}
				return state = false;
			}
			return false;
		}
	};
};

struct tcmp {
	bool operator()(term* _x, term* _y) const {
		term &x = *_x, &y = *_y;
		if (x.szargs != y.szargs) return x.szargs < y.szargs;
		if (x.p != y.p) return x.p < y.p;
		for (termset::iterator i = x.args.begin(), e = x.args.end(), j = y.args.begin(); i != e; ++i, ++j) 
			if ((*i)->p != (*j)->p) return (*i)->p < (*j)->p;
		return false;
	}
};
std::set<term*, tcmp> terms;

const size_t tchunk = 8192, nch = tchunk / sizeof(term);

typedef vector<mapelem<term*, subs>, false > ground;
typedef map<resid, vector<mapelem<term*, ground>, false >, false > evidence;

struct frame {
	term* rule;
	termset::iterator b;
	pframe prev, creator, next;
	subs s;
	int ref;
	ground g() const; // calculate the ground
	frame(pframe c, frame& p, const subs& _s);
	frame(pframe c, term* r, termset::iterator l, pframe p);
	frame(pframe c, term* r, termset::iterator l, pframe p, const subs&  _s);
	void decref();
};
/*
const size_t szf = sizeof(frame), fchunk = 32, szc = szf * fchunk;
size_t fn = 0, fc = 0;
frame* fbuf = 0;//(frame*)malloc(szc);

frame* mkframe(pframe c, frame& p, const subs& _s) {
	if (!(fn%fchunk)) fbuf = (frame*)realloc(fbuf, szc * ++fc);
	new (&((frame*)fbuf)[fn])(frame)(c,p,_s);
	return &((frame*)fbuf)[fn++];
}

frame* mkframe(pframe c, term* r, termset::iterator* l, pframe p) {
	if (!(fn%fchunk)) fbuf = (frame*)realloc(fbuf, szc * ++fc);
	new (&((frame*)fbuf)[fn])(frame)(c,r,l,p);
	return &((frame*)fbuf)[fn++];
}

frame* mkframe(pframe c, term* r, termset::iterator* l, pframe p, const subs& _s) {
	if (!(fn%fchunk)) fbuf = (frame*)realloc(fbuf, szc * ++fc);
	new (&((frame*)fbuf)[fn])(frame)(c,r,l,p,_s);
	return &((frame*)fbuf)[fn++];
}
*/
void prove(pframe _p, pframe& lastp);
