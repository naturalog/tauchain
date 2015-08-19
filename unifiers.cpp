#include "prover.h"

bool prover::unify(termid _s, const subs& ssub, termid _d, subs& dsub) {
//	PROFILE(++unifs);
	if (!_s || !_d) return !_s == !_d;
	return _s->unify(ssub, _d, dsub);
/*	setproc(L"unify_bind");
	termid v;
	bool r, ns = false;
	const term& d = *_d, &s = *_s;
	TRACE(dout << "Trying to unify " << format(_s) << " sub: " << formats(ssub) << " with " << format(_d) << " sub: " << formats(dsub) << endl);
	if (ISVAR(s)) {
		v = evaluate(s, ssub);
		r = v ? unify(v, ssub, _d, dsub) : true;
	}
	else if (ISVAR(d)) {
		v = evaluate(d, dsub);
		if (v) r = unify(_s, ssub, v, dsub);
		else {
			dsub.emplace(d.p, evaluate(s, ssub));
			TRACE(dout<<"new sub:"<<formats(dsub)<<endl);
			r = ns = true;
		}
	}
	else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) r = false;
	else if (!s.s) r = true;
	else if ((r = unify(s.s, ssub, d.s, dsub))) 
		r = unify(s.o, ssub, d.o, dsub);
	TRACE(
		dout << "Trial to unify " << format(s) << " sub: " << formats(ssub) << " with " << format(d) << " sub: " << formats(dsub) << " : ";
		if (r) {
			dout << "passed";
			if (ns) dout << " with new substitution: " << dstr(d.p) << " / " << format(dsub[d.p]);
		} else dout << "failed";
		dout << endl);
	return r;
	*/
}

bool prover::unify_ep(termid _s, const subs& ssub, const term& d, const subs& dsub) {
	PROFILE(++unifs);
	if (!_s) return false;
	setproc(L"unify_ep");
	return _s->unify_ep(ssub, d, dsub);
/*	const term& s = *_s;
	termid v;
	bool r;
	if (ISVAR(s)) {
		v = evaluate(s, ssub);
		r = v ? unify_ep(v, ssub, d, dsub) : true;
	}
	else if (ISVAR(d)) {
		v = evaluate(d, dsub);
		r = v ? unify_ep(_s, ssub, *v, dsub) : true;
	}
	else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) r = false;
	else if (!s.s) r = true;
	else if ((r = unify_ep(s.s, ssub, *d.s, dsub)))
		r = unify_ep(s.o, ssub, *d.o, dsub);
	TRACE(dout << "Trying to unify " << format(s) << " sub: " << formats(ssub)
		  << " with " << format(d) << " sub: " << formats(dsub) << " : ";
		if (r) dout << "passed"; else dout << "failed";
		dout << endl);
	return r;
	*/
}

