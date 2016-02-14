//how c++ lambdas work:
//make how


#include <iostream>
#include <vector>
#include <functional>
#include <unordered_map>


using namespace std;

#define LLL cout << endl << __FILE__ << ":" << __LINE__ << ":" << endl;


int main()
{
LLL
	{
		int i;
		for (i = 0; i < 3; i++)
			[]() mutable {
				static int x = 333;
				cout << x++ << endl;
			}();
	}
/*
333
334
335
*/
	
LLL
	{/*//crash
		vector<function<void()>> zzz;
		int xxx;
		auto a = [xxx, zzz]()mutable {
			xxx++;
			cout << xxx << endl;
			zzz[0]();
		};
		zzz.push_back(a);
		a();
	*/}
LLL
	{
		auto xxx = 0;
		auto a = [xxx]()mutable {
			xxx++;
			cout << xxx << endl;
		};
		auto b = a;
		auto c = a;
		a();
		b();
		c();
	}
/*
1
1
1
*/
LLL
	{
		auto xxx = 0;
		auto a = [xxx]()mutable {
			xxx++;
			cout << xxx << endl;
		};
		vector<function<void()>> ooo;
		ooo.push_back(a);
		auto c = ooo[0];
		a();
		c();
	}
/*
1
1
*/
LLL
	{

		vector<int> xxx;
		xxx.push_back(3);

		auto a = [xxx]()mutable {
			xxx[0]++;
			cout << xxx[0] << endl;
		};
		auto b = a;
		auto c = a;
		a();
		b();
		c();
	}
/*
4
4
4
*/
LLL
	{
		int state = 0;
		function<void()> xxx = [state]()mutable { cout << state++ << endl; };
		std::unordered_map<int, function<function<void()>()>> pgs;
		pgs[0] = [xxx]() { return xxx; };
		pgs[1] = [xxx]() { return xxx; };
		pgs[0]()();
		pgs[1]()();
	}
/*
0
0
*/
LLL
	{
		int state = 0, state2 = 0;
		//this is like a rule stuff
		function<void()> fff = [state2]()mutable { cout << "fff " << state2++ << endl; };
		//this is like a pred lambda
		function<void()> xxx = [state, fff]()mutable {
			cout << state++ << endl;
			fff();
		};
		std::unordered_map<int, function<function<void()>()>> pgs;
		//this is the pred copy lambda
		pgs[0] = [xxx]() { return xxx; };
		pgs[1] = [xxx]() { return xxx; };
		pgs[0]()();
		pgs[1]()();
	}
}
/*
0
fff 0
0
fff 0
*/
