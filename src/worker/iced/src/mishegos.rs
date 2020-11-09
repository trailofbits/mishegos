#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]
// "warning: `extern` block uses type `u128`, which is not FFI-safe"
// "note: 128-bit integers don't currently have a known stable ABI"
#![allow(improper_ctypes)]
#![allow(clippy::redundant_static_lifetimes)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
