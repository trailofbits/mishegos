use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=wrapper.h");

    let clang_args = env::var("RUST_BINDGEN_CLANG_ARGS").unwrap_or_else(|_| "-I../../include".to_string());

    let bindings = PathBuf::from(env::var("OUT_DIR").unwrap()).join("mishegos.rs");
    bindgen::Builder::default()
        .header("../worker.h")
        .clang_args(clang_args.split_ascii_whitespace())
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("failed to generate bindings")
        .write_to_file(bindings)
        .expect("failed to write bindings");
}
