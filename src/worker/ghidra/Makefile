override LDFLAGS :=
override CXXFLAGS :=

.PHONY: all
all: ghidra.so

ghidra.so:
	cmake -B build -S . \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
			-Dsleigh_RELEASE_TYPE=HEAD \
			"-DFETCHCONTENT_SOURCE_DIR_GHIDRASOURCE=./ghidra" && \
		cmake --build build --parallel $(JOBS) --verbose && \
		cp build/ghidra.$(SO_SUFFIX) ./ghidra.so

# Uncomment to build with address sanitizer. Remember to remove 'build'
# directory if switching between the two
#.PHONY: ghidra.so
#.ONESHELL:
#ghidra.so:
#	cmake -B build -S . \
#		-DCMAKE_BUILD_TYPE=Debug \
#		-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
#		-Dsleigh_RELEASE_TYPE=HEAD \
#		"-DCMAKE_C_FLAGS=-Wall -Wpedantic -Wextra -fsanitize=address" \
#		"-DCMAKE_CXX_FLAGS=-Wall -Wpedantic -Wextra -Wconversion -Wsign-conversion -Wcast-qual -Wshadow -Wformat=2 -Wundef -fsanitize=address" \
#		"-DCMAKE_MODULE_LINKER_FLAGS=-fsanitize=address" \
#		"-DFETCHCONTENT_SOURCE_DIR_GHIDRASOURCE=./ghidra"
#	cmake --build build -j --verbose
#	cp build/ghidra.so ./ghidra.so

.PHONY: clean
clean:
	rm -rf build
	rm -f ./ghidra.so
