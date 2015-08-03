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
	for (auto x : cnf) for (auto y : x) if (abs(y)>N)N=abs(y);

	cout	<<"(T T) or  T."<<endl
		<<"(T F) or  T."<<endl
		<<"(F T) or  T."<<endl
		<<"(F F) or  F."<<endl
		<<"T not F."<<endl
		<<"F not T."<<endl
		<<"{ T not T } => false."<<endl
		<<"{ F not F } => false."<<endl
		<<"{ (?X0 ?Y0) or ?Z0 } => { (?Y0 ?X0) or ?Z. }."<<endl
		<<"{ (?X1 ?Y1) or T. (?Y1 ?Z1) or T } => { (?X1 ?Y1 ?Z1) or T }."<<endl
		<<"{ (?X2 ?Y2) or F. (?Y2 ?Z2) or T } => { (?X2 ?Y2 ?Z2) or T }."<<endl
		<<"{ (?X3 ?Y3) or F. (?Y3 ?Z3) or F } => { (?X3 ?Y3 ?Z3) or F }."<<endl;

	for (auto x : cnf) cout << clause(x) << endl;
	//cout << "fin." << endl << negs(N) << "fin." << endl;
	cout << negs(N) << "fin." << endl;
	cout << "?x a T. " << endl << "fin." << endl;
	return 0;
}
