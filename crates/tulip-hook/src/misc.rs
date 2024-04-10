#![allow(unused_variables)]

use anyhow::Result as Anyhow;

pub fn follow_jumps(address: *mut std::ffi::c_void) -> Anyhow<*mut std::ffi::c_void, &'static str> {
    Err("Implement followJumps in platform")
}
