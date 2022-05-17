UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	# I can't even begin to describe how annoying this is:
	# 1. libbfd's headers error out if some defines provided by a config.h are
	#    missing. But config.h is also missing, because libbfd is considered
	#    an "internal" library by the GNU binutils maintainers.
	#    So we stub those in.
	# 2. By default, binutils isn't packaged with libiberty on macOS.
	#    It has to be built manually with `--enable-install-libiberty`, which
	#    then needs to be manually linked in. The code below assumes the manual
	#    build was done with Homebrew.
	SNEAKY_MAKE_BFD_INCLUDES_WORK_DEFINES := -DPACKAGE=nice-try-bfd-maintainers -DPACKAGE_VERSION=1
	override CPPFLAGS := $(CPPFLAGS) \
		$(SNEAKY_MAKE_BFD_INCLUDES_WORK_DEFINES) \
		-I/usr/local/opt/binutils/include
	override LDFLAGS := $(LDFLAGS) -L/usr/local/opt/binutils/lib -liberty -lz
endif

override LDLIBS := $(LDLIBS) -lbfd -lopcodes

.PHONY: all
all: bfd.so

bfd.so: bfd.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf *.o *.so
