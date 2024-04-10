use std::collections::HashMap;

pub struct Wrapper {
    wrappers: HashMap<*mut (), *mut ()>,
    reverse_wrappers: HashMap<*mut (), *mut ()>,
}

impl Wrapper {
    pub fn get() -> Wrapper {
        let instance: Wrapper = Wrapper {
            wrappers: HashMap::new(),
            reverse_wrappers: HashMap::new(),
        };
        instance
    }

    pub fn create_wrapper(
        &mut self,
        address: *mut (),
        metadata: &WrapperMetadata,
    ) -> Result<*mut (), ()> {
        if !self.wrappers.contains_key(&address) {
            let generator = Target::get().get_wrapper_generator(address, metadata);
            let wrapped = generator.generate_wrapper()?;
            self.wrappers.insert(address, wrapped);
        }

        Ok(*self.wrappers.get(&address).unwrap())
    }

    pub fn create_reverse_wrapper(
        &mut self,
        address: *mut (),
        metadata: &WrapperMetadata,
    ) -> Result<*mut (), ()> {
        if !self.reverse_wrappers.contains_key(&address) {
            let generator = Target::get().get_wrapper_generator(address, metadata);
            let wrapped = generator.generate_reverse_wrapper()?;
            self.reverse_wrappers.insert(address, wrapped);
        }

        Ok(*self.reverse_wrappers.get(&address).unwrap())
    }
}

pub struct WrapperMetadata;
pub struct Target;

impl Target {
    pub fn get() -> Target {
        let instance: Target = Target {};

        instance
    }

    pub fn get_wrapper_generator(
        &self,
        _address: *mut (),
        _metadata: &WrapperMetadata,
    ) -> WrapperGenerator {
        WrapperGenerator
    }
}

pub struct WrapperGenerator;

impl WrapperGenerator {
    pub fn generate_wrapper(&self) -> Result<*mut (), ()> {
        Ok(std::ptr::null_mut())
    }

    pub fn generate_reverse_wrapper(&self) -> Result<*mut (), ()> {
        Ok(std::ptr::null_mut())
    }
}
