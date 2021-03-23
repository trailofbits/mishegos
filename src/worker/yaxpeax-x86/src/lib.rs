use std::os::raw::c_char;
use yaxpeax_x86::long_mode as amd64;
use yaxpeax_arch::{AddressBase, Decoder, LengthedInstruction};

// shhh no warnings please
#[allow(warnings)]
mod mishegos {
    include!(concat!(env!("OUT_DIR"), "/mishegos.rs"));
}

use crate::mishegos::{decode_result, MISHEGOS_DEC_MAXLEN, decode_status_S_SUCCESS, decode_status_S_FAILURE, decode_status_S_PARTIAL};

#[no_mangle]
pub static mut worker_name: *const c_char = b"yaxpeax-x86-mishegos\x00".as_ptr() as *const i8;

#[no_mangle]
pub extern "C" fn try_decode(decode_result: *mut decode_result, bytes: *const u8, length: u8) {
    let decode_result = unsafe { decode_result.as_mut().expect("decode_result is not null") };
    let data = unsafe {
        std::slice::from_raw_parts(bytes.as_ref().expect("bytes is not null"), length as usize)
    };
    let decoder = amd64::InstDecoder::default();

    match decoder.decode(data.iter().cloned()) {
        Err(amd64::DecodeError::ExhaustedInput) => {
            decode_result.status = decode_status_S_PARTIAL;
        }
        Err(_error) => {
            decode_result.status = decode_status_S_FAILURE;
        }
        Ok(instr) => {
            decode_result.ndecoded = 0u64.wrapping_offset(instr.len()) as u16;
            let text = instr.to_string();
            assert!(text.len() < MISHEGOS_DEC_MAXLEN as usize);
            for (i, x) in text.as_bytes().iter().enumerate() {
                decode_result.result[i] = *x as i8;
            }
            decode_result.len = text.len() as u16;
            decode_result.status = decode_status_S_SUCCESS;
        }
    };
}
