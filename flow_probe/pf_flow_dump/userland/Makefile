all: libpfring pcap build_examples

###################

lib/Makefile:
	./configure

config: lib/Makefile

libpfring: config
	cd lib; make

build_nbpf: config
	cd nbpf; make

libpcap/Makefile:
	cd libpcap; ./configure --enable-ipv6 --enable-dbus=no --without-libnl --with-snf=no --disable-bluetooth --disable-canusb --with-dag=no

pcap: libpfring libpcap/Makefile
	cd libpcap; make

build_examples: config
	cd examples; make

tcpdump/Makefile:
	cd tcpdump; ./configure

build_tcpdump: libpfring pcap tcpdump/Makefile
	cd tcpdump; make

###################

clean: libpfring_clean pcap_clean examples_clean tcpdump_clean nbpf_clean

libpfring_clean:
	cd lib; make clean

pcap_clean:
	if test -f libpcap/config.status; then cd libpcap; make clean; fi

examples_clean:
	cd examples; make clean

tcpdump_clean:
	if test -f tcpdump/config.status; then cd tcpdump; make clean; fi

nbpf_clean:
	cd nbpf; make clean

install: libpfring pcap examples
	cd lib; make install
	cd libpcap; make install
	cd examples; make install
