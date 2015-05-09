#include <cstring>
#include <cstdio>
#include "strings.h"
#include "prover.h"
#include <boost/algorithm/string.hpp>
#include <map>
#include <vector>
using namespace std;
using namespace boost::algorithm;

string between(string x, char y, char z) {
	if (x.find(y) == string::npos || x.find(z) == string::npos) {
		stringstream ss;
		ss<<"Error while parsing "<<x<<": looked for chars "<<y<<" and "<<z;
		throw runtime_error(ss.str());
	}
	auto py = x.find(y) + 1, pz = x.rfind(z);
	return x.substr(py, pz - py);
}

prover::term* n3(string s) {
	trim(s);
//	auto pos = s.find('\n');
//	while (pos != string::npos) {
//		s.erase(pos);
//		pos = s.find('\n');
//	}
//	s.erase(remove(s.begin(), s.end(), '\n'), s.end());
//	s.erase(remove(s.begin(), s.end(), '\r'), s.end());
	replace_all(s, "\n", " ");
	replace_all(s, "\r", " ");
	cout<<s<<endl;
	if (s.size() && s[0] == '#')
		return 0;
	prover::term* t;
	string ctx;
	map<string, string> prefixes;
	if (starts_with(s, "@prefix")) {
		// @prefix name: <url>.
		string ns, url;
		ns = between(s, ' ', '<');
		trim(ns);
		if (ns == ":") ns = "";
		url = between(s, '<', '>');
		trim(url);
		cout<<"@prefix ns: " << ns << " url: " << url << endl;
		if (!ns.size())
			ctx = ns;
		else
			prefixes[ns] = url;
	} else {
		vector<string> a;
		split( a, s, is_any_of(" "), token_compress_on );
		if (a.size() == 2) {
			cout << "s: " << a[0] << " p: " << a[1] << " o: " << a[2] << endl;
		}
	}
//	if (a.size() == 2)
//		return mkpred(a[0].c_str(), a[1].c_str(), a[2].c_str());
}

int main(int, char**) {
	string s;
	while (getline(cin, s, '.'))
		n3(s);
	return 0;
}
