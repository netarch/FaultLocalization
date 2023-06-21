CC = g++-12
#CFLAGS = -std=c++17 -O3 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
CFLAGS = -g -std=c++17 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
LIBDIR = -Lsparsehash/libs
INCLUDE = -Isparsehash/include

ESTIMATOR_OBJS = bayesian_net.o bayesian_net_continuous.o doubleO7.o score.o net_bouncer.o bayesian_net_sherlock.o

ESTIMATOR_MAINS = black_hole_estimator black_hole_estimator_agg loss_estimator misconfigured_acl equivalence_class

OBJS = flow.o logdata.o utils.o black_hole_utils.o $(ESTIMATOR_OBJS)

all: $(ESTIMATOR_MAINS) loss_estimator_trend reduced_estimator

libflock.a: $(OBJS)
	ar crf libflock.a $(OBJS)

reduced_estimator: $(OBJS) reduced_estimator.cpp reduced_utils.o
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) reduced_utils.o -o reduced_estimator reduced_estimator.cpp

$(ESTIMATOR_MAINS): %: %.cpp $(OBJS)
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) -o $@ $<

loss_estimator_trend: $(OBJS) common_testing_system.o reduced_utils.o loss_estimator_trend.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) common_testing_system.o reduced_utils.o -o loss_estimator_trend loss_estimator_trend.cpp

flow.o: flow.cpp utils.cpp flow.h defs.h utils.h
	$(CC) $(INCLUDE) $(CFLAGS) -c flow.cpp

utils.o: utils.cpp utils.h flow.h defs.h
	$(CC) $(INCLUDE) $(CFLAGS) -c utils.cpp

logdata.o: logdata.cpp utils.cpp logdata.h utils.h flow.h defs.h
	$(CC) $(INCLUDE) $(CFLAGS) -c logdata.cpp

common_testing_system.o: common_testing_system.cpp common_testing_system.h utils.o flow.o logdata.o
	$(CC) $(INCLUDE) $(CFLAGS) -c common_testing_system.cpp

reduced_utils.o: reduced_utils.cpp reduced_utils.h bayesian_net.h flow.h defs.h utils.h utils.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c reduced_utils.cpp

black_hole_utils.o: black_hole_utils.cpp black_hole_utils.h flow.h defs.h utils.h utils.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c black_hole_utils.cpp

$(ESTIMATOR_OBJS): %.o: %.cpp %.h estimator.h flow.h utils.h logdata.h defs.h utils.h utils.cpp black_hole_utils.h black_hole_utils.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c $< $(LIBDIR) -o $@ 

clean:	
	rm -f *.o reduced_estimator loss_estimator_trend libflock.a $(ESTIMATOR_MAINS)
