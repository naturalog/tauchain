all: jsonld.h  json_spirit.h parsers.h  reasoner.h tau.cpp
	clang++ -std=c++1y tau.cpp -lcurl -otau -Wall -ggdb3 -rdynamic
nquads: jsonld.h  json_spirit.h parsers.h  reasoner.h tau.cpp
	clang++ -std=c++1y tau.cpp -lcurl `pkg-config raptor2 --cflags` `pkg-config raptor2 --libs` -otau -Wall -g

clean:
	rm tau
