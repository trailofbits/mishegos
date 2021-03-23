WORKERS = bfd \
	capstone \
	dynamorio \
	fadec \
	udis86 \
	xed \
	zydis \
	bddisasm \
	iced \
	yaxpeax-x86

.PHONY: all
all: worker $(WORKERS)

worker: worker.c

.PHONY: $(WORKERS)
$(WORKERS):
	$(MAKE) \
		CFLAGS="$(CFLAGS) -fPIC" \
		LDFLAGS="-shared" \
		-C $@/

.PHONY: clean
clean:
	rm -rf worker
	for dir in $(WORKERS); do \
		$(MAKE) -C $$dir/ clean; \
	done
