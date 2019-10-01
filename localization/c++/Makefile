CC = g++

CFLAGS = -std=c++17 -O3 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable
#CFLAGS = -g -std=c++17 -fopenmp -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable

LIBS = -L/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/localization/c++/sparsehash/libs

INCLUDE = -I/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/localization/c++/sparsehash/include

all: loss_estimator loss_estimator_trend reduced_estimator

OBJS = flow.o utils.o logdata.o bayesian_net.o

reduced_estimator: $(OBJS) reduced_estimator.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) -o reduced_estimator reduced_estimator.cpp

loss_estimator: $(OBJS) loss_estimator.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) -o loss_estimator loss_estimator.cpp

loss_estimator_trend: $(OBJS) loss_estimator_trend.cpp
	$(CC) $(INCLUDE) $(CFLAGS) $(OBJS) -o loss_estimator_trend loss_estimator_trend.cpp

flow.o: flow.cpp flow.h defs.h
	$(CC) $(CFLAGS) -c flow.cpp

utils.o: utils.cpp utils.h flow.o
	$(CC) $(INCLUDE) $(CFLAGS) -c utils.cpp

logdata.o: logdata.cpp logdata.h utils.o flow.o
	$(CC) $(INCLUDE) $(CFLAGS) -c logdata.cpp

bayesian_net.o: bayesian_net.cpp bayesian_net.h flow.o utils.o logdata.o
	$(CC) $(INCLUDE) $(CFLAGS) -c bayesian_net.cpp $(LIBS) 

clean:	
	rm -f *.o loss_estimator reduced_estimator
