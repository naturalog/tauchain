#include <iostream>
#include "malloc.h"

using namespace std;

#define LLL cout << endl << __FILE__ << ":" << __LINE__ << ":" << endl;



int func(int* a, int *b) __attribute__ ((noinline)) ;

#define R //__restrict__

int func(int  * R  a,   int * R  b)
{
//   LLL
switch((*a) & 0b111)
{
case 0:
	*b = *a*2;
case 1: 
	*b = *a*3;
case 2: 
	*a = *b*2;
case 3:
	*a = *b*3;
case 4:
	*b = *a*4;
case 5: 
	*b = *a*5;
case 6: 
	*a = *b*4;
case 7:
	*a = *b*5;
}
   return 0;
}


int main()
{
LLL

size_t f = 50152921;
int* ar = (int*)malloc(f*sizeof(int));
for(int j = 0; j < 1; j++)
{
	size_t i = 5;
	
	while (i++ < f-10)
		func(ar + i, ar +   f -   i);
	
}
}
