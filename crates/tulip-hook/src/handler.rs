use std::collections::HashMap;
use std::ptr;

#[derive(Clone, Debug)]
pub struct HandlerMetadata {
    pub convention: u32,
    pub abstractd: u32,
}

#[derive(Clone, Debug)]
pub struct HookMetadata {
    pub priority: i32,
}

#[derive(Clone, Debug)]
pub struct HandlerContent {
    functions: Vec<*mut ()>,
}

impl HandlerContent {
    fn new() -> HandlerContent {
        HandlerContent {
            functions: Vec::new(),
        }
    }
}

#[derive(Clone, Debug)]
pub struct Hook {
    pub address: *mut (),
    pub metadata: HookMetadata,
}

impl Hook {
    fn new(address: *mut (), metadata: HookMetadata) -> Hook {
        Hook { address, metadata }
    }
}

#[derive(Clone, Debug)]
pub struct Handler {
    pub address: *mut (),
    pub metadata: HandlerMetadata,
    pub trampoline: *mut (),
    pub trampoline_size: usize,
    pub handler: *mut (),
    pub handler_size: usize,
    pub original_bytes: Vec<u8>,
    pub modified_bytes: Vec<u8>,

    hooks: HashMap<u64, Hook>,
    handles: HashMap<*mut (), u64>,
    content: *mut HandlerContent,
    wrapped: *mut (),
}

impl Handler {
    pub fn new(address: *mut (), metadata: HandlerMetadata) -> Handler {
        Handler {
            address,
            metadata,
            hooks: HashMap::new(),
            handles: HashMap::new(),
            content: ptr::null_mut(),
            trampoline: ptr::null_mut(),
            trampoline_size: 0,
            wrapped: ptr::null_mut(),
            handler: ptr::null_mut(),
            handler_size: 0,
            original_bytes: Vec::new(),
            modified_bytes: Vec::new(),
        }
    }

    pub fn create(address: *mut (), metadata: HandlerMetadata) -> Handler {
        let mut handler = Handler::new(address, metadata);

        handler.content = Box::into_raw(Box::new(HandlerContent::new()));
        handler
    }

    pub fn init(&mut self) -> Result<(), &'static str> {
        self.add_original();
        Ok(())
    }

    pub fn add_original(&mut self) {
        let metadata = HookMetadata {
            priority: std::i32::MAX,
        };
        self.create_hook(self.wrapped, metadata);
    }

    pub fn create_hook(&mut self, address: *mut (), metadata: HookMetadata) -> u64 {
        static mut NEXT_HOOK_HANDLE: u64 = 0;
        unsafe {
            NEXT_HOOK_HANDLE += 1;
            let hook = Hook::new(address, metadata);
            self.hooks.insert(NEXT_HOOK_HANDLE, hook);
            self.handles.insert(address, NEXT_HOOK_HANDLE);
            self.content.as_mut().unwrap().functions.push(address);
            self.reorder_functions();
            NEXT_HOOK_HANDLE
        }
    }

    pub fn remove_hook(&mut self, hook: u64) {
        let address = self.hooks.get(&hook).unwrap().address;
        self.hooks.remove(&hook);
        self.handles.remove(&address);
        unsafe {
            self.content
                .as_mut()
                .unwrap()
                .functions
                .retain(|&x| x != address)
        };
    }

    pub fn clear_hooks(&mut self) {
        self.hooks.clear();
        self.handles.clear();
        unsafe { self.content.as_mut().unwrap().functions.clear() };
        self.add_original();
    }

    pub fn update_hook_metadata(&mut self, hook: u64, metadata: HookMetadata) {
        if let Some(hook) = self.hooks.get_mut(&hook) {
            hook.metadata = metadata;
            self.reorder_functions();
        }
    }

    fn reorder_functions(&mut self) {
        unsafe {
            self.content
                .as_mut()
                .unwrap()
                .functions
                .sort_by_key(|&address| {
                    self.hooks
                        .get(&self.handles[&address])
                        .unwrap()
                        .metadata
                        .priority
                })
        };
    }

    pub fn intervene_function(&mut self) -> Result<(), &'static str> {
        Ok(())
    }

    pub fn restore_function(&mut self) -> Result<(), &'static str> {
        Ok(())
    }
}
