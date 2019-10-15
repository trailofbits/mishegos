# NOTE(ww): I don't fully understand why I need the RPATH here
# but not in the capstone build.
override CFLAGS := $(CFLAGS) -Wl,-rpath,$(shell pwd)/udis86/lib
override CPPFLAGS := $(CPPFLAGS) -Iudis86/include
override LDFLAGS := $(LDFLAGS) -Ludis86/lib
override LDLIBS := $(LDLIBS) -ludis86

.PHONY: all
all: udis86.so

.ONESHELL:
udis86/lib/libudis86.so:
	cd udis86
	./autogen.sh
	./configure --prefix=$(shell pwd)/udis86
	make -j 4
	make install

udis86.so: udis86.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) udis86.o $(LDLIBS) -o $@

udis86.o: udis86/lib/libudis86.so udis86.c
