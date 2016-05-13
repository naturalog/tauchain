all: n3driver.h containers.h ir.h strings.c strings.h
	gcc strings.c -ggdb -c -ostrings.o
	g++ c.cpp n3driver.cpp containers.cpp ir.cpp strings.o -std=c++1y -ggdb -occpp
