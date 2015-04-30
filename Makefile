CC=clang++
CXXFLAGS=-c -std=c++1y -Wall -rdynamic -ggdb -Wextra -W
LDFLAGS=-lcurl

tau.o: tau.cpp cli.h reasoner.h parsers.h jsonld.h json_spirit.h object.h \
 strings.h rdf.h misc.h
	$(CC) $(CXXFLAGS) $< -o $@
jsonld.o: jsonld.cpp jsonld.h json_spirit.h object.h strings.h rdf.h
	$(CC) $(CXXFLAGS) $< -o $@
rdf.o: rdf.cpp jsonld.h json_spirit.h object.h strings.h rdf.h
	$(CC) $(CXXFLAGS) $< -o $@
reasoner.o: reasoner.cpp reasoner.h parsers.h jsonld.h json_spirit.h \
 object.h strings.h rdf.h misc.h
	$(CC) $(CXXFLAGS) $< -o $@
misc.o: misc.cpp reasoner.h parsers.h jsonld.h json_spirit.h object.h \
 strings.h rdf.h misc.h
	$(CC) $(CXXFLAGS) $< -o $@

debug: CXXFLAGS += -DDEBUG

all: tau.o jsonld.o rdf.o reasoner.o misc.o
debug: tau.o jsonld.o rdf.o reasoner.o misc.o

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
ubi-tau: $(OBJECTS) ubi/client.o
	$(CC) $(OBJECTS) ubi/client.o -o $@ $(LDFLAGS)
.cpp.o:
	$(CC) $(CXXFLAGS) $< -o $@

clean:
	rm -rf tau $(OBJECTS) ubi/client.o

ppjson: ppjson.cpp
	clang++ -std=c++1y ppjson.cpp -lcurl -oppjson -Wall -rdynamic -ggdb
