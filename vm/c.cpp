#include "n3driver.h"

void print() {
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
	st.terms.push_back(new ast::term("GND"));
	readdoc(false);
	//print();
//	din.readdoc(true);
	print();
}
