#include <functional>
#include <iostream>
using namespace std;

typedef std::function<bool(int&, bool)> atom;
typedef std::function<bool(int&, int&)> comp;

#define ENV(x) std::cout << " " << #x" = " << (x) << ' ';
#define ENV2(x,y) ENV(x); ENV(y);
#define ENV3(x,y,z) ENV2(x,y); ENV(z);
#define ENV4(x,y,z,t) ENV2(x,y); ENV2(z,t);
#define ENV5(x,y,z,t,a) ENV4(x,y,z,t); ENV(a);
#define ENV6(x,y,z,t,a,b) ENV4(x,y,z,t); ENV2(a,b);
#define ENV7(x,y,z,t,a,b,c) ENV4(x,y,z,t); ENV3(a,b,c);

atom compile_atom(int p) {
	int val = p, state = 0;
	bool bound = false;
	std::function<bool(int&)> u;

	if (p < 0)
		u = [p, val, bound, state](int& x) mutable {
			ENV5(p, val, bound, state, x);
			switch (state) {
			case 0: if (!bound) 
					return val = x, bound = true, (bool)(state = 1);
			case 1: bound = false, state = 2;
				return val == x;
			}
			return false;
		};
	else u = [p, state](int& x) mutable {
		ENV3(p, state, x);
		cout << "atom unif " << x << " with " << p << endl;
		return x == p;
	};

	return [p, u, val, state](int& x, bool unify) mutable {
		ENV6(&p, &u, val, state, x, unify);
		cout << "atom x: " << x << " unify: " << unify << " p: " << p << " val: " << val << endl;
		if (unify)
			return u(x);
		if (!state)
			return x = val, (bool)(state = 1);
		return false;
	};
}

comp compile_triple(atom s, atom o) {
	int state = 0;
	return [s, o, state](int& _s, int& _o) mutable {
		ENV5(&s, &o, state, _s, _o);
		cout << "CT1 _s: " << _s << " _o: " << _o << ' ' << &_s << ' ' << &_o << endl;
		switch (state) {
		case 0: while (s(_s, true)) {
				cout << "CT2 _s: " << _s << " _o: " << _o << ' ' << &_s << ' ' << &_o << endl;
				while (o(_o, true)) {
					cout << "CT3 _s: " << _s << " _o: " << _o << ' ' << &_s << ' ' << &_o << endl;
					return (bool)(state = 1);
		case 1:			;
				}
			}
			state = 2;
		}
		return false;
	};
}

comp compile_unify(comp x, comp y) {
	uint state = 0;
	int ss = 0, so = 0;
	return [x, y, state, ss, so](int& s, int& o) mutable {
		ENV7(&x, &y, s, o, state, ss, so);
		cout << "U1 s: " << s << " o: " << o << ' ' << &s << ' ' << &o << endl;
		switch (state) {
		case 0: while (x(ss, so)) {
				cout << "U2 s: " << s << " o: " << o << ' ' << &s << ' ' << &o << endl;
				while (y(ss, so)) {
					cout << "U3 s: " << s << " o: " << o << ' ' << &s << ' ' << &o << endl;
					return state = 1, true;
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

comp compile_match(comp x, comp y, int& a) {
	uint state = 0;
	return [x, y, a, state](int& s, int& o) mutable {
		ENV6(&x, &y, a, s, o, state);
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
		ENV5(&f, &r, state, s, o);
		cout << "CTS1 s: " << s << " o: " << o << ' ' << &s << ' ' << &o << endl;
		switch (state) {
		case 0: while (f(s, o)) {
				cout << "CTS2 s: " << s << " o: " << o << ' ' << &s << ' ' << &o << endl;
				return (bool)(state = 1);
		case 1: 	;
			}
			while (r(s, o)) {
				cout << "CTS3 s: " << s << " o: " << o << ' ' << &s << ' ' << &o << endl;
				return (bool)(state = 2);
		case 2:		;
			}
			state = 3;
		default:
			return false;
		}
	};
}

comp nil = [](int&, int&) { return false; };

int main() {
	cout << endl;
	comp kb = nil;
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
