#CC=clang++-3.6
CC=g++
#CXXFLAGS=-c -std=c++11 -W -Wall -Wextra -Wpedantic -g -ggdb -DDEBUG -O0 -I/usr/local/include -I/usr/include -I/usr/local/linuxbrew/include -DJSON #-DPREDVARS -DNOPARSER
CXXFLAGS= -c -std=c++11 -Wall -Wextra -I/usr/local/include -DNDEBUG -O3 -I/usr/include -I/usr/local/linuxbrew/include -I/usr/include -I/usr/local/linuxbrew/include #-DJSON -DNOPARSER
LDFLAGS= -L/usr/local/lib #-ldl -pthread -lrt
#OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJECTS= prover.o unifiers.o tau.o jsonld.o rdf.o misc.o json_object.o cli.o nquads.o

all: tau
tau: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp `g++ -std=c++11 $(CXXFLAGS) -M %.cpp`

with_marpa: marpa_tau.o libmarpa/dist/.libs/libmarpa.so

libmarpa/dist/.libs/libmarpa.so:
	git submodule init
	git submodule update
	cd libmarpa;	make dist;	cd dist;	./configure;	make

with_marpa: OBJECTS += marpa_tau.o
with_marpa: CXXFLAGS += -Dwith_marpa  -I libmarpa/dist -ggdb  #-Ilexertl
with_marpa: LDFLAGS += -Llibmarpa/dist/.libs -lmarpa  -ggdb -lboost_regex
debug: CXXFLAGS += -DDEBUG
release: CXXFLAGS -= -DDEBUG CXXFLAGS -= -ggdb CXXFLAGS += -O3 -NDEBUG
cl: CXXFLAGS += -DOPENCL
irc: CXXFLAGS += -DIRC -DDEBUG

with_marpa: $(OBJECTS) $(EXECUTABLE)
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
dimacs2tau: dimacs2tau.cpp
	$(CC) -std=c++11 dimacs2tau.cpp -odimacs2tau -Wall -ggdb
pcpp: p.cpp p.h containers.h
	$(CC) -W -Wall -Wpedantic -Wextra -DDEBUG p.cpp -opcpp -g -std=c++11 -O3
mltt: mltt.cpp
	$(CC) -W -Wall -Wpedantic -Wextra -DDEBUG mltt.cpp -omltt -g -std=c++11 -O3
jit: jit.c dict.cpp
	gcc -Wall -Wpedantic -Wextra jit.c -ojit.o -c -g -DDEBUG
	g++ -W -Wall -Wpedantic -Wextra jit.o dict.cpp -ojit -g
comp: comp.cpp
	$(CC) -W -Wall -Wpedantic -Wextra -std=c++11 comp.cpp -ocomp -g -rdynamic
