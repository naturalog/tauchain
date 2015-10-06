#include <functional>
#include "vm.h"
#include "triples.h"

const size_t max_frames = 1e+6;
const size_t max_gnd = 1e+5;
struct frame {
	int h, b; // head and body ids
	frame* prev;
	vm trail;
};

list<frame> proof;
list<frame*> gnd;

map<int/*head*/,map<int/*body*/, map<int/*head*/, vm>>> intermediate;
map<int/*head*/,map<int/*body*/, map<int, std::function<bool(vm&)>>>> checkers;
map<int/*head*/,map<int/*body*/, map<int, std::function<void(vm&)>>>> appliers;

wostream& operator<<(wostream& os, const vm& r) {
	for (auto e : r.eqs) {
		auto x = e->begin();
	        for(;;) {
			if (!*x) { os << "B "; ++x; continue; }
			os << **x;
			if (++x != e->end()) os << " = ";
			else { os << ";" << endl; break; }
		}
	}
	return os;
}

// var cannot be bound to something that refers to it
bool occurs_check(const res *x, const res *y) {
	if (x == y) return true;
	if (islist(*y))
		for (const res **r = y->args; *r; ++r)
			if (occurs_check(x, *r))
				return true;
	return false;
}

bool compile(const res *x, const res *y, vm& c) {
	if (x == y) return true;
	bool xvar = isvar(*x), yvar = isvar(*y);
	if (xvar && occurs_check(x, y)) return false;
	if (yvar && occurs_check(y, x)) return false;
	if (!xvar && !yvar) {
		if (!islist(*x) || !islist(*y))
			return false;
		const res **rx, **ry;
		for (rx = x->args, ry = y->args; *rx && *ry; ++rx, ++ry)
			if (!*rx != !*ry || !compile(*rx, *ry, c))
				return false;
	}
	if (islist(*x) && islist(*y)) return true;
	return c.apply(x, xvar, y, yvar);
}

// calculate conditions given two triples
bool compile(const triple *x, const triple *y, vm& c) {
	if (!x || !y) return false;
	dout << "preparing " << *x << " and " << *y;
	FOR(n, 3)
		if (!compile(x->r[n], y->r[n], c))
			return dout << " failed." << endl, c.clear(), false;
	return dout << endl << " passed with vm:" << endl << c << endl, true;
}

// calculate conditions for kb
void compile() {
	vm c;
	FOR(n, rules.size())
		FOR(k, rules[n].body.size())
			FOR(m, rules.size()) {
				if (compile(rules[n].body[k], rules[m].head, c)) {
					intermediate[n][k][m] = c;
					checkers[n][k][m] = [c](vm& v) {return v.check(c); };
					appliers[n][k][m] = [c](vm& v) { v.apply(c); };
				}
				c.clear();
			}
}
/*
int *bsizes;

// return a function that pushes new frames according to the static
// conditions and the dynamic frame
function<void()> compile(vm& _c) {
	map<int, function<bool()>> r;
	// we break the body's function to many functions
	// per head, in order to efficiently update the compiled
	// code when one rule changes
	for (vm::const_iterator x = _c.begin(); x != _c.end(); ++x) {
		int h = x->first;
		const vmet& c = x->second;
		r[h] = [c, h]() {
			const res *u, *v;
			for (auto y : get<0>(c)) {
				u = y.first, v = y.second;
				if (!first->trail.apply(u, isvar(*u), v, isvar(*v), true))
					return false;
			}
			for (auto y : get<1>(c)) {
				u = y.first, v = y.second;
				if (!first->trail.apply(u, isvar(*u), v, isvar(*v), true))
					return false;
			}
			for (auto y : get<2>(c)) {
				u = y.first, v = y.second;
				if (!first->trail.apply(u, isvar(*u), v, isvar(*v), true))
					return false;
			}
			last->h = h, last->b = 0, last->prev = first, ++last;
			return true;
	};	}
	return [r]() {
		// TODO: EP. hence need to compile a func for every head to unify with itself.
		for (auto x : r) {
			++last;
			if (!x.second()) --last;
		}
		if (first->b == bsizes[first->h]) {
			if (!first->prev)
				*gnd++ = first;
			else // TODO: unify in the opposite direction here (as in euler)
				++(*++last = *first).b;
		}
	};
}

// compile whole kb
void compile() {
	compile();
	bsizes = new int[rules.size()];
	FOR(n, rules.size())
		if (!(bsizes[n] = rules[n].body.size()))
			program[n][0] = [](){return true;};// fact
		else FOR(k, bsizes[n])
			program[n][k] = compile(intermediate[n][k]);
}

void run() {
	last = first;
	FOR(n, rules.size())
		if (!rules[n].head) // push queries
			(++last)->h = n, last->b = 0, last->prev = 0;
	do {
		// update env, safely assuming it's a valig merge (otherwise
		// this frame wouldn't be created). this has to be compiled too,
		// applying static resources according to body-head pair.
		// we also compile a function per body that pushes the frames
		// by calling check() for statically-coded resources.
		// conversly we can compile one func with one bool param (check/apply)
		// and a goto. the check/apply calls and params are identical, though
		// being called on different locations in the proof flow.
		if (first->prev) first->v.apply(first->prev->v, false, true);
		// execute the func that is indexed by the current frame's head&body id
		program[first->h][first->b]();
		// TODO: run a proof trace collector thread here
		// as well as garbage collector.
	} while (++first <= last);
}
*/
int main() { din.readdoc(), print(), compile()/*run()*/; }

