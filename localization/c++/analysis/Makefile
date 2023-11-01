include ../Makefile.variable

ROOTDIR=$(shell pwd)/..
VPATH = ${ROOTDIR}/algorithms ${ROOTDIR}/utils
LIBDIR = -L../sparsehash/libs -L../utils/ -L../algorithms/
INCLUDE = -I../sparsehash/include -I../utils/ -I../algorithms/
LIBS = -lflock -lflockalg


ESTIMATOR_MAINS = black_hole_estimator black_hole_estimator_agg loss_estimator misconfigured_acl equivalence_class

OBJS = black_hole_utils.o

all: $(ESTIMATOR_MAINS) reproduce_results reduced_estimator

reduced_estimator: $(OBJS) reduced_estimator.cpp reduced_utils.o
	$(CC) $(INCLUDE) $(CFLAGS) ${OBJS} reduced_utils.o -o reduced_estimator reduced_estimator.cpp ${LIBDIR} ${LIBS}

$(ESTIMATOR_MAINS): %: %.cpp $(OBJS)
	$(CC) $(INCLUDE) $(CFLAGS) ${OBJS} -o $@ $< ${LIBDIR} ${LIBS}

reproduce_results: $(OBJS) common_testing_system.o reduced_utils.o reproduce_results.cpp
	$(CC) $(INCLUDE) $(CFLAGS) ${OBJS} common_testing_system.o reduced_utils.o -o reproduce_results reproduce_results.cpp ${LIBDIR} ${LIBS}

common_testing_system.o: common_testing_system.cpp common_testing_system.h utils.o flow.o logdata.o
	$(CC) $(INCLUDE) $(CFLAGS) -c common_testing_system.cpp 

reduced_utils.o: reduced_utils.cpp reduced_utils.h bayesian_net.h flow.h defs.h utils.h utils.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c reduced_utils.cpp

black_hole_utils.o: black_hole_utils.cpp black_hole_utils.h flow.h defs.h utils.h utils.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c black_hole_utils.cpp

clean:	
	rm -f *.o reduced_estimator reproduce_results $(ESTIMATOR_MAINS)
