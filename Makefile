CC=g++
CXXFLAGS=-c -std=c++11 -Wextra -g -DDEBUG -I/usr/local/include
#CXXFLAGS=-c -std=c++11 -Wall -Wextra -W -Wpedantic -O2 -I/usr/local/include
LDFLAGS=-lcurl -lboost_system -lboost_filesystem -pthread -L/usr/local/lib -ldl
#OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJECTS=tau.o jsonld.o rdf.o misc.o object.o cli.o prover.o nquads.o


all: tau
tau: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp `g++ -std=c++11 -M %.cpp`

marpa: marpa.o
marpa:
	OBJECTS += marpa.o
marpa: debug

debug: CXXFLAGS += -DDEBUG
release: CXXFLAGS -= -DDEBUG CXXFLAGS -= -ggdb CXXFLAGS += -O3
cl: CXXFLAGS += -DOPENCL
irc: CXXFLAGS += -DIRC -DDEBUG


debug: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o tau $(LDFLAGS)
release: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o tau $(LDFLAGS)
irc: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o tau $(LDFLAGS)
cl: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o tau $(LDFLAGS) -lOpenCL
ubi-tau: $(OBJECTS) ubi/client.o
	$(CC) $(OBJECTS) ubi/client.o -o $@ $(LDFLAGS)
.cpp.o:
	$(CC) $(CXXFLAGS) $< -o $@
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

clean:
	rm -rf tau $(OBJECTS) ubi/client.o marpa.o

ppjson: ppjson.cpp
	$(CC) -std=c++11 ppjson.cpp -oppjson -Wall -ggdb
