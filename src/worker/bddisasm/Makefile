override CPPFLAGS := $(CPPFLAGS) -Ibddisasm/inc
override LDFLAGS := $(LDFLAGS) -Lbddisasm/bin/x64/Release
override LDLIBS := $(LDLIBS) -lbddisasm

.PHONY: all
all: bddisasm.so

bddisasm/bin/x64/Release/bddisasm.a:
	$(MAKE) -C bddisasm bddisasm

bddisasm.so: bddisasm/bin/x64/Release/bddisasm.a bddisasm.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) bddisasm.o $(LDLIBS) -o $@

.PHONY: clean
clean:
	$(MAKE) -C bddisasm clean
	rm -rf *.o *.so
