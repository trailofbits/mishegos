UNAME ?= $(shell uname)

LLVM_CONFIG=llvm-config
ifeq ($(UNAME), Darwin)
	# Homebrew's LLVM is keg-only and not on PATH by default.
	LLVM_CONFIG := $(shell command -v llvm-config || echo $(shell brew --prefix llvm 2>/dev/null)/bin/llvm-config)
	Z_DEFS :=
else
	Z_DEFS := -Wl,-z,defs
endif
override CPPFLAGS := $(CPPFLAGS) $(shell $(LLVM_CONFIG) --cppflags)
override LDFLAGS := $(LDFLAGS) $(shell $(LLVM_CONFIG) --ldflags) $(Z_DEFS)
override LDLIBS := $(LDLIBS) $(shell $(LLVM_CONFIG) --libs)

.PHONY: all
all: llvm.so

llvm.so: llvm.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf *.o *.so

