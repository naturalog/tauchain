#include <map>
#include <functional>
#include <cstring>
#include <iostream>
#include <vector>
using namespace std;

typedef const char* pcch;
typedef map<pcch, struct term*> env;

struct term {
	pcch p;
	term **args;
	uint sz;
	const bool var, list;
	term(pcch p) : p(strdup(p)), sz(0), args(0), var(p && *p == '?'), list(false) {}
	term(pcch s, pcch _p, pcch o) : p(0), sz(3), var(false), list(true) {
		args = new term*[3];
		args[0] = new term(s);
		args[1] = new term(_p);
		args[2] = new term(o);
	}
	term(const vector<term*>& v) : p(0), args(new term*[v.size()]), sz(v.size()), var(false), list(true) {
		uint n = 0;
		for (auto x : v) args[n++] = x;
	}
	term(const vector<pcch>& v) : p(0), args(new term*[v.size()]), sz(v.size()), var(false), list(true) {
		uint n = 0;
		for (auto x : v) args[n++] = new term(x);
	}
	term(term *t) : p(p), args(new term*[1]), sz(1), var(false), list(true) { *args = t; }
};

typedef function<void(env e)> result;

inline term* rep(term* x, env& e) {
	if (!x || !x->var) return x;
	env::iterator it = e.find(x->p);
	if (it == e.end()) return x;
	return rep(it->second, e);
}

void run(term *s, term *d, const result *t, const result *f, env e = env()) {
	if (!s != !d) { (*f)(e); return; }
	if (!s) { (*t)(e); return; }
	auto _s = rep(s, e), _d = rep(d, e);
	if (s->list && d->list) {
		uint sz = s->sz;
		if (sz != d->sz) { (*f)(e); return; }
		const result *r = t;
		while (sz--) run(rep(s->args[sz], e), rep(d->args[sz], e), r, f);
	}
	else if	(!_s != !_d) 		(*f)(e);
	else if (!_s) 			(*t)(e);
	else if (_s->var)		(*t)(e);
	else if (_d->var)		e[_d->p] = _s, (*t)(e);
	else if (!_s->p == !_d->p && _s->p && !strcmp(_s->p, _d->p))
		(*t)(e);
	else (*f)(e);
}

ostream& operator<<(ostream& os, const term& t) {
	if (!t.sz) return os << t.p;
	os << '(';
	for (uint n = 0; n < t.sz; ++n) os << *t.args[n] << ' ';
	return os << ')';
}

ostream& operator<<(ostream& os, const env& e) {
	for (auto x : e) os << x.first << '=' << (x.second ? *x.second : "(null)") << ';'; return os;
}

int main() {
//	term *x = new term("x", "p", "o");
	term *x = new term( vector<pcch>{ "?x" });
	term *y = new term( { new term("x"), new term("?y"), new term("?ro") } );
	term *z = new term( { y } );
//	term *y = new term(x);
	result *t = new result, *f = new result;
	*t = [](env e) { cout << "true " << e << endl; };
	*f = [](env e) { cout << "false " << e << endl; };
	run(z, x, t, f);

	return 0;
}
