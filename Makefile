export CFLAGS := \
	-std=gnu11 -Wall -pthread \
	-I$(shell pwd)/src/include
export LDLIBS := -ldl -lpthread
export CPPFLAGS :=
export CXXFLAGS := \
	-std=c++11 -Wall -pthread \
	-I$(shell pwd)/src/include
# TODO(ww): https://github.com/rust-lang/rust-bindgen/issues/1651
# export RUSTFLAGS := -D warnings
export RUST_BINDGEN_CLANG_ARGS := \
	-I$(shell pwd)/src/include

export UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	export SO_SUFFIX := dylib
else
	export SO_SUFFIX := so
endif


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
	$(MAKE) -C src/worker

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

