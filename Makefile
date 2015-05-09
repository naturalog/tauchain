CC=clang++-3.5
CXXFLAGS=-c -std=c++1y -Wall -rdynamic -ggdb
LDFLAGS=-lcurl
SOURCES=tau.cpp jsonld.cpp rdf.cpp

debug: CXXFLAGS += -DDEBUG

OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=tau


all: $(SOURCES) $(EXECUTABLE)
debug: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CXXFLAGS) $< -o $@

clean:
	rm -rf tau $(OBJECTS) 

euler: euler.hmc.cpp
	g++ -std=c++1y euler.hmc.cpp -oeuler -Wall -ggdb

ppjson: ppjson.cpp
	clang++ -std=c++1y ppjson.cpp -lcurl -oppjson -Wall -rdynamic -ggdb
