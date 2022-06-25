use std::io;
use std::io::prelude::*;
use std::fs::File;

use crate::serializable::Serializable;

pub type ConstantPoolIndex = u32;

pub type FrameIndex = u16;

pub enum Bytecode {
    PushShort(i8),
    PushInt(i32),
    PushLong(i64),

    PushBool(bool),

    PushLiteral(ConstantPoolIndex),

    PushUnit,

    GetLocal(FrameIndex),
    SetLocal(FrameIndex),

    CallFunc{index: ConstantPoolIndex, arg_cnt: u8},
    Ret,

    Label(String),

    JmpShort(u16),
    Jmp(u32),
    JmpLong(u64),

    BranchShort(u16),
    Branch(u32),
    BranchLong(u64),

    Print{string: ConstantPoolIndex, arg_cnt: u8},

    Iadd,
    Isub,
    Imul,
    Idiv,
    Iand,
    Ior,
    // Rest of binary operations

    Drop,
    Dup,
}

pub struct Code {
    insert_point: Vec<Bytecode>,
}

impl Code {
    pub fn new() -> Self {
        Self { insert_point: vec![] }
    }

    pub fn add(&mut self, bytecode: Bytecode) {
        self.insert_point.push(bytecode);
    }

    pub fn add_cond(&mut self, bytecode: Bytecode, cond: bool) {
        if cond {
            self.add(bytecode);
        }
    }

    pub fn len(&self) -> usize {
        self.insert_point.len()
    }
}

impl Serializable for Code {
    /// Serializes the code into file in format: size - u8 | ins ...
    /// The size is not the size in bytes but number of instructions!
    fn serialize(&self, f: &mut File) -> io::Result<()> {
        f.write_all(&(self.insert_point.len() as u64).to_le_bytes())?;
        for instruction in &self.insert_point {
            instruction.serialize(f)?;
        }
        Ok(())
    }
}


impl Bytecode {
    fn byte_encode(&self) -> u8 {
        match self {
            Bytecode::PushShort(_) => 0x01,
            Bytecode::PushInt(_) => 0x02,
            Bytecode::PushLong(_) => 0x03,
            Bytecode::PushBool(_) => 0x04,
            Bytecode::PushLiteral(_) => 0x05,
            Bytecode::PushUnit => todo!(),
            Bytecode::GetLocal(_) => 0x06,
            Bytecode::SetLocal(_) => 0x07,
            Bytecode::CallFunc { index, arg_cnt } => 0x08,
            Bytecode::Ret => 0x09,
            Bytecode::Label(_) => 0x00,
            Bytecode::JmpShort(_) => 0x0A,
            Bytecode::Jmp(_) => 0x0B,
            Bytecode::JmpLong(_) => 0x0C,
            Bytecode::BranchShort(_) => 0x0D,
            Bytecode::Branch(_) => 0x0E,
            Bytecode::BranchLong(_) => 0x0F,
            Bytecode::Print { string, arg_cnt } => 0x10,
            Bytecode::Iadd => 0x30,
            Bytecode::Isub => 0x31,
            Bytecode::Imul => 0x32,
            Bytecode::Idiv => 0x33,
            Bytecode::Iand => 0x35,
            Bytecode::Ior => 0x36,

            Bytecode::Drop => 0x11,
            Bytecode::Dup => 0x12,
        }
    }
}

impl Serializable for Bytecode {
    fn serialize(&self, f: &mut File) -> io::Result<()> {
        f.write(&[self.byte_encode()])?;
        match self {
            Bytecode::PushShort(v) => f.write_all(&v.to_le_bytes())?,
            Bytecode::PushInt(v) => f.write_all(&v.to_le_bytes())?,
            Bytecode::PushLong(v) => f.write_all(&v.to_le_bytes())?,
            Bytecode::PushBool(v) => f.write_all(&[*v as u8])?,
            Bytecode::PushLiteral(v) => f.write_all(&v.to_le_bytes())?,
            Bytecode::GetLocal(_) => todo!(),
            Bytecode::SetLocal(_) => todo!(),
            Bytecode::CallFunc { index, arg_cnt } => todo!(),
            Bytecode::Ret => { },
            Bytecode::Label(_) => todo!(),
            Bytecode::JmpShort(_) => todo!(),
            Bytecode::Jmp(_) => todo!(),
            Bytecode::JmpLong(_) => todo!(),
            Bytecode::BranchShort(_) => todo!(),
            Bytecode::Branch(_) => todo!(),
            Bytecode::BranchLong(_) => todo!(),
            Bytecode::Print { string, arg_cnt } => todo!(),
            Bytecode::Iadd => { },
            Bytecode::Isub => { },
            Bytecode::Imul => { },
            Bytecode::Idiv => { },
            Bytecode::Iand => { },
            Bytecode::Ior => { },
            Bytecode::Drop => { },
            Bytecode::Dup => { },
            Bytecode::PushUnit => { },
        };
        Ok(())
    }
}
