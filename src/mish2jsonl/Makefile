.PHONY: all
all: cohorts.py

cohorts.py: cohorts.ksy
	kaitai-struct-compiler -t python --outdir . cohorts.ksy

.PHONY: clean
	rm -f cohorts.py
