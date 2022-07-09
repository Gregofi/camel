use std::fmt;

use crate::ast::{Opcode, AST};
use crate::bytecode::{Bytecode, Code, ConstantPoolIndex};
use crate::objects::{ConstantPool, Object};
use crate::serializable::{self, Serializable};

enum Location {
    Global,
    Local, // TODO: Environment
}

pub struct Context {
    constant_pool: ConstantPool,
}

pub struct Program {
    // constant_pool,
    code: Code,
}

impl Context {
    pub fn new() -> Self {
        Context {
            constant_pool: ConstantPool::new(),
        }
    }
}

impl Program {
    pub fn new() -> Self {
        Self { code: Code::new() }
    }
}

impl Serializable for Program {
    fn serialize(&self, f: &mut std::fs::File) -> std::io::Result<()> {
        self.code.serialize(f)?;
        Ok(())
    }
}

impl fmt::Display for Program {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.code.fmt(f)
    }
}

fn check_operator_arity(op: &Opcode, len: usize) -> Result<(), &'static str> {
    let arity = match op {
        Opcode::Add => 2,
        Opcode::Sub => 2,
        Opcode::Mul => 2,
        Opcode::Div => 2,
        Opcode::Less => 2,
        Opcode::LessEq => 2,
        Opcode::Greater => 2,
        Opcode::GreaterEq => 2,
        Opcode::Eq => 2,
    };
    if arity != len {
        Err("Operator arity does not match.")
    } else {
        Ok(())
    }
}

///
/// Inner compile function, compile the AST into bytecode which is inserted into Code.
/// If the result is not intended to be keeped on stack (param drop is true) then
/// a Drop instruction will be generated at the end.
fn _compile(
    ast: &AST,
    code: &mut Code,
    context: &mut Context,
    drop: bool,
) -> Result<(), &'static str> {
    match ast {
        AST::Integer(val) => code.add(Bytecode::PushInt(*val)),
        AST::Float(_) => todo!(),
        AST::Bool(val) => code.add(Bytecode::PushBool(*val)),
        AST::NoneVal => unimplemented!(), // TODO: Think about if we really want null values, some kind of Optional class would probably be better.
        AST::Variable { name, value } => todo!(),
        AST::List { size, values } => todo!(),
        AST::AccessVariable { name } => todo!(),
        AST::AccessList { list, index } => todo!(),
        AST::AssignVariable { name, value } => todo!(),
        AST::AssignList { list, index, value } => todo!(),
        AST::Function {
            name,
            parameters,
            body,
        } => todo!(),
        AST::CallFunction { name, arguments } => {
            for arg in arguments {
                _compile(arg, code, context, false)?;
            }

            let str_index: ConstantPoolIndex = context
                .constant_pool
                .add(Object::from(name.clone()))
                .try_into()
                .expect("Constant pool is full");
            code.add(Bytecode::CallFunc {
                index: str_index,
                arg_cnt: arguments.len().try_into().unwrap(),
            })
        }
        AST::Top(stmts) => {
            for stmt in stmts {
                _compile(stmt, code, context, true)?;
            }
        }
        AST::Block(_) => todo!(),
        AST::While { guard, body } => todo!(),
        AST::Conditional {
            guard,
            then_branch,
            else_branch,
        } => todo!(),
        AST::Operator { op, arguments } => {
            check_operator_arity(op, arguments.len())?;
            match op {
                Opcode::Add => code.add(Bytecode::Iadd),
                Opcode::Sub => code.add(Bytecode::Isub),
                Opcode::Mul => code.add(Bytecode::Imul),
                Opcode::Div => code.add(Bytecode::Idiv),
            }
        }
        AST::Return(_) => todo!(),
        AST::String(lit) => {
            let str_index: ConstantPoolIndex = context
                .constant_pool
                .add(Object::from(lit.clone()))
                .try_into()
                .expect("Constant pool is full");
            code.add(Bytecode::PushLiteral(str_index));
        }
    }
    code.add_cond(Bytecode::Drop, drop);
    Ok(())
}

pub fn compile(ast: &AST) -> Result<Program, &'static str> {
    let mut code = Code::new();
    let mut context = Context::new();

    _compile(ast, &mut code, &mut context, false)?;

    Ok(Program { code })
}
