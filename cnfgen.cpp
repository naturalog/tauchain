// -lcryptominisat4 -std=c++11
#include <cryptominisat4/cryptominisat.h>
#include <assert.h>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <ctime>
using namespace CMSat;
using namespace std;

vector<int> triple(int m) {
	int x = rand() % (2*m) - m;
	int y = rand() % (2*m) - m;
	int z = rand() % (2*m) - m;
	if (abs(x) == abs(y)) return triple(m);
	if (abs(z) == abs(y)) return triple(m);
	if (abs(x) == abs(z)) return triple(m);
	if (!x || !y || !z) return triple(m);
	cout << x << ' ' << y << ' ' << z << " 0" << endl;
	return { x, y, z };
}

int main(int argc, char** argv) {
	srand(time(0));
	float sat = 0, tot = 0;
	bool res = true;
	for(int u = 0; u < 5; ++u) {
		int k = 200; // clauses
		int n = 50; // vars
		sat=tot=0;
		while(res) {
			SATSolver solver;
			vector<Lit> clause;
			solver.set_num_threads(4);
			solver.new_vars(n);
			for (int m = 0; m < k; ++m) {
				auto t = triple(n);
				for (auto x : t) clause.push_back(Lit(abs(x)-1, x>=0));
				solver.add_clause(clause);
				clause.clear();
			}
			lbool ret = solver.solve();
			if ((res=(ret == l_True))) /*cout << "SAT"*/; else { cout << "UNSAT" << endl; return 0; }
			cout << endl;
			++tot;
		}
		cout << endl;
	}
	return 0;
}

