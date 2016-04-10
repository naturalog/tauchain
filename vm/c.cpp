#include "n3driver.h"

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
