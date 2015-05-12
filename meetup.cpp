#include <iostream>
#include <cstdlib>
#include <string>
#include <ctime>
#include <algorithm>
#include <vector>
using namespace std;
// http://listofrandomnames.com",
vector<string> names = { "Enrique", "Arianne", "Muriel", "Santina", "Miguel", "Easter", "Arla", "Karisa", "Bailey", "Brice", "Della", "Pamala", "Mikel", "Roselyn", "Malisa", "Tommye", "Lucien", "Norma", "Fidela", "Loralee", "Waylon", "Ching", "Dewitt", "Saturnina", "Russel", "Helaine", "Gena", "Darlena", "Julianna", "Marylin", "Blanca", "Amee", "Adina", "Ashlea", "Louanne", "Merlyn", "Min", "Vivian", "Jaquelyn", "Mitchell", "Ka", "Suellen", "Jefferey", "Nicolette", "German", "Nisha", "Justine", "Debra", "Alycia", "Valarie" };

string r() {
	return names[rand() % names.size()];
}

string cf = " goesTo theCoffee ";
string mv = " goesTo theMovie ";

string o() {
	if (rand() % 2) return cf;
	return mv;
}

int main() {
	srand(time(0));
	random_shuffle(names.begin(), names.end());
	for (uint n = 0; n < names.size(); ++n) {
//		if (n % 10 != 0) continue;
		string w = o();
		cout << names[n] << w << string("A_") + names[n] << "."<<endl;
		cout << r() << w << string("B_") + names[n] << "."<<endl;
//		cout << r() << o() << string("B_") + names[n] << "."<<endl;
//		cout << r() << o() << string("B_") + names[n] << "."<<endl;
//		cout << r() << o() << string("B_") + names[n] << "."<<endl;
		cout << string("B_") + names[n] << " => " << string("A_") + names[n] << '.' << endl;
	}
	for (uint n = 0; n < 5; ++n) 
		cout << r() << o() << '.' << endl;
	cout<<"fin."<<endl<<"?x goesTo theCoffee."<<endl;
	cout<<"?y goesTo theMovie."<<endl<<"fin."<<endl;
}
