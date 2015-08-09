#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <cstring>

using namespace std;

string var(int v) {
	stringstream ss;
	if (v > 0) ss << "?p" << v;
	else ss << "?n" << (-v);
	return ss.str();
}

string clause(const vector<int>& c) {
	stringstream ss;
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
	int N = 0;
	string s;
	vector<vector<int>> cnf;
	while (getline(cin, s)) {
		if (s.size() && (s[0] == 'c' || s[0] == 'p')) continue;
		cnf.emplace_back();
		auto& clause = cnf.back();
		int x, y, z;
		sscanf(s.c_str(), "%d %d %d", &x, &y, &z);
		clause.push_back(x);
		N = max(N, x);
		if (y) {
			clause.push_back(y);
			N = max(N, y);
			if (z) {
				clause.push_back(z);
				N = max(N, z);
			}
		}
	}
	cout << "T not F.\nF not T.\n(T) or T.\n(T T) or T.\n(T F) or T.\n(F T) or T.\n(T T T) or T.\n(T T F) or T.\n(T F T) or T.\n(T F F) or T.\n(F T T) or T.\n(F T F) or T.\n(F F T) or T.\n{ ";
	for (auto x : cnf) cout << clause(x) << endl;
	//cout << "fin." << endl << negs(N) << "fin." << endl;
	cout << negs(N) << " } => { CNF a SAT }.\nfin.\n";
	cout << "?X a SAT. " << endl << "fin." << endl;
	return 0;
}
