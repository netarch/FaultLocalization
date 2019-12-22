CC = g++
#CFLAGS = -std=c++17 -O3 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
CFLAGS = -g -std=c++17 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
LIBDIR = -Lsparsehash/libs
INCLUDE = -Isparsehash/include

ESTIMATOR_OBJS = bayesian_net.o bayesian_net_continuous.o doubleO7.o score.o net_bouncer.o

OBJS = flow.o logdata.o utils.o $(ESTIMATOR_OBJS)

all: loss_estimator loss_estimator_trend reduced_estimator

libflock.a: $(OBJS)
	ar crf libflock.a $(OBJS)

reduced_estimator: $(OBJS) reduced_estimator.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) -o reduced_estimator reduced_estimator.cpp

loss_estimator: $(OBJS) loss_estimator.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) -o loss_estimator loss_estimator.cpp

loss_estimator_trend: $(OBJS) common_testing_system.o loss_estimator_trend.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) common_testing_system.o -o loss_estimator_trend loss_estimator_trend.cpp

flow.o: flow.cpp flow.h defs.h
	$(CC) $(INCLUDE) $(CFLAGS) -c flow.cpp

utils.o: utils.cpp utils.h flow.h defs.h
	$(CC) $(INCLUDE) $(CFLAGS) -c utils.cpp

logdata.o: logdata.cpp logdata.h utils.h flow.h defs.h
	$(CC) $(INCLUDE) $(CFLAGS) -c logdata.cpp

common_testing_system.o: common_testing_system.cpp common_testing_system.h utils.o flow.o logdata.o
	$(CC) $(INCLUDE) $(CFLAGS) -c common_testing_system.cpp

$(ESTIMATOR_OBJS): %.o: %.cpp %.h estimator.h flow.h utils.h logdata.h defs.h
	$(CC) $(INCLUDE) $(CFLAGS) -c $< $(LIBDIR) -o $@ 

clean:	
	rm -f *.o loss_estimator reduced_estimator loss_estimator_trend libflock.a
