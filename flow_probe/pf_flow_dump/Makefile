all:
	cd kernel; make
	cd userland; ./configure; make

install:
	cd userland; make install

clean:
	-cd kernel; make clean
	-cd userland; make clean

