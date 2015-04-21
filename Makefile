all: jsonld.h  json_spirit.h parsers.h  reasoner.cpp
	g++ -std=c++1y reasoner.cpp -lcurl `pkg-config raptor2 --cflags` `pkg-config raptor2 --libs` -otau -Wall -ggdb3 
test: jsonld.h  json_spirit.h parsers.h  reasoner.cpp
	g++ -std=c++1y reasoner.cpp -lcurl `pkg-config raptor2 --cflags` `pkg-config raptor2 --libs` -otau -Wall -ggdb3 -DTEST
clean:
	rm tau
