export CFLAGS := \
	-std=gnu11 -Wall -Werror -pthread \
	-I$(shell pwd)/src/include \
	-I$(shell pwd)/src/vendor/include
export LDLIBS := -ldl -lrt -lpthread
export CPPFLAGS :=

ALL_SRCS := $(shell \
	find . -type f \
	\( \
		-path '*/capstone/capstone/*' -o \
		-path '*/vendor/*' -o \
		-path '*/xed/xed/*' -o \
		-path '*/xed/mbuild/*' \
	\) \
	-prune -o \
	\( -name '*.c' -o -name '*.h' \) -print \
)

.PHONY: all
all: mishegos worker

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

.PHONY: fmt
fmt:
	clang-format -i -style=file $(ALL_SRCS)

.PHONY: edit
edit:
	$(EDITOR) $(ALL_SRCS)

.PHONY: clean
clean:
	$(MAKE) -C src/worker clean
	$(MAKE) -C src/mishegos clean
