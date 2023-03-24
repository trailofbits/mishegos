WORKERS = bfd \
	capstone \
	dynamorio \
	fadec \
	udis86 \
	xed \
	zydis \
	bddisasm \
	iced \
	yaxpeax-x86 \
	ghidra \
	llvm

.PHONY: all
all: $(WORKERS)

.PHONY: $(WORKERS)
$(WORKERS):
	$(MAKE) \
		CFLAGS="$(CFLAGS) -fPIC" \
		LDFLAGS="-shared -Wl,-z,defs" \
		-C $@/

.PHONY: clean
clean:
	for dir in $(WORKERS); do \
		$(MAKE) -C $$dir/ clean; \
	done
