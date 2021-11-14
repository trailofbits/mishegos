override LDFLAGS :=
override CXXFLAGS :=

.PHONY: all
all: ghidra.so

.PHONY: ghidra.so
.ONESHELL:
ghidra.so:
	cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DSLEIGH_GHIDRA_RELEASE_TYPE=HEAD -DFETCHCONTENT_SOURCE_DIR_GHIDRASOURCE="./ghidra"
	cmake --build build -j --verbose
	cp build/ghidra.so ./ghidra.so

.PHONY: clean
clean:
	rm -rf build
	rm ./ghidra.so
