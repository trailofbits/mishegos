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

static mut INSTR: Option<amd64::Instruction> = None;
#[no_mangle]
pub extern "C" fn try_decode(decode_result: *mut decode_result, bytes: *const u8, length: u8) {
    unsafe {
        if INSTR.is_none() {
            INSTR = Some(amd64::Instruction::default());
        }
    }
    let decode_result = unsafe { decode_result.as_mut().expect("decode_result is not null") };
    let data = unsafe {
        std::slice::from_raw_parts(bytes.as_ref().expect("bytes is not null"), length as usize)
    };
    let decoder = amd64::InstDecoder::default();
    let mut reader = yaxpeax_arch::U8Reader::new(data);

    match decoder.decode_into(unsafe { INSTR.as_mut().unwrap() }, &mut reader) {
        Err(amd64::DecodeError::ExhaustedInput) => {
            decode_result.status = decode_status_S_PARTIAL;
        }
        Err(_error) => {
            decode_result.status = decode_status_S_FAILURE;
        }
        Ok(()) => {
            let instr = unsafe { INSTR.as_ref().unwrap() };
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
