UNAME := $(shell uname)

SO := so

ifeq ($(UNAME), Darwin)
	SO := dylib
endif

.PHONY: all
all: libyaxpeax_x86_mishegos.so

libyaxpeax_x86_mishegos.so: target/release/libyaxpeax_x86_mishegos.$(SO)
	cp target/release/libyaxpeax_x86_mishegos.$(SO) $@

target/release/libyaxpeax_x86_mishegos.$(SO): src/lib.rs Cargo.toml
	cargo build --release

.PHONY: clean
clean:
	cargo clean
	rm -f *.so
