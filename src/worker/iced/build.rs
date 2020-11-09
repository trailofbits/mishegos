use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=wrapper.h");

    let clang_args = env::var("RUST_BINDGEN_CLANG_ARGS").unwrap();
    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .clang_args(clang_args.split_ascii_whitespace())
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
