CC = g++
CFLAGS = -std=c++17 -O3 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
#CFLAGS = -g -std=c++17 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
LIBDIR = -L../sparsehash/libs -L../
INCLUDE = -I../sparsehash/include -I../

LIBS = -lflock -ljsoncpp

all: collector collector_ipfix

#!TODO: Fix dependencies
collector: collector.cpp
	$(MAKE) -C .. libflock.a
	$(CC) $(LIBDIR) $(INCLUDE) $(CFLAGS) collector.cpp -o collector $(LIBS)

#!TODO: Fix dependencies
collector_ipfix: collector_ipfix.cpp
	$(MAKE) -C .. libflock.a
	$(CC) $(LIBDIR) $(INCLUDE) $(CFLAGS) collector_ipfix.cpp -o collector_ipfix $(LIBS)

clean:	
	$(MAKE) -C .. clean
	rm -f ../libflock.a *.o collector collector_ipfix
