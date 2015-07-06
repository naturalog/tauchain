CC=g++
CXXFLAGS=-c -std=c++11 -Wextra -g -DDEBUG -I/usr/local/include
#CXXFLAGS=-c -std=c++11 -Wall -Wextra -W -Wpedantic -O2 -I/usr/local/include
LDFLAGS=-lcurl -lboost_system -lboost_filesystem -pthread -L/usr/local/lib -ldl
#OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJECTS=tau.o jsonld.o rdf.o misc.o object.o cli.o prover.o nquads.o


all: tau
tau: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp `g++ -std=c++11 $(CXXFLAGS) -M %.cpp`

marpa: marpa_tau.o libmarpa/dist/.libs/libmarpa.so

libmarpa/dist/.libs/libmarpa.so:
	git submodule init
	git submodule update
	cd libmarpa
	make dist
	cd dist
	./configure
	make

marpa: OBJECTS += marpa_tau.o
marpa: CXXFLAGS += -Dmarpa  -Ilexertl -I libmarpa/dist  -ggdb
marpa: LDFLAGS += -Llibmarpa/dist/.libs -lmarpa  -ggdb
debug: CXXFLAGS += -DDEBUG
release: CXXFLAGS -= -DDEBUG CXXFLAGS -= -ggdb CXXFLAGS += -O3
cl: CXXFLAGS += -DOPENCL
irc: CXXFLAGS += -DIRC -DDEBUG

marpa: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o tau $(LDFLAGS)
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
	rm -rf tau $(OBJECTS) ubi/client.o marpa.o marpa_tau.o

ppjson: ppjson.cpp
	$(CC) -std=c++11 ppjson.cpp -oppjson -Wall -ggdb
