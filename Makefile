tau: jsonld.cpp json_spirit.h
	g++ -std=c++1y jsonld.cpp -lcurl -otau -Wall -ggdb

euler: euler.cpp
	clang++ -std=c++1y euler.cpp   -Wall -ggdb -o euler

