CC = g++
#TODO remove -g
CFLAGS = -std=c++11 -O3 -ffast-math -w -Wall -Wno-deprecated -Wno-unused-variable

all: loss_estimator

loss_estimator: flow.o utils.o bayesian_net.o loss_estimator.cpp
	$(CC) $(INCLUDE) $(CFLAGS) flow.o utils.o bayesian_net.o  -o loss_estimator loss_estimator.cpp

flow.o: flow.cpp flow.h defs.h
	$(CC) $(CFLAGS) -c flow.cpp

utils.o: utils.cpp utils.h flow.o
	$(CC) $(CFLAGS) -c utils.cpp

bayesian_net.o: bayesian_net.cpp bayesian_net.h flow.o utils.o
	$(CC) $(CFLAGS) -c bayesian_net.cpp

clean:	
	rm -f *.o loss_estimator
