pub struct HookMetadata;

pub struct Hook {
    pub metadata: HookMetadata,
    pub address: *mut std::ffi::c_void,
}

impl Hook {
    pub fn new(address: *mut std::ffi::c_void, metadata: HookMetadata) -> Self {
        Hook { 
            metadata, 
            address 
        }
    }
}
