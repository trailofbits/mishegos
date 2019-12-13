override CPPFLAGS := $(CPPFLAGS) -Ifadec -Ifadec/build
override LDFLAGS := $(LDFLAGS) -Lfadec/build
override LDLIBS := $(LDLIBS) -lfadec

.PHONY: all
all: fadec.so

fadec/build/libfadec.a:
	mkdir -p fadec/build
	cd fadec; env -u CPPFLAGS -u LDFLAGS -u LDLIBS meson build -Dbuildtype=release -Darchmode=only64
	ninja -C fadec/build -v

fadec.so: fadec.c fadec/build/libfadec.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf fadec/build
	rm -rf *.o *.so

