CC=g++
OBJECTS= prover.o unifiers.o tau.o jsonld.o rdf.o misc.o json_object.o cli.o nquads.o
CFLAGS = -Wall -Wpedantic -Wextra -g -DDEBUG

all: jit
tau: $(OBJECTS) $(EXECUTABLE)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
.cpp.o:
	$(CC) $(CXXFLAGS) $< -o $@
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
clean:
	rm -rf jit jit.o n3.o ir.o eq.o
jit: jit.c dict.cpp ir.h ir.c n3.h n3.c eq.c eq.h defs.h
	gcc jit.c -ojit.o -c $(CFLAGS)
	gcc n3.c -on3.o -c $(CFLAGS)
	gcc ir.c -oir.o -c $(CFLAGS)
	gcc eq.c -oeq.o -c $(CFLAGS)
	g++ jit.o n3.o ir.o eq.o dict.cpp -ojit $(CFLAGS) 
