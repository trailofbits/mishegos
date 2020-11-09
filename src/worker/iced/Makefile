.PHONY: all
all: iced.so

iced.so: target/release/libiced.so
	cp target/release/libiced.so $@

target/release/libiced.so:
	cargo test --release
	cargo build --release

.PHONY: clean
clean:
	cargo clean --release
	rm -f *.so
