use std::collections::HashMap;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum X86Register {
    EAX = 0x0,
    ECX,
    EDX,
    EBX,
    ESP,
    EBP,
    ESI,
    EDI,
    XMM0 = 0x80,
    XMM1,
    XMM2,
    XMM3,
    XMM4,
    XMM5,
    XMM6,
    XMM7,
}

#[derive(Debug, Clone, Copy)]
pub struct X86Pointer {
    pub reg: X86Register,
    pub offset: i32,
}

impl X86Pointer {
    pub fn new(reg: X86Register, offset: i32) -> Self {
        X86Pointer { reg, offset }
    }
}

impl std::ops::Add<i32> for X86Register {
    type Output = X86Pointer;

    fn add(self, offset: i32) -> Self::Output {
        X86Pointer::new(self, offset)
    }
}

#[derive(Debug, Clone, Copy)]
pub struct RegMem32;

impl RegMem32 {
    pub fn index(&self, ptr: X86Pointer) -> X86Pointer {
        ptr
    }
}

pub enum X86Operand {
    Register(X86Register),
    ModRM(X86Pointer),
}

impl X86Operand {
    fn reg_idx(&self) -> u8 {
        match self {
            X86Operand::Register(reg) => *reg as u8,
            X86Operand::ModRM(ptr) => ptr.reg as u8,
        }
    }
}

#[derive(Clone, Debug)]
pub struct X86Assembler {
    base_address: i64,
    buffer: Vec<u8>,
    labels: HashMap<String, i64>,
    label_updates: Vec<AssemblerLabelUpdates>,
}

#[derive(Debug, Clone)]
struct AssemblerLabelUpdates {
    address: i64,
    name: String,
    size: u8,
}

impl X86Assembler {
    pub fn new(base_address: i64) -> Self {
        X86Assembler {
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
        for update in &self.label_updates.clone() {
            let label = *self.labels.get(&update.name).unwrap() as i32;
            let address = update.address;

            self.rewrite32(address, label);
        }
    }

    pub fn label32(&mut self, name: &str) {
        self.label_updates.push(AssemblerLabelUpdates {
            address: self.current_address(),
            name: name.to_string(),
            size: 4,
        });
        self.write32(0);
    }

    fn encode_mod_rm(&mut self, op: &X86Operand, digit: i8) {
        match op {
            X86Operand::Register(reg) => {
                self.write8((0b11 << 6) | (digit << 3) | (*reg as i8));
            }
            X86Operand::ModRM(ptr) => {
                let mod_bits = if ptr.offset == 0 && ptr.reg != X86Register::EBP {
                    0b00
                } else if ptr.offset <= 0x7f && ptr.offset >= -0x80 {
                    0b01
                } else {
                    0b10
                };

                self.write8((mod_bits << 6) | (digit << 3) | (ptr.reg as i8));
                if ptr.reg == X86Register::ESP {
                    self.write8(0x24);
                }

                if mod_bits == 0b01 {
                    self.write8(ptr.offset as i8);
                } else if mod_bits == 0b10 {
                    self.write32(ptr.offset);
                }
            }
        }
    }

    pub fn nop(&mut self) {
        self.write8(0x90u8 as i8);
    }

    pub fn int3(&mut self) {
        self.write8(0xCCu8 as i8);
    }

    pub fn ret(&mut self) {
        self.write8(0xC3u8 as i8);
    }

    pub fn ret_offset(&mut self, offset: i16) {
        self.write8(0xC2u8 as i8);
        self.write16(offset);
    }

    pub fn add(&mut self, reg: X86Register, value: i32) {
        self.write8(0x81u8 as i8);
        self.write8(0xC0u8 as i8 | (reg as i8));
        self.write32(value);
    }

    pub fn sub(&mut self, reg: X86Register, value: i32) {
        self.write8(0x81u8 as i8);
        self.write8(0xE8u8 as i8 | (reg as i8));
        self.write32(value);
    }

    pub fn push(&mut self, reg: X86Register) {
        self.write8(0x50 | (reg as i8));
    }

    pub fn push_ptr(&mut self, ptr: X86Pointer) {
        self.write8(0xFFu8 as i8);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), 6);
    }

    pub fn pop(&mut self, reg: X86Register) {
        self.write8(0x58 | (reg as i8));
    }

    pub fn jmp(&mut self, reg: X86Register) {
        self.write8(0xFFu8 as i8);
        self.encode_mod_rm(&X86Operand::Register(reg), 4);
    }

    pub fn jmp_addr(&mut self, address: i64) {
        self.write8(0xE9u8 as i8);
        self.write32((address - self.current_address() - 5 + 1) as i32);
    }

    pub fn call(&mut self, reg: X86Register) {
        self.write8(0xFFu8 as i8);
        self.encode_mod_rm(&X86Operand::Register(reg), 2);
    }

    pub fn call_addr(&mut self, address: i64) {
        self.write8(0xE8u8 as i8);
        self.write32((address - self.current_address() - 5 + 1) as i32);
    }

    pub fn movsd_ptr(&mut self, reg: X86Register, ptr: X86Pointer) {
        self.write8(0xF2u8 as i8);
        self.write8(0x0F);
        self.write8(0x10);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), reg as i8);
    }

    pub fn movsd(&mut self, ptr: X86Pointer, reg: X86Register) {
        self.write8(0xF2u8 as i8);
        self.write8(0x0F);
        self.write8(0x11);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), reg as i8);
    }

    pub fn movss_ptr(&mut self, reg: X86Register, ptr: X86Pointer) {
        self.write8(0xF3u8 as i8);
        self.write8(0x0F);
        self.write8(0x10);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), reg as i8);
    }

    pub fn movss_reg(&mut self, ptr: X86Pointer, reg: X86Register) {
        self.write8(0xF3u8 as i8);
        self.write8(0x0F);
        self.write8(0x11);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), reg as i8);
    }

    pub fn movaps(&mut self, reg: X86Register, ptr: X86Pointer) {
        self.write8(0x0F);
        self.write8(0x28);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), reg as i8);
    }

    pub fn movaps_reg(&mut self, ptr: X86Pointer, reg: X86Register) {
        self.write8(0x0F);
        self.write8(0x29);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), reg as i8);
    }

    pub fn lea(&mut self, reg: X86Register, label: &str) {
        self.write8(0x8Du8 as i8);
        self.write8(0x05 | (reg as i8) << 3);
        self.label32(label);
    }

    pub fn mov_val(&mut self, reg: X86Register, value: i32) {
        self.write8(0xB8u8 as i8 | (reg as i8));
        self.write32(value);
    }

    pub fn mov_ptr(&mut self, reg: X86Register, ptr: X86Pointer) {
        self.write8(0x8Bu8 as i8);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), reg as i8);
    }

    pub fn mov_reg(&mut self, ptr: X86Pointer, reg: X86Register) {
        self.write8(0x89u8 as i8);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), reg as i8);
    }

    pub fn mov_reg2(&mut self, dst: X86Register, src: X86Register) {
        self.write8(0x89u8 as i8);
        self.encode_mod_rm(&X86Operand::Register(dst), src as i8);
    }

    pub fn mov_label(&mut self, reg: X86Register, label: &str) {
        self.write8(0x8Bu8 as i8);
        self.write8(0x05 | (reg as i8) << 3);
        self.label32(label);
    }

    pub fn fstps(&mut self, ptr: X86Pointer) {
        self.write8(0xD9u8 as i8);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), 3);
    }

    pub fn flds(&mut self, ptr: X86Pointer) {
        self.write8(0xD9u8 as i8);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), 0);
    }

    pub fn fstpd(&mut self, ptr: X86Pointer) {
        self.write8(0xDDu8 as i8);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), 3);
    }

    pub fn fldd(&mut self, ptr: X86Pointer) {
        self.write8(0xDDu8 as i8);
        self.encode_mod_rm(&X86Operand::ModRM(ptr), 0);
    }
}
