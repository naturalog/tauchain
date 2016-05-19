all: containers.h strings.c strings.h refuter.cpp
	gcc strings.c -ggdb -c -ostrings.o
#	gcc n3.c -ggdb -c -on3.o -std=c99
	g++ refuter.cpp n3.cpp containers.cpp strings.o -std=c++1y -ggdb -orefuter
clean:
	rm  -rf strings.o refuter
