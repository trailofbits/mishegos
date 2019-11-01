override CPPFLAGS := $(CPPFLAGS) -DLINUX -DX86_64 -DDR_FAST_IR -Iobj/include
override LDFLAGS := $(LDFLAGS) -Lobj/lib64
override LDLIBS := $(LDLIBS) -ldrdecode

.PHONY: all
all: dynamorio.so

dynamorio.so: dynamorio.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) dynamorio.o $(LDLIBS) -o $@

.ONESHELL:
obj/lib64/libdrdecode.a:
	mkdir obj
	cd obj
	cmake ../dynamorio
	make

dynamorio.o: dynamorio.c obj/lib64/libdrdecode.a

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf obj
