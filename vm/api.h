#include "ir.h"

namespace api {
template<typename T> struct nextable { virtual T next() const = 0; };
struct term : public nextable<int> { };
struct rule : public nextable<term*> { term *h; };

ir::match* build(term &b, const vector<rule*>& ast) {
	
}

void build(rule &r, const vector<rule*>& ast, ir::kb_t& res) {
	ir::match *m;
	ir::rule &rl = *new ir::rule;
	auto &v = *new vector<ir::match*>;
	for (term *b = r.next(); b; b = r.next()) {
		if ((m = build(*b, ast))) v.push_back(m);
		rl.push_back(v);
	}

}

ir::kb_t* build(const vector<rule*>& ast) {
	auto res = new ir::kb_t;
	for (rule* r : ast) build(*r, ast, *res);
	return res;
}

};
