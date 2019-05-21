override LDLIBS := $(LDLIBS) -lbfd -lopcodes

.PHONY: all
all: bfd.so

bfd.so: bfd.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf *.o *.so
