#include <string>
#include <iostream>
using namespace std;
// can be one of the following:
// x:y (x is of type y)
// (x=y):z (x equals y in z)

struct expr {
	virtual operator string() const = 0;
};

struct label : public expr {
	string x;
	label(string s) : x(s) {}
	virtual operator string() const { return x; }
};

struct var : public label {
	using label::label;
	virtual operator string() const { return string("?") + x; }
};

struct intro : public expr {
	expr *e, *t;
	intro(expr *_e, expr *_t) : e(_e), t(_t) {}
	virtual operator string() const { return string(*e) + string(":") + string(*t); }
};

struct eq : public expr {
	expr *x, *y;
	eq(expr *_x, expr *_y) : x(_x), y(_y) {}
	virtual operator string() const { return string(*x) + string("=") + string(*y); }
};

int main() {
	// 
	intro i1(new eq(new var("a"), new label("b")), new label("C"));
	cout << (string)intro(new eq(new var("aa"), &i1), new label("CC")) << endl;
}
