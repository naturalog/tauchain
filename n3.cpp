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

string ctx = "@default";
map<string, string> prefixes;
bool addcomma = false;

void n3(string s, bool isq) {
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
	const size_t sz = s.size();
	cout<<s<<endl;
	if (sz && s[0] == '#')
		return 0;
	prover::term* t;
	cout<<"[{\"@graph\":\"@id:\":\""<<ctx<<"\",[";
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
		if (s[0] == '{') {
			if (s[sz - 1] != '}')
				throw runtime_error(string("Error: expected } in ") + s);
			if (s.size() > 1 && substr(s, 1, s.size() - 2).find('{') == string::npos) {
				string x = between(s, '{', '}');
				trim(x);
			} else {
				// we expect to have a bare (no {}) predicate
				string p = between(x, '}', '{');
				trim(p);
				
			}
		}
		
		vector<string> a;
		split( a, s, is_any_of(" "), token_compress_on );
		if (a.size() == 2) {
			string su = a[0], p = a[1], p = a[2];
			trim(su);
			trim(p);
			trim(o);
			if (addcomma) cout<<',';
      			printf( "\"@id\": \"%s\", \"%s\": { \"@id\": \"%s\" } } ", s.c_str(), p.c_str(), o.c_str());
			addcomma = true;
			cout << "s: " << a[0] << " p: " << a[1] << " o: " << a[2] << endl;
		}
	}
	cout<<"]}";
//	if (a.size() == 2)
//		return mkpred(a[0].c_str(), a[1].c_str(), a[2].c_str());
}

int main(int, char**) {
	string s;
	while (getline(cin, s, '.'))
		n3(s);
	return 0;
}
