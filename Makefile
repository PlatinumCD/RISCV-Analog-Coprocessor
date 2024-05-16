CC=g++
CFLAGS=-I.

all: prog

prog: prog.o
	$(CC) -o prog prog.o

prog.o: prog.cpp
	$(CC) $(CFLAGS) -c prog.cpp

docs:
	doxygen Doxyfile

clean:
	rm -rf *.o *.out* docs sst_env/std*-100
