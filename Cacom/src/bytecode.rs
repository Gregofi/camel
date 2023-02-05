use std::collections::HashMap;
use std::fmt;
use std::fs::File;
use std::io;
use std::io::prelude::*;

use crate::ast::Opcode;
use crate::serializable::Serializable;
use crate::utils::Location;

pub type ConstantPoolIndex = u32;
pub type LocalIndex = u16;

pub struct Bytecode {
    pub instr: BytecodeType,
    pub location: Location,
}

pub enum BytecodeType {
    PushShort(i16),
    PushInt(i32),
    PushLong(i64),

    PushBool(bool),

    PushLiteral(ConstantPoolIndex),

    PushNone,

    GetLocal(LocalIndex),
    SetLocal(LocalIndex),

    DeclValGlobal {
        name: ConstantPoolIndex,
    },
    DeclVarGlobal {
        name: ConstantPoolIndex,
    },

    GetGlobal(ConstantPoolIndex),
    SetGlobal(ConstantPoolIndex),

    GetMember(ConstantPoolIndex),
    SetMember(ConstantPoolIndex),

    NewObject(ConstantPoolIndex),

    CallFunc {
        arg_cnt: u8,
    },
    DispatchMethod {
        name: ConstantPoolIndex,
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
    Mod,
    Idiv,
    Iand,
    Ior,
    Iless,
    Ilesseq,
    Igreater,
    Igreatereq,
    Ieq,
    Ineg,
    Neq,
    // Rest of binary operations
    Drop,
    Dropn(u8),
    Dup,
}

impl From<Opcode> for BytecodeType {
    fn from(opcode: Opcode) -> Self {
        match opcode {
            Opcode::Add => BytecodeType::Iadd,
            Opcode::Sub => BytecodeType::Isub,
            Opcode::Mul => BytecodeType::Imul,
            Opcode::Div => BytecodeType::Idiv,
            Opcode::Mod => BytecodeType::Mod,
            Opcode::Less => BytecodeType::Iless,
            Opcode::LessEq => BytecodeType::Ilesseq,
            Opcode::Greater => BytecodeType::Igreater,
            Opcode::GreaterEq => BytecodeType::Igreatereq,
            Opcode::Eq => BytecodeType::Ieq,
            Opcode::Neq => BytecodeType::Neq,
            Opcode::Negate => BytecodeType::Ineg,
        }
    }
}

impl fmt::Display for Bytecode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self.instr {
            BytecodeType::PushShort(v) => write!(f, "Push short: {}", v),
            BytecodeType::PushInt(v) => write!(f, "Push int: {}", v),
            BytecodeType::PushLong(v) => write!(f, "Push long: {}", v),
            BytecodeType::PushBool(v) => write!(f, "Push bool: {}", v),
            BytecodeType::PushLiteral(v) => write!(f, "Push literal: {}", v),
            BytecodeType::PushNone => write!(f, "Push none"),
            BytecodeType::GetLocal(v) => write!(f, "Get local: {}", v),
            BytecodeType::SetLocal(v) => write!(f, "Set local: {}", v),
            BytecodeType::GetMember(v) => write!(f, "Get member: {}", v),
            BytecodeType::SetMember(v) => write!(f, "Set member: {}", v),
            BytecodeType::DeclValGlobal { name } => write!(f, "decl val global: {}", name),
            BytecodeType::DeclVarGlobal { name } => write!(f, "decl var global: {}", name),
            BytecodeType::GetGlobal(v) => write!(f, "Get global: {}", v),
            BytecodeType::SetGlobal(v) => write!(f, "Set global: {}", v),
            BytecodeType::CallFunc { arg_cnt } => write!(f, "Call function, args: {}", arg_cnt),
            BytecodeType::Ret => write!(f, "Ret"),
            BytecodeType::Label(v) => write!(f, "{}:", v),
            BytecodeType::BranchLabel(v) => write!(f, "BranchLabel: {}", v),
            BytecodeType::BranchLabelFalse(v) => write!(f, "BranchLabelFalse: {}", v),
            BytecodeType::JmpLabel(v) => write!(f, "BranchLabel: {}", v),
            BytecodeType::JmpShort(v) => write!(f, "JmpShort: {}", v),
            BytecodeType::Jmp(v) => write!(f, "Jmp: {}", v),
            BytecodeType::JmpLong(v) => write!(f, "JmpLong: {}", v),
            BytecodeType::BranchShort(v) => write!(f, "BranchShort: {}", v),
            BytecodeType::Branch(v) => write!(f, "Branch: {}", v),
            BytecodeType::BranchLong(v) => write!(f, "BranchLong: {}", v),
            BytecodeType::BranchShortFalse(v) => write!(f, "BranchShortFalse: {}", v),
            BytecodeType::BranchFalse(v) => write!(f, "BranchFalse: {}", v),
            BytecodeType::BranchLongFalse(v) => write!(f, "BranchLongFalse: {}", v),
            BytecodeType::Print { arg_cnt } => write!(f, "Print {}", arg_cnt),
            BytecodeType::Iadd => write!(f, "Iadd"),
            BytecodeType::Isub => write!(f, "Isub"),
            BytecodeType::Imul => write!(f, "Imul"),
            BytecodeType::Idiv => write!(f, "Idiv"),
            BytecodeType::Mod => write!(f, "Mod"),
            BytecodeType::Iand => write!(f, "Iand"),
            BytecodeType::Ior => write!(f, "Ior"),
            BytecodeType::Iless => write!(f, "Iless"),
            BytecodeType::Ilesseq => write!(f, "Ilesseq"),
            BytecodeType::Igreater => write!(f, "Igreater"),
            BytecodeType::Igreatereq => write!(f, "Igreatereq"),
            BytecodeType::Ieq => write!(f, "Ieq"),
            BytecodeType::Ineg => write!(f, "Ineg"),
            BytecodeType::Drop => write!(f, "Drop"),
            BytecodeType::Dropn(cnt) => write!(f, "Dropn: {}", cnt),
            BytecodeType::Dup => write!(f, "Dup"),
            BytecodeType::Neq => write!(f, "Neq"),
            BytecodeType::NewObject(idx) => write!(f, "NewObject: {}", idx),
            BytecodeType::DispatchMethod { name, arg_cnt } => {
                write!(f, "DispatchMethod: {} {}", name, arg_cnt)
            }
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
        self.code.len()
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
        match &self.instr {
            BytecodeType::PushShort(_) => 0x01,
            BytecodeType::PushInt(_) => 0x02,
            BytecodeType::PushLong(_) => 0x03,
            BytecodeType::PushBool(_) => 0x04,
            BytecodeType::PushLiteral(_) => 0x05,
            BytecodeType::PushNone => 0x20,
            BytecodeType::GetLocal(_) => 0x06,
            BytecodeType::SetLocal(_) => 0x07,
            BytecodeType::GetGlobal(_) => 0x13,
            BytecodeType::SetGlobal(_) => 0x14,
            BytecodeType::DeclValGlobal { .. } => 0x15,
            BytecodeType::DeclVarGlobal { .. } => 0x16,
            BytecodeType::CallFunc { .. } => 0x08,
            BytecodeType::Ret => 0x09,
            BytecodeType::Label(_) => 0x00,
            BytecodeType::BranchLabel(_) => {
                panic!("Label jumps are not meant to exist in final bytecode!")
            }
            BytecodeType::BranchLabelFalse(_) => {
                panic!("Label jumps are not meant to exist in final bytecode!")
            }
            BytecodeType::JmpLabel(_) => {
                panic!("Label jumps are not meant to exist in final bytecode!")
            }
            BytecodeType::JmpShort(_) => 0x0A,
            BytecodeType::Jmp(_) => 0x0B,
            BytecodeType::JmpLong(_) => 0x0C,
            BytecodeType::BranchShort(_) => 0x0D,
            BytecodeType::Branch(_) => 0x0E,
            BytecodeType::BranchLong(_) => 0x0F,
            BytecodeType::BranchShortFalse(_) => 0x2D,
            BytecodeType::BranchFalse(_) => 0x2E,
            BytecodeType::BranchLongFalse(_) => 0x2F,
            BytecodeType::Print { .. } => 0x10,
            BytecodeType::Iadd => 0x30,
            BytecodeType::Isub => 0x31,
            BytecodeType::Imul => 0x32,
            BytecodeType::Idiv => 0x33,
            BytecodeType::Mod => 0x34,
            BytecodeType::Iand => 0x35,
            BytecodeType::Ior => 0x36,
            BytecodeType::Iless => 0x37,
            BytecodeType::Ilesseq => 0x38,
            BytecodeType::Igreater => 0x39,
            BytecodeType::Igreatereq => 0x3A,
            BytecodeType::Ieq => 0x3B,
            BytecodeType::Ineg => 0x3C,
            BytecodeType::Neq => 0x3D,

            BytecodeType::Drop => 0x11,
            BytecodeType::Dropn(_) => 0x25,
            BytecodeType::Dup => 0x12,
            BytecodeType::NewObject(_) => 0x60,
            BytecodeType::GetMember(_) => 0x61,
            BytecodeType::SetMember(_) => 0x62,
            BytecodeType::DispatchMethod { .. } => 0x63,
        }
    }

    fn update_jump(&mut self, new_dest: usize) {
        match &mut self.instr {
            BytecodeType::JmpShort(v) => *v = new_dest.try_into().unwrap(),
            BytecodeType::Jmp(v) => *v = new_dest.try_into().unwrap(),
            BytecodeType::JmpLong(v) => *v = new_dest.try_into().unwrap(),
            BytecodeType::BranchShort(v) => *v = new_dest.try_into().unwrap(),
            BytecodeType::Branch(v) => *v = new_dest.try_into().unwrap(),
            BytecodeType::BranchLong(v) => *v = new_dest.try_into().unwrap(),
            _ => panic!("Instruction to be updated is not a jump"),
        }
    }

    pub fn size(&self) -> usize {
        if let BytecodeType::Label(_) = self.instr {
            return 0;
        }
        // TODO: Remove hardcoded size and calculate them from the variant, should be easy.
        1 + match &self.instr {
            BytecodeType::PushShort(_) => 2,
            BytecodeType::PushInt(_) => 4,
            BytecodeType::PushLong(_) => 8,
            BytecodeType::PushBool(_) => 1,
            BytecodeType::PushLiteral(_) => std::mem::size_of::<ConstantPoolIndex>(),
            BytecodeType::PushNone => 0,
            BytecodeType::GetLocal(idx) => std::mem::size_of_val(idx),
            BytecodeType::SetLocal(idx) => std::mem::size_of_val(idx),
            BytecodeType::DeclValGlobal { .. } => 4,
            BytecodeType::DeclVarGlobal { .. } => 4,
            BytecodeType::GetGlobal(_) => 4,
            BytecodeType::SetGlobal(_) => 4,
            BytecodeType::CallFunc { arg_cnt } => std::mem::size_of_val(arg_cnt),
            BytecodeType::Ret => 0,
            BytecodeType::Label(_) => unreachable!(),
            BytecodeType::JmpLabel(_) => 4,
            BytecodeType::BranchLabel(_) => 4,
            BytecodeType::BranchLabelFalse(_) => 4,
            BytecodeType::JmpShort(_) => 2,
            BytecodeType::Jmp(_) => 4,
            BytecodeType::JmpLong(_) => 8,
            BytecodeType::BranchShort(_) => 2,
            BytecodeType::Branch(_) => 4,
            BytecodeType::BranchLong(_) => 8,
            BytecodeType::BranchShortFalse(_) => 2,
            BytecodeType::BranchFalse(_) => 4,
            BytecodeType::BranchLongFalse(_) => 8,
            BytecodeType::Print { .. } => 1,
            BytecodeType::Iadd => 0,
            BytecodeType::Isub => 0,
            BytecodeType::Imul => 0,
            BytecodeType::Mod => 0,
            BytecodeType::Idiv => 0,
            BytecodeType::Iand => 0,
            BytecodeType::Ior => 0,
            BytecodeType::Iless => 0,
            BytecodeType::Ilesseq => 0,
            BytecodeType::Igreater => 0,
            BytecodeType::Igreatereq => 0,
            BytecodeType::Ieq => 0,
            BytecodeType::Neq => 0,
            BytecodeType::Ineg => 0,
            BytecodeType::Drop => 0,
            BytecodeType::Dropn(_) => 1,
            BytecodeType::Dup => 0,
            BytecodeType::GetMember(_) => 4,
            BytecodeType::SetMember(_) => 4,
            BytecodeType::NewObject(_) => 4,
            BytecodeType::DispatchMethod { .. } => 5,
        }
    }
}

impl Serializable for Bytecode {
    fn serialize(&self, f: &mut File) -> io::Result<()> {
        f.write_all(&[self.byte_encode()])?;
        match &self.instr {
            BytecodeType::PushShort(v) => f.write_all(&v.to_le_bytes())?,
            BytecodeType::PushInt(v) => f.write_all(&v.to_le_bytes())?,
            BytecodeType::PushLong(v) => f.write_all(&v.to_le_bytes())?,
            BytecodeType::PushBool(v) => f.write_all(&[*v as u8])?,
            BytecodeType::PushLiteral(v) => f.write_all(&v.to_le_bytes())?,
            BytecodeType::GetLocal(idx) => f.write_all(&idx.to_le_bytes())?,
            BytecodeType::SetLocal(idx) => f.write_all(&idx.to_le_bytes())?,
            BytecodeType::CallFunc { arg_cnt } => f.write_all(&arg_cnt.to_le_bytes())?,
            BytecodeType::Ret => {}
            BytecodeType::Label(_) => todo!(),
            BytecodeType::BranchLabel(_) => {
                panic!("Jump labels are not meant to exist in final bytecode")
            }
            BytecodeType::BranchLabelFalse(_) => {
                panic!("Jump labels are not meant to exist in final bytecode")
            }
            BytecodeType::JmpLabel(_) => {
                panic!("Jump labels are not meant to exist in final bytecode")
            }
            BytecodeType::JmpShort(dst) => f.write_all(&dst.to_le_bytes())?,
            BytecodeType::Jmp(dst) => f.write_all(&dst.to_le_bytes())?,
            BytecodeType::JmpLong(dst) => f.write_all(&dst.to_le_bytes())?,
            BytecodeType::BranchShort(dst) => f.write_all(&dst.to_le_bytes())?,
            BytecodeType::Branch(dst) => f.write_all(&dst.to_le_bytes())?,
            BytecodeType::BranchLong(dst) => f.write_all(&dst.to_le_bytes())?,
            BytecodeType::BranchShortFalse(dst) => f.write_all(&dst.to_le_bytes())?,
            BytecodeType::BranchFalse(dst) => f.write_all(&dst.to_le_bytes())?,
            BytecodeType::BranchLongFalse(dst) => f.write_all(&dst.to_le_bytes())?,
            BytecodeType::Print { arg_cnt } => {
                f.write_all(&arg_cnt.to_le_bytes())?;
            }
            BytecodeType::Iadd => {}
            BytecodeType::Isub => {}
            BytecodeType::Imul => {}
            BytecodeType::Idiv => {}
            BytecodeType::Mod => {}
            BytecodeType::Iand => {}
            BytecodeType::Ior => {}
            BytecodeType::Iless => {}
            BytecodeType::Ilesseq => {}
            BytecodeType::Igreater => {}
            BytecodeType::Igreatereq => {}
            BytecodeType::Ieq => {}
            BytecodeType::Neq => {}
            BytecodeType::Ineg => {}
            BytecodeType::Drop => {}
            BytecodeType::Dropn(cnt) => f.write_all(&cnt.to_le_bytes())?,
            BytecodeType::Dup => {}
            BytecodeType::PushNone => {}
            BytecodeType::DeclValGlobal { name } => f.write_all(&name.to_le_bytes())?,
            BytecodeType::DeclVarGlobal { name } => f.write_all(&name.to_le_bytes())?,
            BytecodeType::GetGlobal(idx) => f.write_all(&idx.to_le_bytes())?,
            BytecodeType::SetGlobal(idx) => f.write_all(&idx.to_le_bytes())?,
            BytecodeType::GetMember(idx) => f.write_all(&idx.to_le_bytes())?,
            BytecodeType::SetMember(idx) => f.write_all(&idx.to_le_bytes())?,
            BytecodeType::NewObject(idx) => f.write_all(&idx.to_le_bytes())?,
            BytecodeType::DispatchMethod { name, arg_cnt } => {
                f.write_all(&name.to_le_bytes())?;
                f.write_all(&arg_cnt.to_le_bytes())?;
            }
        };
        self.location.serialize(f)
    }
}
