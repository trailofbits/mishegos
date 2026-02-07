UNAME := $(shell uname)

CFLAGS := \
	-std=gnu11 -Wall -pthread -O2 \
	-I$(shell pwd)/src/include
LDLIBS := -ldl -lpthread
CPPFLAGS :=
CXXFLAGS := \
	-std=c++11 -Wall -pthread -O2 \
	-I$(shell pwd)/src/include
# TODO(ww): https://github.com/rust-lang/rust-bindgen/issues/1651
# RUSTFLAGS := -D warnings
RUST_BINDGEN_CLANG_ARGS := \
	-I$(shell pwd)/src/include

ifeq ($(UNAME), Darwin)
	SO_SUFFIX := dylib
else
	SO_SUFFIX := so
 	# Linux needs -lrt for the POSIX shm(3) family calls.
	LDLIBS := $(LDLIBS) -lrt
endif

JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

export UNAME
export CFLAGS
export LDLIBS
export CPPFLAGS
export CXXFLAGS
export RUST_BINDGEN_CLANG_ARGS
export SO_SUFFIX
export JOBS
export CARGO_BUILD_JOBS = $(JOBS)


ALL_SRCS := $(shell \
	find . -type f \
	\( \
		-path '*/capstone/capstone/*' -o \
		-path '*/vendor/*' -o \
		-path '*/dynamorio/dynamorio/*' -o \
		-path '*/dynamorio/obj/*' -o \
		-path '*/fadec/fadec/*' -o \
		-path '*/udis86/udis86/*' -o \
		-path '*/xed/xed/*' -o \
		-path '*/xed/mbuild/*' -o \
		-path '*/zydis/zydis/*' -o \
		-path '*/bddisasm/bddisasm/*' -o \
		-path '*/ghidra/sleighMishegos*' -o \
		-path '*/ghidra/ghidra/*' -o \
		-path '*/ghidra/build/*' -o \
		-path '*/ghidra/sleigh-cmake/*' \
	\) \
	-prune \
	-o \( \
		-name 'sleighMishegos*' -o \
		-name '*.c' -o \
		-name '*.cc' -o \
		-name '*.h' -o \
		-name '*.hh' \
	\) \
	-print \
)

.PHONY: all
all: mishegos worker mish2jsonl

.PHONY: debug
debug: CPPFLAGS += -DDEBUG
debug: CFLAGS += -g
debug: all

.PHONY: mishegos
mishegos:
	$(MAKE) -C src/mishegos

.PHONY: worker
worker:
	$(MAKE) -C src/worker $(WORKERS)

.PHONY: mish2jsonl
mish2jsonl:
	$(MAKE) -C  src/mish2jsonl

.PHONY: fmt
fmt:
	clang-format -i -style=file $(ALL_SRCS)

.PHONY: lint
lint:
	cppcheck --error-exitcode=1 $(ALL_SRCS)

.PHONY: edit
edit:
	$(EDITOR) $(ALL_SRCS)

.PHONY: clean
clean:
	$(MAKE) -C src/worker clean
	$(MAKE) -C src/mishegos clean
	$(MAKE) -C src/mish2jsonl clean

.PHONY: update-submodules
update-submodules:
	git submodule foreach git pull origin master

