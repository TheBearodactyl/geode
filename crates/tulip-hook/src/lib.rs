use anyhow::Result as Anyhow;
use handler::HandlerMetadata;
use pool::Pool;

pub mod assembler;
pub mod handler;
pub mod misc;
pub mod pool;
pub mod wrapper;

pub fn create_handler(
    address: *mut (),
    metadata: HandlerMetadata,
) -> Anyhow<*mut (), &'static str> {
    Pool::get().create_handler(address, metadata)
}

pub fn remove_handler(address: *mut ()) -> Anyhow<(), &'static str> {
    Pool::get().remove_handler(address)
}
