use std::fmt;
use std::fs::File;
use std::io;
use std::io::prelude::*;

use crate::bytecode::Code;
use crate::bytecode::ConstantPoolIndex;
use crate::serializable::Serializable;

pub enum Object {
    String(String),
    Function {
        name: ConstantPoolIndex,
        parameters_cnt: u8,
        body: Code,
    },
}

pub struct ConstantPool {
    pub data: Vec<Object>,
}

impl ConstantPool {
    pub fn add(&mut self, obj: Object) -> ConstantPoolIndex {
        // Do not add same string multiple times
        if let Object::String(str) = &obj {
            let pos = self.data.iter().position(|obj| match obj {
                Object::String(searchee) => searchee == str,
                _ => false,
            });
            match pos {
                Some(val) => return val.try_into().unwrap(),
                None => (),
            }
        }

        self.data.push(obj);
        (self.data.len() - 1).try_into().unwrap()
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

impl Serializable for ConstantPool {
    fn serialize(&self, f: &mut File) -> io::Result<()> {
        let len: u32 = self
            .data
            .len()
            .try_into()
            .expect("Constant pool maximum size is 2^32");
        f.write_all(&len.to_le_bytes())?;
        for obj in &self.data {
            obj.serialize(f)?;
        }
        Ok(())
    }
}

impl Object {
    fn byte_encode(&self) -> u8 {
        match self {
            Object::String(_) => 0x01,
            Object::Function { .. } => 0x00,
        }
    }
}

impl fmt::Display for Object {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Object::String(str) => write!(f, "String: {}", str),
            Object::Function {
                name,
                parameters_cnt,
                body,
            } => {
                writeln!(f, "Function: {}, parameters: {}", name, parameters_cnt)?;
                writeln!(f, "{}", body)
            }
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
                let len: u32 = v.len().try_into().expect("String is too large");
                f.write_all(&len.to_le_bytes())?;
                f.write_all(v.as_bytes())
            }
            Object::Function {
                name,
                parameters_cnt,
                body,
            } => {
                f.write_all(&name.to_le_bytes())?;
                f.write_all(&parameters_cnt.to_le_bytes())?;
                body.serialize(f)
            }
        }
    }
}
