export CFLAGS := \
	-std=gnu11 -Wall -Werror -pthread \
	-I$(shell pwd)/src/include
export LDLIBS := -ldl -lrt -lpthread
export CPPFLAGS :=
export CXXFLAGS := \
	-std=c++11 -Wall -Werror -pthread \
	-I$(shell pwd)/src/include

ALL_SRCS := $(shell \
	find . -type f \
	\( \
		-path '*/capstone/capstone/*' -o \
		-path '*/vendor/*' -o \
		-path '*/dynamorio/dynamorio/*' -o \
		-path '*/fadec/fadec/*' -o \
		-path '*/udis86/udis86/*' -o \
		-path '*/xed/xed/*' -o \
		-path '*/xed/mbuild/*' -o \
		-path '*/zydis/zydis/*' \
	\) \
	-prune \
	-o \( -name '*.c' -o -name '*.h' \) -print \
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
	$(MAKE) -C src/mish2jsonl

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

.PHONY: update-submodules
update-submodules:
	git submodule foreach git pull origin master

