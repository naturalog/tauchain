#include "n2vm.h"
#include "n3driver.h"

void print() {
	dout << "rules: " << rules.size() << endl;
	for (auto r : rules) dout << r << endl;
}

int main() {
	din_t din(wcin);
	n2vm vm;
	din.readdoc(false, vm);
	print();
	din.readdoc(true, vm);
	print();
	vm.add_rules(&rules[0], rules.size());
	int s = 0;
	while (vm.tick()) dout << "iter: " << s++ << endl;
	return 0;
}
