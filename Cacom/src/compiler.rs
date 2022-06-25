use crate::ast::{AST, Opcode};
use crate::bytecode::{Bytecode, Code};
use crate::serializable::{self, Serializable};

enum Location {
    Global,
    Local, // TODO: Environment
}

struct Context {
    
}

pub struct Program {
    // constant_pool,
    code: Code,
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

fn check_operator_arity(op: &Opcode, len: usize) -> Result<(), &'static str> {
    let arity = match op {
        Opcode::Add => 2,
        Opcode::Sub => 2,
        Opcode::Mul => 2,
        Opcode::Div => 2,
    };
    if arity != len {
        Err("Operator arity does not match.")
    } else {
        Ok(())
    }
}

fn _compile(ast: &AST, code: &mut Code, drop: bool) -> Result<(),  &'static str> {
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
        AST::CallFunction { name, arguments } => todo!(),
        AST::Top(_) => todo!(),
        AST::Block(_) => todo!(),
        AST::While { guard, body } => todo!(),
        AST::Conditional {
            guard,
            then_branch,
            else_branch,
        } => todo!(),
        AST::Print { format, arguments } => todo!(),
        AST::Operator { op, arguments } => {
            check_operator_arity(op, arguments.len())?;
            match op {
                Opcode::Add => code.add(Bytecode::Iadd),
                Opcode::Sub => code.add(Bytecode::Isub),
                Opcode::Mul => code.add(Bytecode::Imul),
                Opcode::Div => code.add(Bytecode::Idiv),
            }
        },
        AST::Return(_) => todo!(),
    }
    code.add_cond(Bytecode::Drop, drop);
    Ok(())
}

pub fn compile(ast: &AST) -> Result<Program, &'static str> {
    let mut code = Code::new();

    _compile(ast, &mut code, false)?;

    Ok(Program{code})
}