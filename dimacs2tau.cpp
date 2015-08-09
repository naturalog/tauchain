#include <string>
#include <sstream>
#include <iostream>
#include <vector>

using namespace std;

string var(int v) {
	stringstream ss;
	if (v > 0) ss << "?x" << v;
	else ss << "?nx" << (-v);
	return ss.str();
}

string clause(vector<int> c) {
	stringstream ss;
	if (c.size() == 1) {
		ss << '('<<var(c[0])<<" F) or T.";
		return ss.str();
	}
	ss << '(';
	for (auto x : c) ss << var(x) << ' ';
	ss << ") or T.";
	return ss.str();
}

string negs(int n) {
	stringstream ss;
	for (int k = 1; k <= n; ++k) ss << var(k) << " not " << var(-k) << '.' <<endl;
	return ss.str();
}

int main() {
	vector<vector<int>> cnf = {
		{ 1, 2, 3 },
		{ -1, 2, 3 },
		{ 1, -2, 3 },
		{ 1, 2, -3 },
		{ -1, -2, 3 },
		{ 1, -2, -3 }
	};
	int N = 0;
	cout << "T not F.\nF not T.\n(T T T) or T.\n(T T F) or T.\n(T F T) or T.\n(T F F) or T.\n(F T T) or T.\n(F T F) or T.\n(F F T) or T.\n{ ";
	for (auto x : cnf) for (auto y : x) if (abs(y)>N) N = abs(y);
	for (auto x : cnf) cout << clause(x) << endl;
	//cout << "fin." << endl << negs(N) << "fin." << endl;
	cout << negs(N) << " } => { CNF a SAT }.\nfin.\n";
	cout << "?X a SAT. " << endl << "fin." << endl;
	return 0;
}
