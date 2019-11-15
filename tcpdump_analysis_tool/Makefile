CXX=g++ -std=c++11
RM=rm -f

SRCS=utils.cpp main.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: main

main: $(OBJS)
	$(CXX) -o main $(OBJS)

main.o: main.cpp utils.h

utils.o: utils.cpp utils.h

clean:
	$(RM) $(OBJS) main
