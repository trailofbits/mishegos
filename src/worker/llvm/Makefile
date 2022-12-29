LLVM_CONFIG=llvm-config
override CPPFLAGS := $(CPPFLAGS) $(shell $(LLVM_CONFIG) --cppflags)
override LDFLAGS := $(LDFLAGS) $(shell $(LLVM_CONFIG) --ldflags) -Wl,-z,defs
override LDLIBS := $(LDLIBS) $(shell $(LLVM_CONFIG) --libs)

.PHONY: all
all: llvm.so

llvm.so: llvm.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf *.o *.so

