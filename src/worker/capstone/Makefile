override CPPFLAGS := $(CPPFLAGS) -Icapstone/include
override LDFLAGS := $(LDFLAGS) -L./capstone
override LDLIBS := $(LDLIBS) -lcapstone

.PHONY: all
all: capstone.so

# Use the static archive as the build stamp: unlike the shared library,
# its name doesn't embed a version or vary between platforms.
LIBCAPSTONE := capstone/libcapstone.a

# This is some stupidity thanks to Capstone's misbehaving Makefile.
MAKEOVERRIDES := $(filter-out CFLAGS=%,$(MAKEOVERRIDES))
MAKEOVERRIDES := $(filter-out CPPFLAGS=%,$(MAKEOVERRIDES))
MAKEOVERRIDES := $(filter-out LDFLAGS=%,$(MAKEOVERRIDES))
MAKEOVERRIDES := $(filter-out LDLIBS=%,$(MAKEOVERRIDES))
$(LIBCAPSTONE):
	$(MAKE) -C capstone/ \
		CAPSTONE_ARCHS="x86" CAPSTONE_X86_ATT_DISABLE=yes CAPSTONE_BUILD_CORE_ONLY=yes V=1

capstone.so: $(LIBCAPSTONE) capstone.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) capstone.o $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf *.o *.so
	$(MAKE) -C capstone/ clean
