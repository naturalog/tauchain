CC=g++
#CXXFLAGS=-c -std=c++11 -Wall -Wextra -W -Wpedantic -I/usr/local/include/compute -ggdb -DDEBUG
CXXFLAGS=-c -std=c++11 -Wall -Wextra -W -Wpedantic -I/usr/local/include/compute -O3
LDFLAGS=-lcurl -lboost_system -lboost_filesystem -pthread -lOpenCL
OBJECTS=tau.o jsonld.o rdf.o misc.o object.o cli.o prover.o nquads.o match.o

all: tau
tau: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

match.o: match.cpp prover.h
nquads.o: nquads.cpp rdf.h
cli.o: cli.cpp prover.h misc.h rdf.h object.h parsers.h jsonld.h \
 json_spirit.h strings.h cli.h
jsonld.o: jsonld.cpp jsonld.h json_spirit.h object.h strings.h rdf.h
misc.o: misc.cpp prover.h misc.h rdf.h object.h parsers.h jsonld.h \
 json_spirit.h strings.h
object.o: object.cpp object.h
prover.o: prover.cpp misc.h object.h prover.h rdf.h parsers.h jsonld.h \
 json_spirit.h strings.h
rdf.o: rdf.cpp cli.h rdf.h object.h parsers.h jsonld.h json_spirit.h \
 strings.h prover.h misc.h
tau.o: tau.cpp cli.h rdf.h object.h parsers.h jsonld.h json_spirit.h \
 strings.h prover.h misc.h
marpa.o: marpa.cpp cli.h rdf.h object.h parsers.h jsonld.h json_spirit.h \
 strings.h prover.h misc.h

debug: CXXFLAGS += -DDEBUG
irc: CXXFLAGS += -DIRC -DDEBUG
marpa: CXXFLAGS += -Dmarpa OBJECTS += marpa.o

marpa: $(OBJECTS) marpa.o $(EXECUTABLE)
	$(CC) $(OBJECTS) -o tau $(LDFLAGS)

debug: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o tau $(LDFLAGS)
irc: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o tau $(LDFLAGS)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
ubi-tau: $(OBJECTS) ubi/client.o
	$(CC) $(OBJECTS) ubi/client.o -o $@ $(LDFLAGS)
.cpp.o:
	$(CC) $(CXXFLAGS) $< -o $@

clean:
	rm -rf tau $(OBJECTS) ubi/client.o

ppjson: ppjson.cpp
	clang++ -std=c++11 ppjson.cpp -oppjson -Wall -ggdb
