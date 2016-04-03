#include <iostream>


using namespace std;

#define LLL cout << endl << __FILE__ << ":" << __LINE__ << ":" << endl;



int func(int* a, int *b) __attribute__ ((noinline)) ;

int func(int *a, int *b)
{
   *a = 5; 
   int bb = *b; 
   return bb + 10;
}


int main()
{
LLL

size_t i = 5;
size_t f = 1152921;
int ar[f];
while (i++ < f-10)
for(int j = 0; j < 100; j++)
	func(ar + i, f+ar-i);
	
	
}
