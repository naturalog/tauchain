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
comp* sb;


atom compile_atom(int p) {
	int val = p, state = 0;
	bool bound = false;
	std::function<bool(int&)> u;

	if (p < 0)
		u = [p, val, bound, state](int& x) mutable {
		auto env = [=]() {
			cout << KMAG << 41 << ": ";
			std::cout << "p"" = " << (p) << '\t';
			std::cout << "val"" = " << (val) << '\t';
			std::cout << "bound"" = " << (bound) << '\t';
			std::cout << "state"" = " << (state) << '\t';
			std::cout << "x"" = " << (x) << '\t';
			cout << KNRM <<endl;
		};
		env();
		std::cout << KGRN << "switch (state) { case 0: if (!bound) { (val = x, bound = true, state = 1); return true; } case 1: (bound = false, state = 2); return val == x; } return false;" << KNRM << endl;
		switch (state) {
		case 0:
			if (!bound) {
				(val = x, bound = true, state = 1);
				return true;
			}
		case 1:
			(bound = false, state = 2);
			return val == x;
		}
		return false;
		;
	};
	else u = [p, state](int& x) mutable {
		auto env = [=]() {
			cout << KMAG << 55 << ": ";
			std::cout << "p"" = " << (p) << '\t';
			std::cout << "state"" = " << (state) << '\t';
			std::cout << "x"" = " << (x) << '\t';
			cout << KNRM <<endl;
		};
		env();
		std::cout << KGRN << "return x == p" << KNRM << endl;
		return x == p;
	};

	return [p, u, val, state](int& x, bool unify) mutable {
		auto env = [=]() {
			cout << KMAG << 60 << ": ";
			std::cout << "p"" = " << (p) << '\t';
			std::cout << "val"" = " << (val) << '\t';
			std::cout << "state"" = " << (state) << '\t';
			std::cout << "x"" = " << (x) << '\t';
			std::cout << "unify"" = " << (unify) << '\t';
			std::cout << "(sb-(comp*)&u)"" = " << ((sb-(comp*)&u)) << '\t';
			cout << KNRM <<endl;
		};
		env();
		std::cout << KGRN << "if (unify) return u(x); if (!state) { (x = val, state = 1); return true; } return false;" << KNRM << endl;
		if (unify) return u(x);
		if (!state) {
			(x = val, state = 1);
			return true;
		}
		return false;






		;
	};
}

comp compile_triple(atom s, atom o) {
	int state = 0;
	return [s, o, state](int& _s, int& _o) mutable {
		auto env = [=]() {
			cout << KMAG << 75 << ": ";
			std::cout << "state"" = " << (state) << '\t';
			std::cout << "_s"" = " << (_s) << '\t';
			std::cout << "_o"" = " << (_o) << '\t';
			std::cout << "(sb-(comp*)&s)"" = " << ((sb-(comp*)&s)) << '\t';
			std::cout << "(sb-(comp*)&o)"" = " << ((sb-(comp*)&o)) << '\t';
			cout << KNRM <<endl;
		};
		env();
		std::cout << KGRN << "switch (state) { case 0: while (s(_s, true)) { while (o(_o, true)) { state = 1; return true; case 1: ; } } state = 2; } return false;" << KNRM << endl;
		switch (state) {
		case 0:
			while (s(_s, true)) {
				while (o(_o, true)) {
					state = 1;
					return true;
				case 1:
					;
				}
			}
			state = 2;
		}
		return false;
		;
	};
}

comp compile_unify(comp x, comp y) {
	uint state = 0;
	int ss = 0, so = 0;
	return [x, y, state, ss, so](int& s, int& o) mutable {
		auto env = [=]() {
			cout << KMAG << 96 << ": ";
			std::cout << "s"" = " << (s) << '\t';
			std::cout << "o"" = " << (o) << '\t';
			std::cout << "state"" = " << (state) << '\t';
			std::cout << "ss"" = " << (ss) << '\t';
			std::cout << "so"" = " << (so) << '\t';
			std::cout << "(sb-(comp*)&x)"" = " << ((sb-(comp*)&x)) << '\t';
			std::cout << "(sb-(comp*)&y)"" = " << ((sb-(comp*)&y)) << '\t';
			cout << KNRM <<endl;
		};
		env();
		std::cout << KGRN << "switch (state) { case 0: while (x(ss, so)) { while (y(ss, so)) { state = 1; return true; case 1: ; } state = 2; return false; } default: return false; }" << KNRM << endl;
		switch (state) {
		case 0:
			while (x(ss, so)) {
				while (y(ss, so)) {
					state = 1;
					return true;
				case 1:
					;
				}
				state = 2;
				return false;
			}
		default:
			return false;
		}
		;
	};
}

comp compile_match(comp x, comp y, int& a) {
	uint state = 0;
	return [x, y, a, state](int& s, int& o) mutable {
		auto env = [=]() {
			cout << KMAG << 118 << ": ";
			std::cout << "a"" = " << (a) << '\t';
			std::cout << "s"" = " << (s) << '\t';
			std::cout << "o"" = " << (o) << '\t';
			std::cout << "state"" = " << (state) << '\t';
			std::cout << "(sb-(comp*)&x)"" = " << ((sb-(comp*)&x)) << '\t';
			std::cout << "(sb-(comp*)&y)"" = " << ((sb-(comp*)&y)) << '\t';
			cout << KNRM <<endl;
		};
		switch (state) {
		case 0:
			while (x(a, o)) {
				while (y(s, a)) {
					return (bool)(state = 1);
				case 1:
					;
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
		auto env = [=]() {
			cout << KMAG << 137 << ": ";
			std::cout << "state"" = " << (state) << '\t';
			std::cout << "s"" = " << (s) << '\t';
			std::cout << "o"" = " << (o) << '\t';
			std::cout << "(sb-(comp*)&f)"" = " << ((sb-(comp*)&f)) << '\t';
			std::cout << "(sb-(comp*)&r)"" = " << ((sb-(comp*)&r)) << '\t';
			cout << KNRM <<endl;
		};
		env();
		std::cout << KGRN << "switch (state) { case 0: while (f(s, o)) { return (bool)(state = 1); case 1: ; } while (r(s, o)) { return (bool)(state = 2); case 2: ; } state = 3; default: return false; }" << KNRM << endl;
		switch (state) {
		case 0:
			while (f(s, o)) {
				return (bool)(state = 1);
			case 1:
				;
			}
			while (r(s, o)) {
				return (bool)(state = 2);
			case 2:
				;
			}
			state = 3;
		default:
			return false;
		}
	};
}

comp nil = [](int&, int&) {
	return false;
};

void test() {
	comp c = nil;
	int n = 0, s, o;
	for (; n < 4; ++n)
		c = compile_triples(compile_triple(compile_atom(n), compile_atom(n+1)), c);
	n = 0;
	auto env = [=]() {
		cout << KMAG << 163 << ": ";
		std::cout << "n"" = " << (n) << '\t';
		std::cout << "s"" = " << (s) << '\t';
		std::cout << "o"" = " << (o) << '\t';
		cout << KNRM <<endl;
	};
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

	comp m = compile_unify(t1, t2);
	int s, o;
	while (m(s, o)) cout << s << ' ' << o << endl;
}
