#include "prover.h"
/*
string prover::emit(ruleid r) {
	std::wstringstream ss;
	std::wostream& os = ss; // dout;
//	termid _t = heads[r];
//	if (!_t) {
//		os << L"#define match" << r << "(x, y) false" << endl;
//		return ss.str();
//	}
	term& t = heads[r];
	os << L"match" << r << L"(term& t, const subs& s) {" << endl;
	
	os << L"}" << endl;
}
*/
#ifndef LAMBDA
termid term::evvar(const subs& ss) {
	static subs::const_iterator it;
	return ((it = ss.find(p)) == ss.end()) ? 0 : it->second->p < 0 ? 0 : it->second->evaluate(ss);
}

termid term::evpred(const subs&) {
	return this;
}

termid term::evaluate(const subs& ss) {
	if (p < 0) return evvar(ss);
	if (!s && !o) return evpred(ss);
	if (!s || !o) throw 0;
	return ev(ss);
}

#define UNIFVAR(x) { \
		PROFILE(++unifs); \
		if (!_d) return false; \
		setproc(L"unify_var"); \
		static termid v; \
		if ((v = evaluate(ssub))) { \
			return v->x(ssub, _d, dsub); \
		} \
		static subs::const_iterator it; \
		if ((it = dsub.find(p)) != dsub.end() && it->second != _d && it->second->p > 0) return false; \
		dsub[p] = _d; \
		return true; \
	}
//			(v = evaluate(dsub)) ? v->x(ssub, _d, dsub) : true; 
#define UNIFVAREP(x) { \
		PROFILE(++unifs); \
		if (!_d) return false; \
		setproc(L"unify_var"); \
		static termid v; \
		if ((v = evaluate(ssub))) return v->x(ssub, _d, dsub); \
		return true; \
	}
//			(v = evaluate(dsub)) ? v->x(ssub, _d, dsub) : true; 
bool term::unifvar(const subs& ssub, term* _d, subs& dsub) UNIFVAR(unify)
bool term::unifvar_ep(const subs& ssub, term* _d, const subs& dsub) UNIFVAREP(unify_ep)

bool term::unif(const subs& ssub, term* _d, subs& dsub) {
	if (!_d) return false;
	static termid v;
	term& d = *_d;
	if (!d.s) return false;
	if (ISVAR(d)) {
		if ((v = d.evaluate(dsub))) return unify(ssub, v, dsub);
//		if ((v = d.evaluate(ssub))) return unify(ssub, v, dsub);
		dsub.emplace(d.p, evaluate(ssub));
		if (dsub[d.p]->s) throw 0;
		return true;
	}
	return p == d.p && s->unify(ssub, d.s, dsub) && o->unify(ssub, d.o, dsub);
}

bool term::unifpred(const subs& ssub, term* _d, subs& dsub) {
	if (!_d) return false;
	static termid v;
	term& d = *_d;
	if (d.s) return false;
	if (ISVAR(d)) {
		if ((v = d.evaluate(dsub))) return unify(ssub, v, dsub);
//		if ((v = d.evaluate(ssub))) return unify(ssub, v, dsub);
		dsub.emplace(d.p, evaluate(ssub));
		if (dsub[d.p]->s) throw 0;
		return true;
	}
	return p == d.p;
}

bool term::unif_ep(const subs& ssub, term* _d, const subs& dsub) {
	if (!_d) return false;
	static termid v;
	term& d = *_d;
	if (!d.s) return false;
	if (ISVAR(d)) {
		if ((v = d.evaluate(dsub))) return unify_ep(ssub, v, dsub);
//		if ((v = d.evaluate(ssub))) return unify_ep(ssub, v, dsub);
		return true;
	}
	return p == d.p && s->unify_ep(ssub, d.s, dsub) && o->unify_ep(ssub, d.o, dsub);
}

bool term::unifpred_ep(const subs& ssub, term* _d, const subs& dsub) {
	if (!p) return false;
	if (!_d) return false;
	static termid v;
	term& d = *_d;
	if (d.s) return false;
	if (ISVAR(d)) {
		if ((v = d.evaluate(dsub))) return unify_ep(ssub, v, dsub);
//		if ((v = d.evaluate(ssub))) return unify_ep(ssub, v, dsub);
		return true;
	}
	return p == d.p;
}
bool term::unify_ep(const subs& ssub, term* _d, const subs& dsub) {
	if (!p) return false;
	if (p < 0) return unifvar_ep(ssub, _d, dsub);
	if (!s) return unifpred_ep(ssub, _d, dsub);
	return unif_ep(ssub, _d, dsub);
}
bool term::unify(const subs& ssub, term* _d, subs& dsub) {
	setproc(L"unify_bind");
	bool r;
	if (!p) return false;
	TRACE(dout << "Trying to unify " << prover::format(this) << " sub: " << prover::formats(ssub) << " with " << prover::format(_d) << " sub: " << prover::formats(dsub) << endl);
	if (p < 0) r = unifvar(ssub, _d, dsub);
	else if (!s) r = unifpred(ssub, _d, dsub);
	else r = unif(ssub, _d, dsub);
	TRACE(
		dout << "Trial to unify " << prover::format(this) << " sub: " << prover::formats(ssub) << " with " << prover::format(_d) << " sub: " << prover::formats(dsub) << " : ";
		if (r) dout << "passed";
		else dout << "failed";
		dout << endl);
	return r;
}
#endif
bool prover::unify(termid _s, const subs& ssub, term* _d, subs& dsub) {
//	PROFILE(++unifs);
	if (!_s || !_d) return !_s == !_d;
	return _s->unify(ssub, _d, dsub);
/*	setproc(L"unify_bind");
	termid v;
	bool r, ns = false;
	term& d = *_d, &s = *_s;
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

bool prover::unify_ep(term* _s, const subs& ssub, term& _d, const subs& dsub) {
	PROFILE(++unifs);
	if (!_s) return false;
	setproc(L"unify_ep");
	return _s->unify_ep(ssub, &_d, dsub);
/*	term& s = *_s;
	termid v;
	bool r;
	if (ISVAR(s)) {
		v = evaluate(s, ssub);
		r = v ? unify_ep(v, ssub, _d, dsub) : true;
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

