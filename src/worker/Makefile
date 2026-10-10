UNAME ?= $(shell uname)

WORKERS = bfd \
	capstone \
	dynamorio \
	fadec \
	xed \
	zydis \
	bddisasm \
	iced \
	yaxpeax-x86 \
	ghidra \
	llvm

# DynamoRIO's build hardcodes Linux and its decode library
# doesn't support macOS hosts, so skip it there by default.
ifeq ($(UNAME), Darwin)
	WORKERS := $(filter-out dynamorio,$(WORKERS))
endif

.PHONY: all
all: $(WORKERS)

# ld64 rejects -z; undefined symbols are an error by default on macOS.
ifeq ($(UNAME), Darwin)
SHARED_LDFLAGS := -shared
else
SHARED_LDFLAGS := -shared -Wl,-z,defs
endif

.PHONY: $(WORKERS)
$(WORKERS):
	$(MAKE) \
		CFLAGS="$(CFLAGS) -fPIC" \
		LDFLAGS="$(SHARED_LDFLAGS)" \
		-C $@/

.PHONY: clean
clean:
	for dir in $(WORKERS); do \
		$(MAKE) -C $$dir/ clean; \
	done
