use {
    crate::handler::{Handler, HandlerMetadata},
    anyhow::Result as Anyhow,
    std::collections::HashMap,
};

#[derive(Clone, Debug)]
pub struct Pool {
    handlers: HashMap<*mut (), Handler>,
}

impl Pool {
    pub fn new() -> Pool {
        Pool {
            handlers: HashMap::new(),
        }
    }

    pub fn get() -> Pool {
        let instance: Pool = Pool::new();

        instance
    }

    pub fn create_handler(
        &mut self,
        address: *mut (),
        metadata: HandlerMetadata,
    ) -> Anyhow<*mut (), &'static str> {
        if !self.handlers.contains_key(&address) {
            let handler = Handler::create(address, metadata);

            self.handlers.insert(address, handler);
            self.handlers.get_mut(&address).unwrap().init()?;
        }

        self.handlers
            .get_mut(&address)
            .unwrap()
            .intervene_function()?;

        Ok(address)
    }

    pub fn remove_handler(&mut self, address: *mut ()) -> Anyhow<(), &'static str> {
        if let Some(handler) = self.handlers.get_mut(&address) {
            handler.clear_hooks();
            handler.restore_function()?;
        }

        Ok(())
    }

    pub fn get_handler(&self, address: *mut ()) -> Option<&Handler> {
        self.handlers.get(&address)
    }
}

impl Default for Pool {
    fn default() -> Self {
        Self::new()
    }
}
