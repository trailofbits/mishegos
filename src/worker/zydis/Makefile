# Include stupidity.
override CPPFLAGS := $(CPPFLAGS) \
	-DZYDIS_STATIC_BUILD \
	-Izydis/include \
	-Izydis/dependencies/zycore/include \
	-Izydis/build \
	-Izydis/build/zycore

.PHONY: all
all: zydis.so

zydis/build/libZydis.a:
	cd zydis && \
		mkdir build && \
		cd build && \
		cmake -DZYDIS_BUILD_TOOLS=OFF -DZYDIS_BUILD_EXAMPLES=OFF -DZYDIS_STATIC_DEFINE=ON .. && \
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
