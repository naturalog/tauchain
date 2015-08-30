#include <cstdlib>
#include <iostream>
using namespace std;

struct atom {
	int id;
	atom(int i) : id(i), value(this), x(this), y(this) {}
	atom(const atom& a) : atom(a.id) {}
	atom(atom&& a) : atom(a.id) {}
	virtual atom* evaluate() { return this; }
	virtual bool isvar() const { return false; }
	virtual bool operator()(atom& a) {
		switch (state) {
		case 0:
			x = evaluate(), y = a.evaluate();
			if (x->isvar()) {
				value = x->value;
				while ((*x)(*y)) {
					return value = x->value, state = 1;
		case 1:			;
				}
				return (state = 0);
			} else if (y->isvar()) {
				while ((*y)(*x)) {
					return value = y->value, state = 2;
		case 2:			;
				}
				return (state = 0);
			}
		}
		return a.id == id;
	}
	virtual atom& operator*() { return *evaluate(); }
	virtual atom* operator->() { return evaluate(); }
	atom *value;
private:
	atom *x, *y;
	size_t state = 0;
};

typedef atom res;

struct var : atom {
	using atom::atom;
	virtual bool isvar() const { return true; }
	virtual atom* evaluate() { return bound ? value : this; }
	virtual bool operator()(atom& a) {
		bool r;
		if (!state && !bound) {
			value = &a, state = 1;
			r = bound = true;
		} else if (state == 1)
			bound = false, state = 0, r = (**value)(a), value = (*value)->value;
		cout << "unification of\t" << id << "\twith\t" << a.id;
		if (value) cout << "\tunder " << value->id;
		cout << "\treturned " << r << endl;
		return r;
	}
private:
	bool bound = false;
	size_t state = 0;
};

ostream& operator<<(ostream& os, const struct term& t);

struct term {
	atom *p, *s, *o;
	term(){}
	term(int _p, int _s = 0, int _o = 0) : 
		p(_p > 0 ? (atom*)new res(_p) : (atom*)new var(_p)), 
		s(_s > 0 ? (atom*)new res(_s) : (atom*)new var(_s)), 
		o(_o > 0 ? (atom*)new res(_o) : (atom*)new var(_o))
		{ cout << "added term: " << *this << endl; }
	bool unify(term& t) {
		bool r;
		if (!t.s != !s || !t.o != !o || p->id != t.p->id) r = false;
		else r = (*s)(*t.s) && (*o)(*t.o);
		cout << "unification of\t" << *this << "\twith\t" << t << "\treturned " << r << endl;
		return r;
	}
	term *value = 0;
	size_t nbody = 0, state = 0, n, k;
	static term** heads;
	static size_t nheads;

	virtual bool operator()() {
		switch (state) {
		case 0: for (n = 0; n < nbody; ++n)
				for (k = 0; k < nheads; ++k) {
					if (body[n]->unify(*heads[k])) {
						value = heads[k];
						return (state = 1);
					}
		case 1:			;
				}
			return (state = 0);
		}
		return (state = 0);
	}

	term& operator*() { return *value; }
	term* operator->() { return value; }

	term& addbody(term* t) {
		body = (term**)realloc(body, sizeof(term*) * ++nbody);
		body[nbody-1] = t;
		return *this;
	}
	term **body = 0;
};

term** term::heads = 0;
size_t term::nheads = 0;

term& addhead(term* t) {
	term::heads = (term**)realloc(term::heads, sizeof(term*) * ++term::nheads);
	term::heads[term::nheads-1] = t;
	return *t;
}

ostream& operator<<(ostream& os, const term& t) { 
	os << t.p->id; 
	if (t.s) os << '(' << t.s->id << ',' << t.o->id << ')';
	if (t.nbody) { 
		os << " <= {";
		for (size_t n = 0; n < t.nbody; ++n)
			os << (*t.body[n]) << ' ';
		os << "}";
	}
	return os;
}

bool prove(term& t) {
	while (t()) {
	//	if (&t == &*t || !t->p || !t->p->id) continue;
		cout << "head: " << t << endl << " body: " << *t << endl;
//		if (t->nbody && !prove(*t)) continue;//return false;
		prove(*t);
	}
	return true;
}

int main() {
	int socrates = 1, a = 2, man = 3, mortal = 4;
	int x = -1, y = -1;
	addhead(new term(a, socrates, man));
	addhead(new term(a, 5, man));
	addhead(new term(a, x, mortal)).addbody(new term(a, x, man));
	cout << prove((new term(0))->addbody(new term(a, y, mortal))) << endl;
}
