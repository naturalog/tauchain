CC=clang++
CXXFLAGS=-c -std=c++1y -Wall -rdynamic -ggdb -Wextra
LDFLAGS=-lcurl
SOURCES=tau.cpp jsonld.cpp rdf.cpp reasoner.cpp misc.cpp

debug: CXXFLAGS += -DDEBUG

ubi-tau: CXXFLAGS += -DDEBUG -DUBI  -I./ubi/
ubi-tau: LDFLAGS += `xmlrpc-c-config  c++2 client --libs --cflags` 

#ubi/client.o: ubi/client.cpp
#	$(CC) `xmlrpc-c-config  c++2 client --libs --cflags` ubi/client.cpp -o ubi/client.o

OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=tau


all: $(SOURCES) $(EXECUTABLE)
debug: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
ubi-tau: $(OBJECTS) ubi/client.o
	$(CC) $(OBJECTS) ubi/client.o -o $@ $(LDFLAGS)
.cpp.o:
	$(CC) $(CXXFLAGS) $< -o $@

clean:
	rm -rf tau $(OBJECTS) ubi/client.o

#all: jsonld.h  json_spirit.h parsers.h reasoner.h strings.h rdf.h logger.h object.h tau.o jsonld.o rdf.o
#	$(CC) tau.o jsonld.o rdf.o -lcurl -otau
#debug: jsonld.h  json_spirit.h parsers.h reasoner.h strings.h rdf.h logger.h object.h tau.o jsonld.o rdf.o
#	$(CC) tau.o jsonld.o rdf.o -lcurl -otau
#nquads: jsonld.h  json_spirit.h parsers.h  reasoner.h tau.cpp
#	clang++ -std=c++1y tau.cpp -lcurl `pkg-config raptor2 --cflags` `pkg-config raptor2 --libs` -otau -Wall -g
ppjson: ppjson.cpp
	clang++ -std=c++1y ppjson.cpp -lcurl -oppjson -Wall -rdynamic -ggdb
#
