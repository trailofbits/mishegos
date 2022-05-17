.PHONY: all
all: iced.so

iced.so: target/release/libiced.$(SO_SUFFIX)
	cp target/release/libiced.$(SO_SUFFIX) $@

target/release/libiced.$(SO_SUFFIX):
	cargo test --release
	cargo build --release

.PHONY: clean
clean:
	cargo clean --release
	rm -f *.so
