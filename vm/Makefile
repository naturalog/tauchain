CC=g++
OBJECTS= prover.o unifiers.o tau.o jsonld.o rdf.o misc.o json_object.o cli.o nquads.o
CFLAGS = -Wall -Wpedantic -Wextra -g -DDEBUG

all: vm
tau: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
.cpp.o:
	$(CC) $(CXXFLAGS) $< -o $@
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
clean:
	rm -rf vm vm.o n3.o ir.o cfpds.o
vm: vm.c dict.cpp vm.h n3.c cfpds.c cfpds.h
	gcc vm.c -ovm.o -c $(CFLAGS)
	gcc n3.c -on3.o -c $(CFLAGS)
	gcc ir.c -oir.o -c $(CFLAGS)
	gcc cfpds.c -ocfpds.o -c $(CFLAGS)
	g++ vm.o n3.o ir.o cfpds.o dict.cpp -ovm $(CFLAGS) 
