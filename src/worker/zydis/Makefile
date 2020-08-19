# Include stupidity.
override CPPFLAGS := $(CPPFLAGS) \
	-Izydis/include \
	-Izydis/dependencies/zycore/include \
	-Izydis/build \
	-Izydis/build/zycore

.PHONY: all
all: zydis.so

.ONESHELL:
zydis/build/libZydis.a:
	cd zydis
	mkdir build
	cd build
	cmake ..
	cmake --build . -- -j4

zydis.so: zydis.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) \
	-Wl,--whole-archive zydis/build/libZydis.a -Wl,--no-whole-archive \
	zydis.o $(LDLIBS) -o $@

zydis.o: zydis/build/libZydis.a zydis.c

.PHONY: clean
clean:
	rm -rf *.o *.so
	rm -rf zydis/build
