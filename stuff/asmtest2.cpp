#include <iostream>


using namespace std;

#define LLL cout << endl << __FILE__ << ":" << __LINE__ << ":" << endl;



void func(int* a, int *b) __attribute__ ((noinline)) ;

void func(int* a, int *b) 

{
	*(a) = (*b)+1;
}
	


int main()
{
LLL

size_t i = 5;
size_t f = 1152921;
int ar[f];
while (i++ < f-10)
for(int j = 0; j < 1; j++)
	func(ar + i, ar+f-i);
	
	
}
