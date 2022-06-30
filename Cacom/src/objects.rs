use std::io;
use std::io::prelude::*;
use std::fs::File;

use crate::bytecode::{ConstantPoolIndex, FrameIndex, Code};
use crate::serializable::Serializable;

pub struct Range{begin: u64, length: u64}

pub enum Object {
    String(String),
    Function{
        name: ConstantPoolIndex,
        parameters_cnt: u8,
        locals: FrameIndex,
        // The beginning and length of the bytecode
        range: Range,
    },
}

pub struct ConstantPool{data: Vec<Object>}

impl ConstantPool {
    pub fn add(&mut self, obj: Object) -> usize {
        self.data.push(obj);
        self.data.len() - 1
    }

    pub fn new() -> Self {
        let c: Self;
        c
    }
}

impl Object {
    fn byte_encode(&self) -> u8 {
        match self {
            Object::String(_) => 0x01,
            Object::Function { name, parameters_cnt, locals, range } => 0x00,
        }
    }
}

impl From<String> for Object {
    fn from(v: String) -> Self {
        Object::String(v)
    }
}

impl Serializable for Object {
    fn serialize(&self, f: &mut File) -> io::Result<()> {
        f.write_all(&self.byte_encode().to_le_bytes())?;
        match self {
            Object::String(v) => {
                f.write_all(v.as_bytes())
            },
            Object::Function { name, parameters_cnt, locals, range } => {
                f.write_all(&name.to_le_bytes())?;
                f.write_all(&parameters_cnt.to_le_bytes())?;
                f.write_all(&locals.to_le_bytes())?;
                f.write_all(&range.begin.to_le_bytes())?;
                f.write_all(&range.length.to_le_bytes())?;
                Ok(())
            },
        }
    }
}
