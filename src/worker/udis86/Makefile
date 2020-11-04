# NOTE(ww): I don't fully understand why I need the RPATH here
# but not in the capstone build.
override CFLAGS := $(CFLAGS) -Wl,-rpath,$(shell pwd)/udis86/build/lib
override CPPFLAGS := $(CPPFLAGS) -Iudis86/build/include
override LDFLAGS := $(LDFLAGS) -Ludis86/build/lib
override LDLIBS := $(LDLIBS) -ludis86

.PHONY: all
all: udis86.so

.ONESHELL:
udis86/build/lib/libudis86.so:
	cd udis86
	./autogen.sh
	./configure --prefix=$(shell pwd)/udis86/build/
	make -j 4
	make install

udis86.so: udis86.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) udis86.o $(LDLIBS) -o $@

udis86.o: udis86/build/lib/libudis86.so udis86.c

.PHONY: clean
clean:
	make -C udis86 clean
	rm -rf *.o *.so
