include Makefile.variable

all:
	cd utils && $(MAKE)
	cd algorithms && $(MAKE)
	cd analysis && $(MAKE)

clean:
	cd utils && $(MAKE) clean
	cd algorithms && $(MAKE) clean
	cd analysis && $(MAKE) clean
