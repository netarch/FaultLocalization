CC = g++
CFLAGS = -std=c++17 -O3 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
#CFLAGS = -g -std=c++17 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
LIBS = -Lsparsehash/libs
INCLUDE = -Isparsehash/include

OBJS = flow.o logdata.o bayesian_net_continuous.o bayesian_net.o doubleO7.o score.o utils.o

all: loss_estimator loss_estimator_trend reduced_estimator

reduced_estimator: $(OBJS) reduced_estimator.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) -o reduced_estimator reduced_estimator.cpp

loss_estimator: $(OBJS) loss_estimator.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) -o loss_estimator loss_estimator.cpp

loss_estimator_trend: $(OBJS) common_testing_system.o loss_estimator_trend.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) common_testing_system.o -o loss_estimator_trend loss_estimator_trend.cpp

flow.o: flow.cpp flow.h defs.h
	$(CC) $(INCLUDE) $(CFLAGS) -c flow.cpp

utils.o: utils.cpp utils.h flow.o
	$(CC) $(INCLUDE) $(CFLAGS) -c utils.cpp

logdata.o: logdata.cpp logdata.h utils.o flow.o
	$(CC) $(INCLUDE) $(CFLAGS) -c logdata.cpp

common_testing_system.o: common_testing_system.cpp common_testing_system.h utils.o flow.o logdata.o
	$(CC) $(INCLUDE) $(CFLAGS) -c common_testing_system.cpp

doubleO7.o: doubleO7.cpp doubleO7.h estimator.h flow.o utils.o logdata.o
	$(CC) $(INCLUDE) $(CFLAGS) -c doubleO7.cpp $(LIBS) 

score.o: score.cpp score.h estimator.h flow.o utils.o logdata.o
	$(CC) $(INCLUDE) $(CFLAGS) -c score.cpp $(LIBS) 

bayesian_net.o: bayesian_net.cpp bayesian_net.h estimator.h flow.o utils.o logdata.o
	$(CC) $(INCLUDE) $(CFLAGS) -c bayesian_net.cpp $(LIBS) 

bayesian_net_continuous.o: bayesian_net_continuous.cpp bayesian_net_continuous.h estimator.h flow.o utils.o logdata.o
	$(CC) $(INCLUDE) $(CFLAGS) -c bayesian_net_continuous.cpp $(LIBS) 
clean:	
	rm -f *.o loss_estimator reduced_estimator loss_estimator_trend
