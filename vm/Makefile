all: n3driver.h containers.h ir.h strings.c strings.h ast.h ast.cpp
	gcc strings.c -ggdb -c -ostrings.o
	g++ c.cpp n3driver.cpp containers.cpp ir.cpp rule.cpp ast.cpp strings.o -std=c++1y -ggdb -occpp
bset: bset.h bset.cpp
	g++ bset.cpp -std=c++1y -g
