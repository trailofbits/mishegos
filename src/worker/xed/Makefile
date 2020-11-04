# NOTE(ww): I don't fully understand why I need the RPATH here
# but not in the capstone build.
override CFLAGS := $(CFLAGS) -Wl,-rpath,$(shell pwd)/xed/kits/xed-mishegos/lib
override CPPFLAGS := $(CPPFLAGS) -Ixed/kits/xed-mishegos/include
override LDFLAGS := $(LDFLAGS) -Lxed/kits/xed-mishegos/lib
override LDLIBS := $(LDLIBS) -lxed

.PHONY: all
all: xed.so

.ONESHELL:
xed/kits/xed-mishegos/libxed.so:
	cd xed
	python3 ./mfile.py install --shared --install-dir=kits/xed-mishegos -j 4

xed.so: xed.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) xed.o $(LDLIBS) -o $@

xed.o: xed/kits/xed-mishegos/libxed.so xed.c

.PHONY: clean
clean:
	cd xed && python3 ./mfile.py clean && rm -rf kits
	rm -rf *.o *.so
