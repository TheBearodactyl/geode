use std::collections::HashMap;

pub struct BaseAssembler {
    pub base_address: i64,
    pub buffer: Vec<u8>,
    pub labels: HashMap<String, i64>,
    pub label_updates: Vec<AssemblerLabelUpdates>,
}

#[derive(Debug)]
struct AssemblerLabelUpdates {
    address: i64,
    name: String,
    size: u8,
}

impl BaseAssembler {
    pub fn new(base_address: i64) -> Self {
        BaseAssembler {
            base_address,
            buffer: Vec::new(),
            labels: HashMap::new(),
            label_updates: Vec::new(),
        }
    }

    pub fn current_address(&self) -> i64 {
        self.base_address + (self.buffer.len() as i64)
    }

    pub fn buffer(&self) -> &[u8] {
        &self.buffer
    }

    pub fn read8(&self, address: i64) -> i8 {
        self.buffer[(address - self.base_address) as usize] as i8
    }

    pub fn read16(&self, address: i64) -> i16 {
        let lo = self.read8(address) as u16;
        let hi = self.read8(address + 1) as u16;
        ((hi << 8) | lo) as i16
    }

    pub fn read32(&self, address: i64) -> i32 {
        let lo = self.read16(address) as u32;
        let hi = self.read16(address + 2) as u32;
        ((hi << 16) | lo) as i32
    }

    pub fn read64(&self, address: i64) -> i64 {
        let lo = self.read32(address) as u64;
        let hi = self.read32(address + 4) as u64;
        ((hi << 32) | lo) as i64
    }

    pub fn write8(&mut self, value: i8) {
        self.buffer.push(value as u8);
    }

    pub fn write16(&mut self, value: i16) {
        self.write8(value as i8);
        self.write8((value >> 8) as i8);
    }

    pub fn write32(&mut self, value: i32) {
        self.write16(value as i16);
        self.write16((value >> 16) as i16);
    }

    pub fn write64(&mut self, value: i64) {
        self.write32(value as i32);
        self.write32((value >> 32) as i32);
    }

    pub fn rewrite8(&mut self, address: i64, value: i8) {
        self.buffer[(address - self.base_address) as usize] = value as u8;
    }

    pub fn rewrite16(&mut self, address: i64, value: i16) {
        self.rewrite8(address, value as i8);
        self.rewrite8(address + 1, (value >> 8) as i8);
    }

    pub fn rewrite32(&mut self, address: i64, value: i32) {
        self.rewrite16(address, value as i16);
        self.rewrite16(address + 2, (value >> 16) as i16);
    }

    pub fn rewrite64(&mut self, address: i64, value: i64) {
        self.rewrite32(address, value as i32);
        self.rewrite32(address + 4, (value >> 32) as i32);
    }

    pub fn label(&mut self, name: &str) {
        self.labels.insert(name.to_string(), self.current_address());
    }

    pub fn update_labels(&mut self) {
        // TODO: Implement `update_labels` method
    }
}
