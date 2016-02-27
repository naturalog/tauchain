CXX=clang++
###NEW= -DNEW
ASAN=   -Xclang -fcolor-diagnostics -ferror-limit=10 -fsanitize=address -fsanitize=integer -fsanitize=unsigned-integer-overflow -fsanitize=undefined -ggdb -gline-tables-only # -fsanitize-undefined-trap-on-error 
DEBUG=  -DDEBUG -O0  -fno-omit-frame-pointer -fno-optimize-sibling-calls 
DBG= $(ASAN) -g -ggdb $(DEBUG)
CXXFLAGS= -c -O3 $(DBG) $(NEW) -std=c++1y -W -Wall -Wextra -Wpedantic -I/usr/local/include -I/usr/include -I/usr/local/linuxbrew/include 
LDFLAGS=  $(DBG) -L/usr/local/lib #-ldl -pthread -lrt
OBJECTS= prover.o unifiers.o univar.o tau.o jsonld.o rdf.o misc.o json_object.o jsonld_tau.o nquads.o


all: with_marpa

with_marpa: libmarpa/dist/.libs/libmarpa.so marpa_tau.o 

libmarpa/dist/.libs/libmarpa.so:
	git submodule init
	git submodule update
	cd libmarpa;	make dist;	cd dist;	./configure;	make

with_marpa: OBJECTS += marpa_tau.o
with_marpa: CXXFLAGS += -Dwith_marpa  -I libmarpa/dist #-DNOPARSER -DJSON
with_marpa: LDFLAGS += -Llibmarpa/dist/.libs -lmarpa  -lboost_regex


tau: $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
cppout: out.o prover.o unifiers.o jsonld.o rdf.o misc.o json_object.o jsonld_tau.o nquads.o

	$(CXX) out.o -o $@ $(LDFLAGS)
#tau-new: CXXFLAGS += -DNEW
#tau-new: $(OBJECTS) 
#	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp `${CXX} -std=c++11 $(CXXFLAGS) -M %.cpp`


how: stuff/how-lambdas-work.cpp
	clang++ $(ASAN) -std=c++11 -W -Wall -Wextra -Wpedantic -g -ggdb -O3   stuff/how-lambdas-work.cpp



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
	rm -rf tau $(OBJECTS) ubi/client.o marpa.o marpa_tau.o m-tau tau-new a.out 

ppjson: ppjson.cpp
	$(CXX) -std=c++11 ppjson.cpp -oppjson -Wall -ggdb
dimacs2tau: dimacs2tau.cpp
	$(CXX) -std=c++11 dimacs2tau.cpp -odimacs2tau -Wall -ggdb
