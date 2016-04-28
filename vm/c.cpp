#include "n3driver.h"
//#include "ir.h"

using n3driver::din;
using n3driver::atom;
using n3driver::rule;
using n3driver::atoms;
using n3driver::rules;

void print() {
	dout << "rules: " << rules.size() << endl;
	for (auto r : rules) dout << r << endl;
}

int main() {
	atoms.push_back(new atom(L"GND"));
	din.readdoc(false);
	print();
	din.readdoc(true);
	print();
}
