.PHONY: all
all: cohort.py

cohort.py: cohort.ksy
	kaitai-struct-compiler -t python --outdir . cohort.ksy

.PHONY: clean
	rm -f cohort.py
