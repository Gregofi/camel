use std::collections::HashMap;
use std::fmt;
use std::fs::File;
use std::io;
use std::io::prelude::*;

use crate::serializable::Serializable;

pub type ConstantPoolIndex = u32;

pub type FrameIndex = u16;

pub enum Bytecode {
    PushShort(i8),
    PushInt(i32),
    PushLong(i64),

    PushBool(bool),

    PushLiteral(ConstantPoolIndex),

    PushNone,

    GetLocal(FrameIndex),
    SetLocal(FrameIndex),

    GetGlobal(ConstantPoolIndex),
    SetGlobal(ConstantPoolIndex),

    CallFunc {
        index: ConstantPoolIndex,
        arg_cnt: u8,
    },
    Ret,

    Label(String),

    // These guys are only used as helpers for code generation
    // They are not real instructions and can't be exported.
    // This helps with optimization because code removal
    // doesn't mess with jump offsets.
    JmpLabel(String),
    BranchLabel(String),
    BranchLabelFalse(String),

    JmpShort(u16),
    Jmp(u32),
    JmpLong(u64),

    BranchShort(u16),
    Branch(u32),
    BranchLong(u64),
    BranchShortFalse(u16),
    BranchFalse(u32),
    BranchLongFalse(u64),

    Print {
        arg_cnt: u8,
    },

    Iadd,
    Isub,
    Imul,
    Idiv,
    Iand,
    Ior,
    Iless,
    Ilesseq,
    Igreater,
    Igreatereq,
    Ieq,
    // Rest of binary operations
    Drop,
    Dup,
}

impl fmt::Display for Bytecode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Bytecode::PushShort(v) => write!(f, "Push short: {}", v),
            Bytecode::PushInt(v) => write!(f, "Push int: {}", v),
            Bytecode::PushLong(v) => write!(f, "Push long: {}", v),
            Bytecode::PushBool(v) => write!(f, "Push bool: {}", v),
            Bytecode::PushLiteral(v) => write!(f, "Push literal: {}", v),
            Bytecode::PushNone => write!(f, "Push none"),
            Bytecode::GetLocal(v) => write!(f, "Get local: {}", v),
            Bytecode::SetLocal(v) => write!(f, "Set local: {}", v),
            Bytecode::GetGlobal(v) => write!(f, "Get global: {}", v),
            Bytecode::SetGlobal(v) => write!(f, "Set global: {}", v),
            Bytecode::CallFunc { index, arg_cnt } => {
                write!(f, "Call function {}: {}", index, arg_cnt)
            }
            Bytecode::Ret => write!(f, "Ret"),
            Bytecode::Label(v) => write!(f, "{}:", v),
            Bytecode::BranchLabel(v) => write!(f, "BranchLabel: {}", v),
            Bytecode::BranchLabelFalse(v) => write!(f, "BranchLabelFalse: {}", v),
            Bytecode::JmpLabel(v) => write!(f, "BranchLabel: {}", v),
            Bytecode::JmpShort(v) => write!(f, "JmpShort: {}", v),
            Bytecode::Jmp(v) => write!(f, "Jmp: {}", v),
            Bytecode::JmpLong(v) => write!(f, "JmpLong: {}", v),
            Bytecode::BranchShort(v) => write!(f, "BranchShort: {}", v),
            Bytecode::Branch(v) => write!(f, "Branch: {}", v),
            Bytecode::BranchLong(v) => write!(f, "BranchLong: {}", v),
            Bytecode::BranchShortFalse(v) => write!(f, "BranchShortFalse: {}", v),
            Bytecode::BranchFalse(v) => write!(f, "BranchFalse: {}", v),
            Bytecode::BranchLongFalse(v) => write!(f, "BranchLongFalse: {}", v),
            Bytecode::Print { arg_cnt } => write!(f, "Print {}", arg_cnt),
            Bytecode::Iadd => write!(f, "Iadd"),
            Bytecode::Isub => write!(f, "Isub"),
            Bytecode::Imul => write!(f, "Imul"),
            Bytecode::Idiv => write!(f, "Idiv"),
            Bytecode::Iand => write!(f, "Iand"),
            Bytecode::Ior => write!(f, "Ior"),
            Bytecode::Iless => write!(f, "Iless"),
            Bytecode::Ilesseq => write!(f, "Ilesseq"),
            Bytecode::Igreater => write!(f, "Igreater"),
            Bytecode::Igreatereq => write!(f, "Igreatereq"),
            Bytecode::Ieq => write!(f, "Ieq"),
            Bytecode::Drop => write!(f, "Drop"),
            Bytecode::Dup => write!(f, "Dup"),
        }
    }
}

pub struct Code {
    pub code: Vec<Bytecode>,
}

impl fmt::Display for Code {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for ins in &self.code {
            writeln!(f, " {}", ins)?;
        }
        Ok(())
    }
}

impl Code {
    pub fn new() -> Self {
        Self { code: Vec::new() }
    }

    /// Appends the given instruction and returns its new index.
    pub fn add(&mut self, instruction: Bytecode) -> usize {
        self.code.push(instruction);
        self.code.len() - 1
    }

    pub fn add_cond(&mut self, instruction: Bytecode, cond: bool) {
        if cond {
            self.add(instruction);
        }
    }
}

impl Serializable for Code {
    /// Serializes the code into file in format: size - u16 | ins ...
    /// The size is not the size in bytes but number of instructions!
    fn serialize(&self, f: &mut File) -> io::Result<()> {
        let size: u32 =
            self.code.len().try_into().expect(
                "Bytecode overflow: There can be maximum of 2^32 instructions in one function",
            );
        f.write_all(&size.to_le_bytes())?;
        for instruction in &self.code {
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
            Bytecode::PushNone => 0x20,
            Bytecode::GetLocal(_) => 0x06,
            Bytecode::SetLocal(_) => 0x07,
            Bytecode::GetGlobal(_) => 0x13,
            Bytecode::SetGlobal(_) => 0x14,
            Bytecode::CallFunc { index, arg_cnt } => 0x08,
            Bytecode::Ret => 0x09,
            Bytecode::Label(_) => 0x00,
            Bytecode::BranchLabel(_) => {
                panic!("Label jumps are not meant to exist in final bytecode!")
            }
            Bytecode::BranchLabelFalse(_) => {
                panic!("Label jumps are not meant to exist in final bytecode!")
            }
            Bytecode::JmpLabel(_) => {
                panic!("Label jumps are not meant to exist in final bytecode!")
            }
            Bytecode::JmpShort(_) => 0x0A,
            Bytecode::Jmp(_) => 0x0B,
            Bytecode::JmpLong(_) => 0x0C,
            Bytecode::BranchShort(_) => 0x0D,
            Bytecode::Branch(_) => 0x0E,
            Bytecode::BranchLong(_) => 0x0F,
            Bytecode::BranchShortFalse(_) => 0x2D,
            Bytecode::BranchFalse(_) => 0x2E,
            Bytecode::BranchLongFalse(_) => 0x2F,
            Bytecode::Print { arg_cnt } => 0x10,
            Bytecode::Iadd => 0x30,
            Bytecode::Isub => 0x31,
            Bytecode::Imul => 0x32,
            Bytecode::Idiv => 0x33,
            Bytecode::Iand => 0x35,
            Bytecode::Ior => 0x36,
            Bytecode::Iless => 0x37,
            Bytecode::Ilesseq => 0x38,
            Bytecode::Igreater => 0x39,
            Bytecode::Igreatereq => 0x3A,
            Bytecode::Ieq => 0x3B,

            Bytecode::Drop => 0x11,
            Bytecode::Dup => 0x12,
        }
    }

    fn update_jump(&mut self, new_dest: usize) {
        match self {
            Bytecode::JmpShort(v) => *v = new_dest.try_into().unwrap(),
            Bytecode::Jmp(v) => *v = new_dest.try_into().unwrap(),
            Bytecode::JmpLong(v) => *v = new_dest.try_into().unwrap(),
            Bytecode::BranchShort(v) => *v = new_dest.try_into().unwrap(),
            Bytecode::Branch(v) => *v = new_dest.try_into().unwrap(),
            Bytecode::BranchLong(v) => *v = new_dest.try_into().unwrap(),
            _ => panic!("Instruction to be updated is not a jump"),
        }
    }
}

impl Serializable for Bytecode {
    fn serialize(&self, f: &mut File) -> io::Result<()> {
        f.write_all(&[self.byte_encode()])?;
        match self {
            Bytecode::PushShort(v) => f.write_all(&v.to_le_bytes())?,
            Bytecode::PushInt(v) => f.write_all(&v.to_le_bytes())?,
            Bytecode::PushLong(v) => f.write_all(&v.to_le_bytes())?,
            Bytecode::PushBool(v) => f.write_all(&[*v as u8])?,
            Bytecode::PushLiteral(v) => f.write_all(&v.to_le_bytes())?,
            Bytecode::GetLocal(_) => todo!(),
            Bytecode::SetLocal(_) => todo!(),
            Bytecode::CallFunc { index, arg_cnt } => {
                f.write_all(&index.to_le_bytes())?;
                f.write_all(&arg_cnt.to_le_bytes())?;
            }
            Bytecode::Ret => {}
            Bytecode::Label(name) => todo!(),
            Bytecode::BranchLabel(_) => {
                panic!("Jump labels are not meant to exist in final bytecode")
            }
            Bytecode::BranchLabelFalse(_) => {
                panic!("Jump labels are not meant to exist in final bytecode")
            }
            Bytecode::JmpLabel(_) => panic!("Jump labels are not meant to exist in final bytecode"),
            Bytecode::JmpShort(dst) => f.write_all(&dst.to_le_bytes())?,
            Bytecode::Jmp(dst) => f.write_all(&dst.to_le_bytes())?,
            Bytecode::JmpLong(dst) => f.write_all(&dst.to_le_bytes())?,
            Bytecode::BranchShort(dst) => f.write_all(&dst.to_le_bytes())?,
            Bytecode::Branch(dst) => f.write_all(&dst.to_le_bytes())?,
            Bytecode::BranchLong(dst) => f.write_all(&dst.to_le_bytes())?,
            Bytecode::BranchShortFalse(dst) => f.write_all(&dst.to_le_bytes())?,
            Bytecode::BranchFalse(dst) => f.write_all(&dst.to_le_bytes())?,
            Bytecode::BranchLongFalse(dst) => f.write_all(&dst.to_le_bytes())?,
            Bytecode::Print { arg_cnt } => {
                f.write_all(&arg_cnt.to_le_bytes())?;
            }
            Bytecode::Iadd => {}
            Bytecode::Isub => {}
            Bytecode::Imul => {}
            Bytecode::Idiv => {}
            Bytecode::Iand => {}
            Bytecode::Ior => {}
            Bytecode::Iless => {}
            Bytecode::Ilesseq => {}
            Bytecode::Igreater => {}
            Bytecode::Igreatereq => {}
            Bytecode::Ieq => {}
            Bytecode::Drop => {}
            Bytecode::Dup => {}
            Bytecode::PushNone => {}
            Bytecode::GetGlobal(idx) => f.write_all(&idx.to_le_bytes())?,
            Bytecode::SetGlobal(idx) => f.write_all(&idx.to_le_bytes())?,
        };
        Ok(())
    }
}
