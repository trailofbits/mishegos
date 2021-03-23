.PHONY: all
all: libyaxpeax_x86_mishegos.so

libyaxpeax_x86_mishegos.so: target/release/libyaxpeax_x86_mishegos.so
	cp target/release/libyaxpeax_x86_mishegos.so $@

target/release/libyaxpeax_x86_mishegos.so: src/lib.rs Cargo.toml
	cargo build --release

.PHONY: clean
clean:
	cargo clean
	rm -f *.so
