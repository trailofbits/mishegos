UNAME := $(shell uname)

SO := so

ifeq ($(UNAME), Darwin)
	SO := dylib
endif

.PHONY: all
all: iced.so

iced.so: target/release/libiced.$(SO)
	cp target/release/libiced.$(SO) $@

target/release/libiced.$(SO):
	cargo test --release
	cargo build --release

.PHONY: clean
clean:
	cargo clean --release
	rm -f *.so
