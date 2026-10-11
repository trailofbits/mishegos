override CPPFLAGS := $(CPPFLAGS) -Icapstone/include
override LDFLAGS := $(LDFLAGS) -L./capstone
override LDLIBS := $(LDLIBS) -lcapstone

.PHONY: all
all: capstone.so

# Use the static archive as the build stamp: unlike the shared library,
# its name doesn't embed a version or vary between platforms.
LIBCAPSTONE := capstone/libcapstone.a

# Let Capstone extend flags from the environment instead of command-line overrides.
# Filtering MAKEOVERRIDES splits escaped spaces in multi-word flag values.
MAKEOVERRIDES :=
$(LIBCAPSTONE):
	$(MAKE) -C capstone/ \
		CAPSTONE_ARCHS="x86" CAPSTONE_X86_ATT_DISABLE=yes CAPSTONE_BUILD_CORE_ONLY=yes V=1

capstone.so: $(LIBCAPSTONE) capstone.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) capstone.o $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf *.o *.so
	$(MAKE) -C capstone/ clean
