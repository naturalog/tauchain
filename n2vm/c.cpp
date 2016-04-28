#include "n2vm.h"
#include "n3driver.h"

using n3driver::din;
//using n3driver::term;
using n3driver::terms;
using n3driver::rules;

void print() {
	dout << "rules: " << rules.size() << endl;
	for (auto r : rules) dout << r << endl;
}

int main() {
	terms.push_back(new term(L"GND"));
	din.readdoc(false);
	print();
	din.readdoc(true);
	print();
}
