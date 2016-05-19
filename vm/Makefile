all: containers.h strings.c strings.h ast.h ast.cpp
	gcc strings.c -ggdb -c -ostrings.o
#	gcc n3.c -ggdb -c -on3.o -std=c99
	g++ c.cpp n3driver.cpp containers.cpp ast.cpp strings.o n3.o -std=c++1y -ggdb -occpp
bset: bset.h bset.cpp
	g++ bset.cpp -std=c++1y -g
