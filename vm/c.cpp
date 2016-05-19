#include "ast.h"

void print(const ast& st) {
	dout << "rules: " << st.rules.size() << endl;
	uint n = 0;
	word kb;
	for (auto r : st.rules) {
		word k = r->crds(++n);
		for (auto x : k) kb.insert(x);
	}
	dout << kb << endl;
	for (auto r : st.rules) *r >> dout << endl;
}

int main() {
//	strings_test();
	ast st;
	st.terms.push_back(new ast::term("GND"));
	readdoc(false, &st);
	//print();
//	din.readdoc(true);
	print(st);
}
