# NOTE(ww): I don't fully understand why I need the RPATH here
# but not in the capstone build.
override CFLAGS := $(CFLAGS) -Wl,-rpath,$(shell pwd)/xed/kits/xed-mishegos/lib
override CPPFLAGS := $(CPPFLAGS) -Ixed/kits/xed-mishegos/include
override LDFLAGS := $(LDFLAGS) -Lxed/kits/xed-mishegos/lib
override LDLIBS := $(LDLIBS) -lxed

.PHONY: all
all: xed.so

SO_SUFFIX ?= so
UNAME ?= $(shell uname)

# XED's hardened build (security level 2, the default) passes Linux-only
# linker flags (-z relro etc.) that ld64 rejects, so back off on macOS.
ifeq ($(UNAME), Darwin)
	XED_BUILD_FLAGS := --security-level=1
endif

xed/kits/xed-mishegos/lib/libxed.$(SO_SUFFIX):
	cd xed && \
		python3 ./mfile.py install --shared --install-dir=kits/xed-mishegos $(XED_BUILD_FLAGS) -j $(JOBS)

xed.so: xed.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) xed.o $(LDLIBS) -o $@
ifeq ($(UNAME), Darwin)
	# libxed.dylib's install name is its build-tree path ("obj/libxed.dylib"),
	# so rewrite the reference to resolve via the rpath embedded above.
	install_name_tool -change obj/libxed.dylib @rpath/libxed.dylib $@
endif

xed.o: xed/kits/xed-mishegos/lib/libxed.$(SO_SUFFIX) xed.c

.PHONY: clean
clean:
	cd xed && python3 ./mfile.py clean && rm -rf kits
	rm -rf *.o *.so
