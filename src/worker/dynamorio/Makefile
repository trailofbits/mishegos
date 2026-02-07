override CPPFLAGS := $(CPPFLAGS) -DLINUX -DX86_64 -DDR_FAST_IR -Iobj/include
override LDFLAGS := $(LDFLAGS) -Lobj/lib64
override LDLIBS := $(LDLIBS) -ldrdecode -ldrlibc

DYNAMORIO_BUILD_OPTS := -DBUILD_DRSTATS=NO \
	-DBUILD_SAMPLES=NO \
	-DBUILD_EXT=NO \
	-DBUILD_CLIENTS=NO

.PHONY: all
all: dynamorio.so

dynamorio.so: dynamorio.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) dynamorio.o $(LDLIBS) -o $@

obj/lib64/libdrdecode.a:
	mkdir -p obj && \
		cd obj && \
		env -u CFLAGS -u CXXFLAGS \
			cmake $(DYNAMORIO_BUILD_OPTS) ../dynamorio && \
		cmake --build . --parallel $(JOBS)

dynamorio.o: dynamorio.c obj/lib64/libdrdecode.a

.PHONY: clean
clean:
	rm -rf *.so
	rm -rf *.o
	rm -rf obj
