CC = g++
CFLAGS = -std=c++17 -O3 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
#CFLAGS = -g -std=c++17 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
LIBDIR = -L../sparsehash/libs -L../
INCLUDE = -I../sparsehash/include -I../

LIBS = -lflock -ljsoncpp

all: collector

#!TODO: Fix dependencies
collector: collector.cpp
	$(MAKE) -C .. libflock.a
	$(CC) $(LIBDIR) $(INCLUDE) $(CFLAGS) collector.cpp -o collector $(LIBS)

clean:	
	rm -f *.o collector
