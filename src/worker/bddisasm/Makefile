override CPPFLAGS := $(CPPFLAGS) -Ibddisasm/inc
override LDFLAGS := $(LDFLAGS) -Lbddisasm/build
override LDLIBS := $(LDLIBS) -lbddisasm

.PHONY: all
all: bddisasm.so

bddisasm/build/bddisasm.a:
	cmake -B bddisasm/build -S bddisasm -DCMAKE_BUILD_TYPE=Release -DBDD_INCLUDE_TOOL=OFF -DBDD_INCLUDE_ISAGENERATOR=OFF
	cmake --build bddisasm/build --target bddisasm --parallel $(JOBS)

bddisasm.so: bddisasm/build/bddisasm.a bddisasm.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) bddisasm.o $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf bddisasm/build
	rm -rf *.o *.so
