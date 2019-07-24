CC = g++
CFLAGS = -std=c++11 -O3 -w -Wall -Wno-deprecated -Wno-unused-variable

all: loss_estimator

loss_estimator: flow.o utils.o loss_estimator.cpp
	$(CC) $(INCLUDE) $(CFLAGS) flow.o utils.o  -o loss_estimator loss_estimator.cpp

flow.o: flow.cpp flow.h
	$(CC) $(CFLAGS) -c flow.cpp

utils.o: utils.cpp utils.h
	$(CC) $(CFLAGS) -c utils.cpp

clean:	
	rm *.o loss_estimator
