#include "n2vm.h"
#include "n3driver.h"
#include <sstream>

void test1() {
	std::wstringstream in;
	din_t din(in);
	n2vm vm;
	din.readdoc(false, vm);
}

int main() {
	din_t din(wcin);
	n2vm vm;
	vector<rule> k = din.readdoc(false, vm);
	k.append(din.readdoc(true, vm));
	dout << "rules: " << k.size() << endl;

	for (auto r : k) dout << r << endl;
	vm.add_rules(&k[0], k.size());
	int s = 0;
	while (vm.tick()) dout << "iter: " << s++ << endl;
	return 0;
}
