#include "n2vm.h"
#include "n3driver.h"

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
