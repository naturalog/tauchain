all: jsonld.h  json_spirit.h parsers.h  reasoner.h tau.cpp
	g++ -std=c++1y tau.cpp -lcurl -otau -Wall -rdynamic -ggdb
debug: jsonld.h  json_spirit.h parsers.h  reasoner.h tau.cpp
	g++ -std=c++1y tau.cpp -lcurl -otau -Wall -rdynamic -ggdb -DDEBUG
nquads: jsonld.h  json_spirit.h parsers.h  reasoner.h tau.cpp
	clang++ -std=c++1y tau.cpp -lcurl `pkg-config raptor2 --cflags` `pkg-config raptor2 --libs` -otau -Wall -g
ppjson: ppjson.cpp
	clang++ -std=c++1y ppjson.cpp -lcurl -oppjson -Wall -rdynamic -ggdb

clean:
	rm tau ppjson
