#include <functional>
#include <iostream>
using namespace std;

typedef std::function<bool(int&, bool)> atom;
typedef std::function<bool(int&, int&)> comp;

const char KNRM[] = "\x1B[0m";
const char KRED[] = "\x1B[31m";
const char KGRN[] = "\x1B[32m";
const char KYEL[] = "\x1B[33m";
const char KBLU[] = "\x1B[34m";
const char KMAG[] = "\x1B[35m";
const char KCYN[] = "\x1B[36m";
const char KWHT[] = "\x1B[37m";

#define ENV(x) std::cout << #x" = " << (x) << '\t';
#define ENV2(x,y) ENV(x); ENV(y);
#define ENV3(x,y,z) ENV2(x,y); ENV(z);
#define ENV4(x,y,z,t) ENV2(x,y); ENV2(z,t);
#define ENV5(x,y,z,t,a) ENV4(x,y,z,t); ENV(a);
#define ENV6(x,y,z,t,a,b) ENV4(x,y,z,t); ENV2(a,b);
#define ENV7(x,y,z,t,a,b,c) ENV4(x,y,z,t); ENV3(a,b,c);
#define E(y) auto env = [=](){ cout << KMAG << __LINE__ << ": ";y;cout << KNRM <<endl;};
//#define EMIT(x) env(), std::cout << __LINE__ << " I: " << #x << endl, x
#define EMIT(x) \
	env();\
	std::cout << KGRN << #x << KNRM << endl;\
	x

comp* sb;
#define PTR(x) (sb-(comp*)&x)
#define cvp
// (int*)

atom compile_atom(int p) {
	int val = p, state = 0;
	bool bound = false;
	std::function<bool(int&)> u;

	if (p < 0)
		u = [p, val, bound, state](int& x) mutable {
			E(ENV5(p, val, bound, state, x));
			EMIT(
			switch (state) {
			case 0: if (!bound) {
					(val = x, bound = true, state = 1);
					return true;
			}
			case 1: (bound = false, state = 2);
				return val == x;
			}
			return false;
			);
		};
	else u = [p, state](int& x) mutable {
		E(ENV3(p, state, x));
		EMIT(return x == p);
	};

	return [p, u, val, state](int& x, bool unify) mutable {
		E(ENV6(p, val, state, x, unify, PTR(u)));
		EMIT(
		if (unify)
			return u(x);
		if (!state) {
			(x = val, state = 1);
			return true;
		}
		return false;);
	};
}

comp compile_triple(atom s, atom o) {
	int state = 0;
	return [s, o, state](int& _s, int& _o) mutable {
		E(ENV5(state, _s, _o, PTR(s), PTR(o)));
		EMIT(
		switch (state) {
		case 0: while (s(_s, true)) {
				while (o(_o, true)) {
					state = 1;
					return true;
		case 1:			;
				}
			}
			state = 2;
		}
		return false;
		);
	};
}

comp compile_unify(comp x, comp y) {
	uint state = 0;
	int ss = 0, so = 0;
	return [x, y, state, ss, so](int& s, int& o) mutable {
		E(ENV7(s, o, state, ss, so, PTR(x), PTR(y)));
		EMIT(
		switch (state) {
		case 0: while (x(ss, so)) {
				while (y(ss, so)) {
					state = 1;
					return true;
		case 1:			;
				}
				state = 2;
				return false;
			}
		default:
			return false;
		}
		);
	};
}

comp compile_match(comp x, comp y, int& a) {
	uint state = 0;
	return [x, y, a, state](int& s, int& o) mutable {
		E(ENV6(a, s, o, state, PTR(x), PTR(y)));
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

comp compile_triples(comp f, comp r) {
	uint state = 0;
	return [f, r, state](int& s, int& o) mutable {
		E(ENV5(state, s, o, PTR(f), PTR(r)));
		EMIT(
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
		})
	};
}

comp nil = [](int&, int&) { return false; };

void test() {
	comp c = nil;
	int n = 0, s, o;
	for (; n < 4; ++n)
		c = compile_triples(compile_triple(compile_atom(n), compile_atom(n+1)), c);
	n = 0;
	E(ENV3(n, s, o));
	while (c(s, o)) {
		cout << "##";
		env();
		n++;
	}
}

int main() {
	cout << endl;
	comp kb = nil;
	sb = &kb;
	test();
	return 0;
	atom x = compile_atom(1);
	atom y = compile_atom(2);
	atom z = compile_atom(3);
	atom v = compile_atom(-1);
	comp t1 = compile_triple(x, y);
	comp t2 = compile_triple(v, y);
	comp t3 = compile_triple(z, z);
	kb = compile_triples(t1, compile_triples(t2, compile_triples(t3, nil)));
	int ii = 1;
//	comp m = compile_match(kb, kb, ii);
	comp m = compile_unify(t1, t2);
	int s, o;
	while (m(s, o)) cout << s << ' ' << o << endl;
}
