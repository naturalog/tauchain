#include "n3driver.h"
//#include "ir.h"

//using n3driver::din;
//using n3driver::term;
//using n3driver::rule;
//using n3driver::terms;
//using n3driver::rules;

void print() {
	dout << "rules: " << rules.size() << endl;
	uint n = 0;
	word l, v;
	for (auto r : rules) r->crds(l, v, ++n);
	dout << "l:" <<endl<< l << "v:" << endl << v << endl;
	for (auto r : rules) *r >> dout << endl;
}

int main() {
//	strings_test();
	terms.push_back(new term("GND"));
	din.readdoc(false);
	//print();
	din.readdoc(true);
	print();
}
