include ../Makefile.variable

ROOTDIR=$(shell pwd)/..
VPATH = ${ROOTDIR}/algorithms ${ROOTDIR}/utils
LIBDIR = -L../sparsehash/libs -L../utils/ -L../algorithms/
INCLUDE = -I../sparsehash/include -I../utils/ -I../algorithms/
LIBS = -lflock -lflockalg


ESTIMATOR_MAINS = loss_estimator misconfigured_acl equivalence_class

all: $(ESTIMATOR_MAINS) reproduce_results reduced_estimator

reduced_estimator: reduced_estimator.cpp reduced_utils.o
	$(CC) $(INCLUDE) $(CFLAGS) reduced_utils.o -o reduced_estimator reduced_estimator.cpp ${LIBDIR} ${LIBS}

$(ESTIMATOR_MAINS): %: %.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $< ${LIBDIR} ${LIBS}

reproduce_results: common_testing_system.o reduced_utils.o reproduce_results.cpp
	$(CC) $(INCLUDE) $(CFLAGS) common_testing_system.o reduced_utils.o -o reproduce_results reproduce_results.cpp ${LIBDIR} ${LIBS}

common_testing_system.o: common_testing_system.cpp common_testing_system.h utils.o flow.o logdata.o
	$(CC) $(INCLUDE) $(CFLAGS) -c common_testing_system.cpp 

reduced_utils.o: reduced_utils.cpp reduced_utils.h bayesian_net.h flow.h defs.h utils.h utils.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c reduced_utils.cpp

clean:	
	rm -f *.o reduced_estimator reproduce_results $(ESTIMATOR_MAINS)
