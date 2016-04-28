#include "n2vm.h"
#include "n3driver.h"

void print() {
	dout << "rules: " << rules.size() << endl;
	for (auto r : rules) dout << r << endl;
}

int main() {
	terms.push_back(new term(L"GND"));
	n2vm vm;
	din.readdoc(false, vm);
	print();
	din.readdoc(true, vm);
	print();
}
