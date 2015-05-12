#include <iostream>
#include <cstdlib>
#include <string>
#include <ctime>
#include <algorithm>
#include <vector>
using namespace std;
// http://listofrandomnames.com",
vector<string> names = { "<a:Enrique>", "<a:Arianne>", "<a:Muriel>", "<a:Santina>", "<a:Miguel>", "<a:Easter>", "<a:Arla>", "<a:Karisa>", "<a:Bailey>", "<a:Brice>", "<a:Della>", "<a:Pamala>", "<a:Mikel>", "<a:Roselyn>", "<a:Malisa>", "<a:Tommye>", "<a:Lucien>", "<a:Norma>", "<a:Fidela>", "<a:Loralee>", "<a:Waylon>", "<a:Ching>", "<a:Dewitt>", "<a:Saturnina>", "<a:Russel>", "<a:Helaine>", "<a:Gena>", "<a:Darlena>", "<a:Julianna>", "<a:Marylin>", "<a:Blanca>", "<a:Amee>", "<a:Adina>", "<a:Ashlea>", "<a:Louanne>", "<a:Merlyn>", "<a:Min>", "<a:Vivian>", "<a:Jaquelyn>", "<a:Mitchell>", "<a:Ka>", "<a:Suellen>", "<a:Jefferey>", "<a:Nicolette>", "<a:German>", "<a:Nisha>", "<a:Justine>", "<a:Debra>", "<a:Alycia>", "<a:Valarie>" };

string r() {
	return names[rand() % names.size()];
}

string cf = " <a:goesTo> <a:theCoffee> ";
string mv = " <a:goesTo> <a:theMovie> ";

string o() {
	if (rand() % 2) return cf;
	return mv;
}

bool n3;

ostream& p(string s, string po, string c = "") {
	if (n3) cout << '{';
	cout << s << po<<' ';
	if (n3) cout << '}';
	else cout<<c;
	return cout;
}

int main(int argc, char** argv) {
	n3 = argc == 2;
	srand(time(0));
	random_shuffle(names.begin(), names.end());
	uint sz = names.size();
	for (uint k = 0; k < 1; ++k) 
		for (uint n = 0; n < sz; ++n) {
			string s = names[n];
			random_shuffle(s.begin() + 3, --s.end());
			names.push_back(s);
		}

	for (uint n = 0; n < names.size(); ++n) {
		string w = o();
		p(names[n], w, string("A_") + names[n]) << (n3 ? "<=" : ".\n");
		for (uint k = 0; k < sz; ++k)
			p( r(), o(), string("B_") + names[n] ) << "." <<endl;
		if (!n3) cout << string("B_") + names[n] << " => " << string("A_") + names[n] << '.' << endl;
	}
	for (uint n = 0; n < sz/4; ++n) 
		cout << r() << o() << '.' << endl;
	if (n3)
		cout << "{?x" << cf <<". ?x" << mv <<"}=>{?x <a:staysAt> <a:Home>}."<<endl;
	else {
		cout << "?x" << cf <<"T1."<<endl;
		cout << "?x" << mv <<"T1."<<endl;
		cout << "?x staysAt Home T2."<<endl;
		cout << "T1 => T2."<<endl;
		cout<<"fin."<<endl<<"?y staysAt Home."<<endl<<"fin."<<endl;
	}
}
