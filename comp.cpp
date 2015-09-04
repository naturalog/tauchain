#include <functional>
#include <iostream>
using namespace std;

typedef std::function<bool(int&, bool)> atom;
typedef std::function<bool(int&, int&)> comp;

atom compile_atom(int p) {
	int val = p, state = 0;
	bool bound = false;
	std::function<bool(int&)> u;

	if (p < 0)
		u = [p, val, bound, state](int& x) mutable {
			switch (state) {
			case 0: if (!bound) return val = x, bound = true, (bool)(state = 1);
			case 1: return val = p, bound = false, (bool)(state = 2);
			}
			return false;
		};
	else u = [p, state](int& x) mutable { return x == p; };

	return [u, val, state](int& x, bool unify) mutable {
		if (unify)
			return u(x);
		if (!state) return x = val, (bool)(state = 1);
		return false;
	};
}

comp compile_triple(atom s, atom o) {
	bool state = false;
	return [s, o, state](int& _s, int& _o) mutable {
		switch (state) {
		case 0: while (s(_s, false)) {
				while (o(_o, false)) {
					return (bool)(state = 1);
		case 1:			;
				}
			}
			state = 2;
		}
		return false;
	};
}

comp nil = [](int&, int&) { return false; };

comp compile_triples(comp f, comp r) {
	uint state = 0;
	return [f, r, state](int& s, int& o) mutable {
		switch (state) {
		case 0: while (f(s, o)) {
				return (bool)(state = 1);
		case 1: 	;
			}
			while (r(s, o)) {
				return (bool)(state = 2);
		case 2:		;
			}
			state = 3;
		default:
			return false;
		}
	};
}

comp compile_match(comp x, comp y, int& a) {
	uint state = 0;
	return [x, y, a, state](int& s, int& o) mutable {
		switch (state) {
		case 0: while (x(a, o)) {
				while (y(s, a)) {
					return (bool)(state = 1);
		case 1:			;
				}
				state = 2;
				return false;
			}
		default:
			return false;
		}
	};
}

comp compile_triples(comp f, atom s, atom o) { return compile_triples(f, compile_triple(s, o)); }
comp fact(int s, int o, comp f = nil) { return compile_triples(f, compile_atom(s), compile_atom(o)); } 
comp fact(atom s, atom o, comp f = nil) { return compile_triples(f, s, o); }

ostream& operator<<(ostream& os, atom a) {
	int aa;
	while (a(aa, false)) os << aa << ' ';
	return os;
}

ostream& operator<<(ostream& os, comp p) {
	int s = 0, o = 0;
	while (p(s, o))
		os << s << ' ' << o << endl;
	return os;
}

int main() {
	comp kb = nil;
/*	atom socrates = compile_atom(1);
//	atom A = compile_atom(2);
	atom man = compile_atom(2);
	atom mortal = compile_atom(3);
	atom x = compile_atom(-1);
	kb = fact(socrates, man, kb);
	kb = fact(x, man, kb);
	kb = fact(x, mortal, kb);
//	kb = fact(1, 2, kb);
*/
	atom x = compile_atom(1);
	atom y = compile_atom(2);
	atom z = compile_atom(3);
	atom v = compile_atom(-1);
	comp t1 = compile_triple(x, y);
	comp t2 = compile_triple(v, y);
	comp t3 = compile_triple(z, z);
	kb = compile_triples(t1, compile_triples(t2, compile_triples(t3, nil)));
	int ii = 1;
	comp m = compile_match(kb, kb, ii);
	cout << kb << endl;
	int s, o;
	while (m(s, o)) cout << s << o << endl;
}
