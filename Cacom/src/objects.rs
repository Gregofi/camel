use std::fmt;
use std::io;
use std::io::prelude::*;
use std::fs::File;

use crate::bytecode::{ConstantPoolIndex, FrameIndex};
use crate::serializable::Serializable;

#[derive(PartialEq)]
pub struct Range{begin: u64, length: u64}

#[derive(PartialEq)]
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

impl fmt::Display for Range {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "begin: {}, len: {}", self.begin, self.length)
    }
}

impl ConstantPool {
    pub fn add(&mut self, obj: Object) -> usize {
        // Check if the item was already added before, if so, just return its index.
        // TODO: Linear for number of items
        if self.data.contains(&obj) {
            let pos = self.data.iter().position(|item| *item == obj);
            if pos.is_some() {
                return pos.unwrap();
            }
        }

        // Else push it back
        self.data.push(obj);
        self.data.len() - 1
    }

    pub fn new() -> Self {
        ConstantPool { data: Vec::new() }
    }
}

impl fmt::Display for ConstantPool {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for obj in &self.data {
            writeln!(f, "{}", obj)?;
        }
        Ok(())
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

impl fmt::Display for Object {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Object::String(str) => write!(f, "String: {}", str),
            Object::Function { name, parameters_cnt, locals, range } => {
                write!(f, "Function: {}, parameters: {}, locals: {}, range: {}", name, parameters_cnt, locals, range)
            },
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
