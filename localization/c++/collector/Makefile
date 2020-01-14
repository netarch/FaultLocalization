CC = g++
#TODO Add DNDEBUG flag -DNDEBUG 
CFLAGS = -std=c++17 -O3 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
#CFLAGS = -g -std=c++17 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
LIBDIR = -L../sparsehash/libs -L../
INCLUDE = -I../sparsehash/include -I../

LIBS = -lflock #-ljsoncpp

all: collector collector_ipfix

.PHONY: libflock.a

libflock.a:
	$(MAKE) -C .. libflock.a

#!TODO: Fix dependencies
collector: collector.cpp libflock.a
	$(CC) $(LIBDIR) $(INCLUDE) $(CFLAGS) collector.cpp -o collector $(LIBS)

#!TODO: Fix dependencies
collector_ipfix: collector_ipfix.cpp thread_pool.h flow_parser.o collector_utils.o libflock.a
	$(CC) $(LIBDIR) $(INCLUDE) $(CFLAGS) flow_parser.o collector_utils.o -o collector_ipfix $(LIBS) collector_ipfix.cpp

flow_parser.o: flow_parser.cpp flow_parser.h libflock.a
	$(CC) $(LIBDIR) $(INCLUDE) $(CFLAGS) -c flow_parser.cpp $(LIBS)

collector_utils.o: collector_utils.h collector_utils.cpp flow_parser.h libflock.a
	$(CC) $(LIBDIR) $(INCLUDE) $(CFLAGS) -c collector_utils.cpp $(LIBS)

clean:	
	$(MAKE) -C .. clean
	rm -f ../libflock.a *.o collector collector_ipfix
