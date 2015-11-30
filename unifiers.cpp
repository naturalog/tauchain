#include "prover.h"



bool prover::unify(termid _s, const subs& ssub, termid _d, subs& dsub) {
	PROFILE(++unifs);
	if (!_s || !_d) return !_s == !_d;
	setproc("unify_bind");
	termid v;
	bool r, ns = false;
	const term& d = *_d, &s = *_s;
	TRACE(dout << "Trying to unify " << format(_s) << " sub: " << formats(ssub) << " with " << format(_d) << " sub: " << formats(dsub) << endl);
	if (ISVAR(s)) {
		v = evalvar(s, ssub);
		r = v ? (ISVAR(d) ? unify_snovar_dvar(*v, ssub, d, dsub) : unify_sdnovar(*v, ssub, d, dsub)) : true;
	}
	else if (ISVAR(d)) {
		v = evalvar(d, dsub);
		if (v) r = unify_sdnovar(s, ssub, *v, dsub);
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
}

bool prover::unify_ep(termid _s, const subs& ssub, const term& d, const subs& dsub) {
	PROFILE(++unifs);
	if (!_s) return false;
	setproc("unify_ep");
	const term& s = *_s;
	termid v;
	bool r;
	if (ISVAR(s)) {
		v = evalvar(s, ssub);
		r = v ? (ISVAR(d) ? unify_snovar_dvar_ep(*v, ssub, d, dsub) : unify_sdnovar_ep(*v, ssub, d, dsub)) : true;
	}
	else if (ISVAR(d)) {
		v = evalvar(d, dsub);
		r = v ? unify_sdnovar_ep(s, ssub, *v, dsub) : true;
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
}

bool prover::unify_ep(termid _s, const term& d) {
	PROFILE(++unifs);
	if (!_s) return false;
	setproc("unify_ep");
	const term& s = *_s;
	bool r;
	if (ISVAR(s) || ISVAR(d)) return true;
	else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) r = false;
	else if (!s.s) r = true;
	else if ((r = unify_ep(s.s, *d.s)))
		r = unify_ep(s.o, *d.o);
	return r;
}

bool prover::unify_snovar_ep(const term& s, const subs& ssub, const term& d) {
	PROFILE(++unifs);
	if (ISVAR(d)) return true;
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	if (!s.s) return true;
	if (!unify_ep(s.s, ssub, *d.s)) return false;
	return unify_ep(s.o, ssub, *d.o);
}

bool prover::unify_ep(termid _s, const subs& ssub, const term& d) {
	PROFILE(++unifs);
	if (!_s) return false;
	setproc("unify_ep");
	const term& s = *_s;
	termid v;
	bool r;
	if (ISVAR(s)) {
		v = evalvar(s, ssub);
		r = v ? unify_snovar_ep(*v, ssub, d) : true;
	}
	else if (ISVAR(d)) return true;
	else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) r = false;
	else if (!s.s) r = true;
	else if ((r = unify_ep(s.s, ssub, *d.s)))
		r = unify_ep(s.o, ssub, *d.o);
	return r;
}

bool prover::unify_ep(termid _s, const term& d, const subs& dsub) {
	PROFILE(++unifs);
	if (!_s) return false;
	setproc("unify_ep");
	const term& s = *_s;
	termid v;
	bool r;
	if (ISVAR(s)) return true;
	else if (ISVAR(d)) {
		v = evalvar(d, dsub);
		r = v ? unify_sdnovar_ep(s, *v, dsub) : true;
	}
	else if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) r = false;
	else if (!s.s) r = true;
	else if ((r = unify_ep(s.s, *d.s, dsub)))
		r = unify_ep(s.o, *d.o, dsub);
	return r;
}

bool prover::unify_snovar(const term& s, const subs& ssub, termid _d, subs& dsub) {
	PROFILE(++unifs);
	const term& d = *_d;
	if (ISVAR(d)) {
		termid v = evalvar(d, dsub);
		if (v) return unify_sdnovar(s, ssub, *v, dsub);
//		dsub[d.p] = evaluate(s, ssub);
		dsub.emplace(d.p, evaluate(s, ssub));
		TRACE(dout<<"new sub:"<<format(dsub[d.p])<<endl);
		return true;
	}
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	if (!s.s) return true;
	if (!unify(s.s, ssub, d.s, dsub)) return false;
	return unify(s.o, ssub, d.o, dsub);
}

bool prover::unify_snovar_dvar(const term& s, const subs& ssub, const term& d, subs& dsub) {
	PROFILE(++unifs);
	termid v = evalvar(d, dsub);
	if (v) return unify_sdnovar(s, ssub, *v, dsub);
//	dsub[d.p] = evaluate(s, ssub);
	dsub.emplace(d.p, evaluate(s, ssub));
	TRACE(dout<<"new sub:"<<format(dsub[d.p])<<endl);
	return true;
}

bool prover::unify_dnovar(termid _s, const subs& ssub, const term& d, subs& dsub) {
	PROFILE(++unifs);
	const term& s = *_s;
	if (ISVAR(s)) {
		termid v = evalvar(s, ssub);
		return v ? unify_sdnovar(*v, ssub, d, dsub) : true;
	}
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	if (!s.s) return true;
	if (!unify(s.s, ssub, d.s, dsub)) return false;
	return unify(s.o, ssub, d.o, dsub);
}

bool prover::unify_snovar_dvar_ep(const term& s, const subs& ssub, const term& d, const subs& dsub) {
	PROFILE(++unifs);
	termid v = evalvar(d, dsub);
	return v ? unify_sdnovar_ep(s, ssub, *v, dsub) : true;
}

bool prover::unify_dnovar_ep(termid _s, const subs& ssub, const term& d, const subs& dsub) {
	PROFILE(++unifs);
	setproc("unify");
	const term& s = *_s;
	if (ISVAR(s)) {
		termid v = evalvar(s, ssub);
		return v ? unify_sdnovar_ep(*v, ssub, d, dsub) : true;
	}
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	if (!s.s) return true;
	if (!unify_ep(s.s, ssub, *d.s, dsub)) return false;
	return unify_ep(s.o, ssub, *d.o, dsub);
}

bool prover::unify(termid _s, termid _d, subs& dsub) {
	PROFILE(++unifs);
	if (!_s || !_d) return !_s == !_d;
	const term& d = *_d, &s = *_s;
	if (ISVAR(s)) return true;
	if (ISVAR(d)) {
		termid v = evalvar(d, dsub);
		if (v) return unify_sdnovar(s, *v, dsub);
		//dsub[d.p] = evaluate(s);
		dsub.emplace(d.p, evaluate(s));
		TRACE(dout<<"new sub:"<<format(dsub[d.p])<<endl);
		return true;
	}
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	if (!s.s) return true;
	if (!unify(s.s, d.s, dsub)) return false;
	return unify(s.o, d.o, dsub);
}

bool prover::unify_dnovar(termid _s, const term& d, subs& dsub) {
	PROFILE(++unifs);
	setproc("unify");
	const term& s = *_s;
	if (ISVAR(s)) return true;
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	if (!s.s) return true;
	if (!unify(s.s, d.s, dsub)) return false;
	return unify(s.o, d.o, dsub);
}

bool prover::unify_sdnovar(const term& s, const subs& ssub, const term& d, subs& dsub) {
	PROFILE(++unifs);
	setproc("unify");
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	if (!s.s) return true;
	if (!unify(s.s, ssub, d.s, dsub)) return false;
	return unify(s.o, ssub, d.o, dsub);
}

bool prover::unify_sdnovar_ep(const term& s, const subs& ssub, const term& d) {
	PROFILE(++unifs);
	setproc("unify");
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	if (!s.s) return true;
	if (!unify_ep(s.s, ssub, *d.s)) return false;
	return unify_ep(s.o, ssub, *d.o);
}

bool prover::unify_sdnovar_ep(const term& s, const term& d, const subs& dsub) {
	PROFILE(++unifs);
	setproc("unify");
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	if (!s.s) return true;
	if (!unify_ep(s.s, *d.s, dsub)) return false;
	return unify_ep(s.o, *d.o, dsub);
}

bool prover::unify_sdnovar_ep(const term& s, const subs& ssub, const term& d, const subs& dsub) {
	PROFILE(++unifs);
	setproc("unify");
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	if (!s.s) return true;
	if (!unify_ep(s.s, ssub, *d.s, dsub)) return false;
	return unify_ep(s.o, ssub, *d.o, dsub);
}

bool prover::unify_sdnovar(const term& s, const term& d, subs& dsub) {
	PROFILE(++unifs);
	setproc("unify");
	if (!(s.p == d.p && !s.s == !d.s && !s.o == !d.o)) return false;
	if (!s.s) return true;
	if (!unify(s.s, d.s, dsub)) return false;
	return unify(s.o, d.o, dsub);
}

