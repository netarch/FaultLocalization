all: dummydump

dummydump: dummydump.o ${LIBPFRING}
	gcc -o dummydump dummydump.c -lpthread

clean:
	rm dummydump.o dummydump
