override CPPFLAGS := $(CPPFLAGS) -Icapstone/include
override LDFLAGS := $(LDFLAGS) -L./capstone
override LDLIBS := $(LDLIBS) -lcapstone

.PHONY: all
all: capstone.so

# This is some stupidity thanks to Capstone's misbehaving Makefile.
MAKEOVERRIDES := $(filter-out CFLAGS=%,$(MAKEOVERRIDES))
MAKEOVERRIDES := $(filter-out CPPFLAGS=%,$(MAKEOVERRIDES))
MAKEOVERRIDES := $(filter-out LDFLAGS=%,$(MAKEOVERRIDES))
MAKEOVERRIDES := $(filter-out LDLIBS=%,$(MAKEOVERRIDES))
capstone/libcapstone.so.5:
	$(MAKE) -C capstone/ \
		CAPSTONE_ARCHS="x86" CAPSTONE_X86_ATT_DISABLE=yes V=1

capstone.so: capstone/libcapstone.so.5 capstone.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) capstone.o $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf *.o *.so
	$(MAKE) -C capstone/ clean
