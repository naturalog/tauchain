#include <cstring>
#include <cstdio>
#include "strings.h"
//#include "prover.h"
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

vector<string> split_dots(string s) {
	vector<string> r;
	int curly = 0;
	int angle = 0;
	bool quotes = false;
	const char* x = s.c_str();
	const char* y = s.c_str();
	stringstream ss;
	while (*x) {
		switch (*x) {
		case '{': curly++; break;
		case '}': curly--; break;
		case '<': angle++; break;
		case '>': angle--; break;
		case '\"': 
			if (!(x-y) || ((x-y) == 1 && *(x-1) != '\\' && ((x-y) != 2 && *(x-2) != '\\')))
				quotes = !quotes;
			break;
		default: break;
		}
		ss << *x;
		if ((!quotes && !curly && !angle && *x == '.') || !*(x+1)) {
			string t = ss.str();
			r.push_back(t);
			ss.str("");
			ss.clear();
			y = ++x;
		} else
			++x;
	}
	return r;
}

string n3(string s) {
	stringstream ss;
	trim(s);
	replace_all(s, "\n", " ");
	replace_all(s, "\r", " ");
	const size_t sz = s.size();
	if (sz && s[0] == '#')
		return "";
//	prover::term* t;
	ss<<"[{\"@graph\":\"@id:\":\""<<ctx<<"\",[";
	if (starts_with(s, "@prefix")) {
		// @prefix name: <url>.
		string ns, url;
		ns = between(s, ' ', '<');
		trim(ns);
		if (ns == ":") ns = "";
		url = between(s, '<', '>');
		trim(url);
//		cout<<"@prefix ns: " << ns << " url: " << url << endl;
		if (!ns.size())
			ctx = ns;
		else
			prefixes[ns] = url;
	} else {
		if (s[0] == '{') {
			if (s[sz - 1] != '}')
				throw runtime_error(string("Error: expected } in ") + s);
			if (s.size() > 1 && s.substr(1, s.size() - 2).find('{') == string::npos) {
				string x = between(s, '{', '}');
				trim(x);
				auto a = split_dots(x);
				ss << "[{\"@graph\":[";
				for (auto z : a)
					ss << n3(z);
				ss << "]";
			} else {
				// we expect to have a bare (no {}) predicate
				string p = between(s, '}', '{');
				trim(p);
      				ss<< "\"@id\": \""
					<< n3(between(s.substr(s.substr(0, s.size()-1).find('{')),'{','}') )
					<< "\", \""
					<< p
					<< "\": { \"@id\": \"" 
					<< n3(between(s.substr(s.substr(1, s.size()-1).find('{')),'{','}') ) 
					<<"\" } } ";
			}
		}
		
		vector<string> a;
		split( a, s, is_any_of(" "), token_compress_on );
		if (a.size() == 2) {
			string su = a[0], p = a[1], o = a[2];
			trim(su);
			trim(p);
			trim(o);
			if (addcomma) ss<<',';
      			ss<< "\"@id\": \""<<su<<"\", \""<<p<<"\": { \"@id\": \""<<o<<"\" } } ";
			addcomma = true;
//			cout << "s: " << a[0] << " p: " << a[1] << " o: " << a[2] << endl;
		}
	}
	ss<<"]}";
	return ss.str();
//	if (a.size() == 2)
//		return mkpred(a[0].c_str(), a[1].c_str(), a[2].c_str());
}

int main(int, char**) {
	string line;
	stringstream ss;
	while ( getline(cin, line) ) {
		trim(line);
		if (line[0] == '#')
			continue;
		ss << line;
	}
	for (string s : split_dots(ss.str()) )
		cout<<n3(s)<<endl;
	return 0;
}
