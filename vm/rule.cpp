#include <map>
#include <functional>
#include <cstring>
#include <iostream>
using namespace std;

typedef const char* pcch;
typedef map<pcch, const struct term*> env;

#define isvar(x) ((x) && (*(x)->p == '?'))
#define islist(x) ((x) && (!(x)->p))

struct term {
	pcch p;
	term **args;
	uint sz;
	term(pcch p) : p(strdup(p)), sz(0), args(0) {}
	term(pcch s, pcch _p, pcch o) : p(0), sz(3) {
		args = new term*[3];
		args[0] = new term(s);
		args[1] = new term(_p);
		args[2] = new term(o);
	}
};

typedef function<void(env e)> rule;

inline const term* rep(const term* x, const env& e) {
	if (!isvar(x)) return x;
	env::const_iterator it = e.find(x->p);
	if (it == e.end()) return x;
	return rep(it->second, e);
}

const rule* rule_create(const term *s, const term *d, const rule *t, const rule *f) {
	if (!s != !d) return f;
	if (islist(s) && islist(d)) {
		uint sz = s->sz;
		if (sz != d->sz) return f;

		const rule *r = t;
		for (uint n = sz; n--;)
			r = rule_create(s->args[n], d->args[n], r, f);
		return r;
	}
	return &(*new rule = [s, d, t, f](env e) { // two nonlists
		auto _s = rep(s, e), _d = rep(d, e);
		if (!_s != !_d) (*f)(e);
		else if (!_s) (*t)(e);
		else if (isvar(_s)) (*t)(e);
		else if (isvar(_d)) e[_d->p] = _s, (*t)(e);
		else (!_s->p == !_d->p && _s->p && !strcmp(_s->p, _d->p) ? *t : *f)(e);
	});
}

ostream& operator<<(ostream& os, const term& t) {
	os << t.p;
	if (!t.sz) return os;
	os << '(';
	for (uint n = 0; n < t.sz; ++n) os << *t.args[n] << ' ';
	return os << ')';
}
ostream& operator<<(ostream& os, const env& e) {
	for (auto x : e) os << x.first << '=' << (x.second ? *x.second : "(null)") << ';'; return os;
}

int main() {
	term *x = new term("x", "p", "o");
	term *y = new term("x", "?y", "?ro");
	rule *t = new rule, *f = new rule;
	*t = [](env e) { cout << "true " << e << endl; };
	*f = [](env e) { cout << "false " << e << endl; };
	(*rule_create(x, y, t, f))(env());

	return 0;
}
