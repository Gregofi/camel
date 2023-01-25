use std::fmt;
use std::fs::File;
use std::io;
use std::io::prelude::*;

use crate::bytecode::{Code, ConstantPoolIndex, LocalIndex};
use crate::serializable::Serializable;

// TODO: Replace Object::Function internals with this guy
pub struct Function {
    pub name: ConstantPoolIndex,
    pub parameters_cnt: u8,
    pub locals_cnt: LocalIndex,
    pub body: Code,
}

pub enum Object {
    String(String),
    Function(Function),
    Class {
        name: ConstantPoolIndex,
        methods: Vec<Function>,
        constructor: Function,
    },
}

impl fmt::Display for Function {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        writeln!(
            f,
            "Function: {}, parameters: {}, locals: {}",
            self.name, self.parameters_cnt, self.locals_cnt
        )?;
        writeln!(f, "{}", self.body)
    }
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
            Object::Class {
                name,
                methods,
                constructor,
            } => todo!(),
        }
    }
}

impl fmt::Display for Object {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Object::String(str) => write!(f, "String: {}", str),
            Object::Function(Function {
                name,
                parameters_cnt,
                locals_cnt,
                body,
            }) => {
                writeln!(
                    f,
                    "Function: {}, parameters: {}, locals: {}",
                    name, parameters_cnt, locals_cnt
                )?;
                writeln!(f, "{}", body)
            }
            Object::Class {
                name,
                methods,
                constructor} => {
                writeln!(f, "Class: {}", name)?;
                writeln!(f, "=== Contructor ===")?;
                writeln!(f, "{}", constructor)?;
                if methods.len() != 0 {
                    writeln!(f, "=== Methods ===")?;
                    for method in methods {
                        writeln!(f, "{}", method)?;
                    }
                }
                Ok(())
            }
        }
    }
}

impl From<String> for Object {
    fn from(v: String) -> Self {
        Object::String(v)
    }
}

impl Serializable for Function {
    fn serialize(&self, f: &mut File) -> io::Result<()> {
        f.write_all(&self.name.to_le_bytes())?;
        f.write_all(&self.parameters_cnt.to_le_bytes())?;
        f.write_all(&self.locals_cnt.to_le_bytes())?;
        self.body.serialize(f)
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
            Object::Function(fun) => {
                fun.serialize(f)
            }
            Object::Class {
                name,
                methods,
                constructor,
            } => {
                f.write_all(&name.to_le_bytes())?;
                constructor.serialize(f)?;
                f.write_all(&methods.len().to_le_bytes())?;
                for method in methods {
                    method.serialize(f)?;
                }
                Ok(())
            }
        }
    }
}
