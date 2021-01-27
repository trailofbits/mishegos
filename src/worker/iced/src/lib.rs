mod mishegos;

use iced_x86::*;
use mishegos::{
    decode_result, decode_status_S_FAILURE, decode_status_S_PARTIAL, decode_status_S_SUCCESS,
};

// This is pretty ugly and assumes sizeof(char) == 1
#[no_mangle]
pub static mut worker_name: *const std::os::raw::c_char =
    WORKER_NAME.as_ptr() as *const std::os::raw::c_char;
static WORKER_NAME: &str = "iced\0";

#[allow(clippy::missing_safety_doc)]
#[no_mangle]
pub unsafe extern "C" fn try_decode(result: *mut decode_result, raw_insn: *const u8, length: u8) {
    assert!(!result.is_null());
    assert!(!raw_insn.is_null());
    let data = std::slice::from_raw_parts(raw_insn, length as usize);
    let result = &mut *result;
    let error = match try_decode_safe(64, 0, data) {
        Err(error) => error,
        Ok((instr_len, output)) => {
            result.ndecoded = instr_len as u16;
            assert_eq!(std::mem::size_of::<std::os::raw::c_char>(), 1);
            if output.len() > result.result.len() {
                decode_status_S_FAILURE
            } else {
                std::ptr::copy(
                    output.as_ptr(),
                    result.result.as_mut_ptr() as *mut u8,
                    output.len(),
                );
                result.len = output.len() as u16;
                decode_status_S_SUCCESS
            }
        }
    };
    result.status = error;
}

fn try_decode_safe(bitness: u32, ip: u64, data: &[u8]) -> Result<(usize, String), u32> {
    const DECODER_OPTIONS: u32 = DecoderOptions::NONE;

    let mut decoder = Decoder::new(bitness, data, DECODER_OPTIONS);
    decoder.set_ip(ip);
    let instr = decoder.decode();

    if instr.is_invalid() {
        match decoder.last_error() {
            DecoderError::None => unreachable!(),
            DecoderError::NoMoreBytes => Err(decode_status_S_PARTIAL),
            _ => Err(decode_status_S_FAILURE),
        }
    } else {
        let mut formatter = IntelFormatter::new();
        // Try to match default XED output
        formatter.options_mut().set_hex_suffix("");
        formatter.options_mut().set_hex_prefix("0x");
        formatter.options_mut().set_uppercase_hex(false);
        formatter
            .options_mut()
            .set_space_after_operand_separator(true);
        formatter
            .options_mut()
            .set_memory_size_options(MemorySizeOptions::Always);
        formatter.options_mut().set_always_show_scale(true);
        formatter.options_mut().set_rip_relative_addresses(true);
        formatter
            .options_mut()
            .set_small_hex_numbers_in_decimal(false);
        formatter.options_mut().set_cc_ge(CC_ge::nl);
        formatter.options_mut().set_cc_a(CC_a::nbe);
        formatter.options_mut().set_cc_e(CC_e::z);
        formatter.options_mut().set_cc_ne(CC_ne::nz);
        formatter.options_mut().set_cc_ae(CC_ae::nb);
        formatter.options_mut().set_cc_g(CC_g::nle);
        formatter.options_mut().set_show_branch_size(false);
        formatter.options_mut().set_branch_leading_zeroes(false);
        formatter.options_mut().set_use_pseudo_ops(false);

        let mut output = String::new();
        formatter.format(&instr, &mut output);

        Ok((instr.len(), output))
    }
}
