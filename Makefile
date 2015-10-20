CXX=clang++

ASAN=  -fno-omit-frame-pointer -fno-optimize-sibling-calls  -Xclang -fcolor-diagnostics -ferror-limit=10 -fsanitize=address -fsanitize=integer -fsanitize=undefined -fsanitize=unsigned-integer-overflow 
DBG= $(ASAN) -DDEBUG -g -ggdb -O1
CXXFLAGS= -c -O3 $(DBG) -std=c++11 -W -Wall -Wextra -Wpedantic -I/usr/local/include -I/usr/include -I/usr/local/linuxbrew/include
LDFLAGS=  $(DBG) -L/usr/local/lib #-ldl -pthread -lrt
OBJECTS= prover.o unifiers.o univar.o tau.o jsonld.o rdf.o misc.o json_object.o cli.o nquads.o


all: tau
tau: $(OBJECTS) $(EXECUTABLE)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp `${CXX} -std=c++11 $(CXXFLAGS) -M %.cpp`


test1: univar.txt.cpp
	clang++ $(ASAN) -std=c++11 -W -Wall -Wextra -Wpedantic -g -ggdb -O3   univar.txt.cpp


with_marpa: marpa_tau.o libmarpa/dist/.libs/libmarpa.so

libmarpa/dist/.libs/libmarpa.so:
	git submodule init
	git submodule update
	cd libmarpa;	make dist;	cd dist;	./configure;	make

with_marpa: OBJECTS += marpa_tau.o
with_marpa: CXXFLAGS += -Dwith_marpa  -I libmarpa/dist
with_marpa: LDFLAGS += -Llibmarpa/dist/.libs -lmarpa  -lboost_regex

debug: CXXFLAGS += -DDEBUG
release: CXXFLAGS -= -DDEBUG CXXFLAGS -= -ggdb CXXFLAGS += -O3 -NDEBUG
cl: CXXFLAGS += -DOPENCL
irc: CXXFLAGS += -DIRC -DDEBUG

with_marpa: $(OBJECTS) $(EXECUTABLE)
	$(CXX) $(OBJECTS) -o tau $(LDFLAGS)
debug: $(OBJECTS) $(EXECUTABLE)
	$(CXX) $(OBJECTS) -o tau $(LDFLAGS)
release: $(OBJECTS) $(EXECUTABLE)
	$(CXX) $(OBJECTS) -o tau $(LDFLAGS)
irc: $(OBJECTS) $(EXECUTABLE)
	$(CXX) $(OBJECTS) -o tau $(LDFLAGS)
cl: $(OBJECTS) $(EXECUTABLE)
	$(CXX) $(OBJECTS) -o tau $(LDFLAGS) -lOpenCL
ubi-tau: $(OBJECTS) ubi/client.o
	$(CXX) $(OBJECTS) ubi/client.o -o $@ $(LDFLAGS)
.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@
$(EXECUTABLE): $(OBJECTS) 
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

clean:
	rm -rf tau $(OBJECTS) ubi/client.o marpa.o marpa_tau.o

ppjson: ppjson.cpp
	$(CXX) -std=c++11 ppjson.cpp -oppjson -Wall -ggdb
dimacs2tau: dimacs2tau.cpp
	$(CXX) -std=c++11 dimacs2tau.cpp -odimacs2tau -Wall -ggdb
