override CPPFLAGS := $(CPPFLAGS) -DLINUX -DX86_64 -DDR_FAST_IR -Iobj/include
override LDFLAGS := $(LDFLAGS) -Lobj/lib64
override LDLIBS := $(LDLIBS) -ldrdecode

DYNAMORIO_BUILD_OPTS := \
	-DBUILD_DRSTATS=NO \
	-DBUILD_TOOLS=NO \
	-DBUILD_SAMPLES=NO \
	-DBUILD_EXT=NO \
	-DBUILD_CLIENTS=NO

.PHONY: all
all: dynamorio.so

dynamorio.so: dynamorio.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) dynamorio.o $(LDLIBS) -o $@

.ONESHELL:
obj/lib64/libdrdecode.a:
	mkdir obj
	cd obj
	cmake $(DYNAMORIO_BUILD_OPTS) ../dynamorio
	make

dynamorio.o: dynamorio.c obj/lib64/libdrdecode.a

.PHONY: clean
clean:
	rm -rf *.so
	rm -rf *.o
	rm -rf obj
