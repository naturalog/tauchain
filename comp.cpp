#include <functional>
#include <iostream>
using namespace std;

typedef std::function<bool(int&, bool)> atom;
typedef std::function<bool(int&, int&, bool)> comp;

const char KNRM[] = "\x1B[0m";
const char KRED[] = "\x1B[31m";
const char KGRN[] = "\x1B[32m";
const char KYEL[] = "\x1B[33m";
const char KBLU[] = "\x1B[34m";
const char KMAG[] = "\x1B[35m";
const char KCYN[] = "\x1B[36m";
const char KWHT[] = "\x1B[37m";


int _ind = 0;
void ind() { for (int n = 0; n < _ind - 1; ++n) cout << '\t'; }
#define ENV(x) std::cout << #x" = " << (x) << '\t';
#define ENV2(x,y) ENV(x); ENV(y);
#define ENV3(x,y,z) ENV2(x,y); ENV(z);
#define ENV4(x,y,z,t) ENV2(x,y); ENV2(z,t);
#define ENV5(x,y,z,t,a) ENV4(x,y,z,t); ENV(a);
#define ENV6(x,y,z,t,a,b) ENV4(x,y,z,t); ENV2(a,b);
#define ENV7(x,y,z,t,a,b,c) ENV4(x,y,z,t); ENV3(a,b,c);

#define E(x, y) ++_ind;auto env = [&](){ cout << x << ' ' << __LINE__ << ": " << KRED;ind();y;cout << KNRM <<endl;}; \
	struct eonexit { \
		std::function<void()> _f; \
		eonexit(std::function<void()> f):_f(f) {} \
		~eonexit(){cout<<"ret-"; _f();--_ind;}}onexit__(env);
//#define EMIT(x) env(), std::cout << __LINE__ << " I: " << #x << endl, x

//#define E(x) 
#define EMIT(x) \
	env(); \
	x

//	std::cout << __LINE__ << ' ' << KGRN << #x << KNRM << endl;
//	x

//#define EMIT(x) x

comp* sb;
#define PTR(x) ((sb - (comp*)&x)&0xFFF)

atom compile_atom(int p) {
	int val = p, e = 0;
	bool bound = false;
	std::function<bool(int&)> u;

	if (p < 0)
		u = [p, val, bound, e](int& x) mutable {
			E("unify_var", ENV5(p, val, bound, e, x));
			switch (e) {
			case 0: if (!bound && x != p)
					{ EMIT(return (val = x, bound = (e = 1))); }
			case 1: EMIT(return (bound = false, e = 2, val == x);)
			case 2: e = 0, val = p; return false;
			}
			return false;
		};
	else u = [p, e](int& x) mutable {
		E("unify_res", ENV3(p, e, x));
		if (!e) { EMIT(e = 1; return x == p); }
		e = 0;
		return false;
	};

	return [p, u, val, e](int& x, bool unify) mutable {
		E("atom", ENV6(p, val, e, x, unify, PTR(u)));
		if (unify)
			return u(x);
		if (!e)
			{ EMIT(return (x = p, e = 1, true);); }
		EMIT(e = 0; return false);
	};
}

comp compile_triple(atom s, atom o) {
	int e = 0;
	return [s, o, e](int& _s, int& _o, bool u) mutable {
		E("triple", ENV5(e, _s, _o, PTR(s), PTR(o)));
		switch (e) {
		case 0: while (s(_s, u)) {
				while (o(_o, u)) {
					EMIT(
					e = 1;
					return true);
		case 1:			;
				}
			}
			EMIT(e = 2);
		case 2: e = 0; return false;
		}
		return false;
	};
}

comp compile_unify(comp x, comp y) {
	int ss = 0, so = 0, e = 0;
	return [x, y, e, ss, so](int& s, int& o, bool) mutable {
		E("unify", ENV7(s, o, e, ss, so, PTR(x), PTR(y)));
		switch (e) {
		case 0: while (x(s, o, false)) {
				while (y(s, o, true)) {
					EMIT( return (e = 1, true););
		case 1:			;
				}
			}
			EMIT(e = 2);
		case 2: e = ss = so = 0;
			EMIT(return false);
		}
		return false;
	};
}

comp compile_match(comp x, comp y, int& a) {
	uint e = 0;
	return [x, y, a, e](int& s, int& o, bool u) mutable {
		E("match", ENV6(a, s, o, e, PTR(x), PTR(y)));
		switch (e) {
		case 0: while (x(a, o, u)) {
				while (y(s, a, u)) {
					return (bool)(e = 1);
		case 1:			;
				}
				e = 2;
				return false;
			}
		default:
			return false;
		}
	};
} 

comp compile_triples(comp f, comp r) {
	uint e = 0;
	return [f, r, e](int& s, int& o, bool u) mutable {
		E("triples", ENV5(e, s, o, PTR(f), PTR(r)));
		switch (e) {
		case 0: while (f(s, o, u)) {
				return (bool)(e = 1);
		case 1: 	;
			}
			while (r(s, o, u)) {
				return (bool)(e = 2);
		case 2:		;
			}
			e = 3;
		case 3: e = 0; return false;
		}
		return false;
	};
}

comp nil = [](int&, int&, bool) { return false; };

void test() {
	comp c = nil;
	int n = 0, s = 0, o = 0;
	for (; n < 12; ++n)
		c = compile_triples(compile_triple(compile_atom(n), compile_atom(n+1)), c);
	E("test",ENV3(n, s, o));
	while (c(s, o, false)) {
		if (s+1 != o || n != o) throw 0;
//		cout << "##" << n << ' ' << s << ' ' << o << endl;
		n--;
	}
}

int main() {
	cout << endl;
	comp kb = nil;
	sb = &kb;
//	test();
	atom x = compile_atom(1);
	atom y = compile_atom(2);
	atom z = compile_atom(3);
	atom v = compile_atom(-1);
	comp t1 = compile_triple(x, y);
	comp t2 = compile_triple(v, y);
	comp t3 = compile_triple(z, z);
	comp t4 = compile_triple(z, y);
	kb = compile_triples(t1, compile_triples(t2, compile_triples(t3, compile_triples(t4, nil))));
//	comp m = compile_match(kb, kb, ii);
	comp m = compile_unify(kb, t2);
	int s = 0, o = 0;
	while (m(s, o, true)) cout << "Result: " << s << ' ' << o << endl;
}
