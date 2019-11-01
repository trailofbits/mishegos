WORKERS = bfd \
	capstone \
	dynamorio \
	udis86 \
	xed \
	zydis

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
