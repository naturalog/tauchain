#include "triples.h"

struct vm {
	typedef list<const res*> row;
	// struct eq expresses that all listed
	// resources have to be equal: x=?y=?z=...=?t
	struct eq : row {
		using row::row;
		bool bound() const { return !front(); }
		bool free() const { return front(); }
	};
	list<sp<eq>> eqs;
	// function apply(x,y): try to put x,y at the same row and keep the state valid.
	// on our context that means: mark them as required them to be equal.
	// rules of state validation:
	// 1. all rows must contain at most one nonvar.
	// 2. nonvars must be the second element in a row, while
	// the first element in a row containing a nonvar must be null.
	// this is a referred as a bounded row. nulls must not appear anywhere else.
	// 3. every element appears at most once in at most one row.
	// 4. once element was put in a row, it cannot be moved, except for merging 
	// whole rows in a way that doesn't violate the above conditions.
	bool apply(const res* x, bool xvar, const res* y, bool yvar,
			bool checkOnly = false, bool dontEvenCheck = false) {
		// TODO: the condition at the following line must be checked
		// externally ahead of time and not rechecked here.
		list<sp<eq>>::iterator n, k;
		findrow(x, n, y, k);
		if (dontEvenCheck || !check(xvar, yvar, *n, *k)) return false;
		return checkOnly || apply(x, n, xvar, y, k, yvar);
	}
	list<sp<eq>>::iterator findrow(const res* x) {
		for (list<sp<eq>>::iterator e = eqs.begin(); e != eqs.end(); ++e)
			for (const res* r : **e) if (r == x) return e;
		return eqs.end();
	}
	void findrow(const res* x, list<sp<eq>>::iterator& n, const res* y, list<sp<eq>>::iterator& k) {
		char f = 0;
		n = k = eqs.end();
		for (list<sp<eq>>::iterator e = eqs.begin(); e != eqs.end() && f < 2; ++e)
			for (const res* r : **e)
				if 	(r == x){ n = e; ++f; }
				else if (r == y){ k = e; ++f; }
	}
	bool apply(const res* x, list<sp<eq>>::iterator& itn, bool xvar, const res* y, list<sp<eq>>::iterator& itk, bool yvar) {
		sp<eq> n = *itn, k = *itk;
		// at the following logic we take
		// into account that check() returned
		// true so we avoid duplicate checks.
		if (!n) { // x never appears
			if (!k) { // y never appears
				if (!xvar) eqs.push_front(new eq(0, x, y));
				else if (!yvar) eqs.push_front(new eq(0, y, x));
				else eqs.push_front(new eq(x, y));
			} else	// if y appears and x doesn't, push x to y's row
				// (we can assume we may do that since check() succeeded)
				push(x, xvar, *k);
		} else if (!k) // relying on check(), if x appears and y doesn't, 
			push(y, yvar, *n);// push y to x's row
		else if (n != k) // if x,y appear at different rows, we can safely merge
			merge(itn, itk); // these two rows (again relying on check())
		return true;
	}
	void clear() { eqs = list<sp<eq>>(); }
private:	
	void push(const res* x, bool var, eq& e) { var ? e.push_back(x) : e.push_front(x), e.push_front(0); }
	void merge(list<sp<eq>>::iterator& n, list<sp<eq>>::iterator& k) {
		(*n)->free() ? (*k)->splice(*eqs.erase(n)) : (*n)->splice(*eqs.erase(k));
	}
	bool check(bool xvar, bool yvar, sp<eq> n, sp<eq> k) {
		// if x never appears, require that y never appears, or
		// that x is a variable (hence unbounded since it never appears),
		// or that y's row is unbounded (so even nonvar x can be appended
		// to it and bind all vars in it)
		if (!n) return !k || xvar || k->free();
		// if k never appears but x appears, require that
		// either y is a var or that y's row is unbounded
		if (!k) return yvar || n->free();
		// if x,y both appear, require that x,y to either appear at the same
		// row, or that one of the rows they appear at is unbounded.
		return n == k || n->front() || k->free();
	}
//	bool apply(const vm& v) { }
};
