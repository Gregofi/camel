use std::collections::HashMap;
use std::fmt;

use crate::ast::{Opcode, AST};
use crate::bytecode::{Bytecode, Code, ConstantPoolIndex};
use crate::objects::{ConstantPool, Object};
use crate::serializable::Serializable;

enum Location {
    Global,
    Local, // TODO: Environment
}

struct Function {
    name: String,
    parameters: Vec<String>,
    body: Box<AST>,
}

pub struct Context {
    constant_pool: ConstantPool,
    // Functions are first collected and generated
    // only when all of global statement have been generated.
    functions_def: Vec<Function>,
}

pub struct Program {
    constant_pool: ConstantPool,
    code: Code,
}

impl Context {
    pub fn new() -> Self {
        Context {
            constant_pool: ConstantPool::new(),
            functions_def: Vec::new(),
        }
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
        self.constant_pool.fmt(f)?;
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

/// Removes jumps to labels and replaces them with offset jumps
fn jump_pass(code: Vec<Bytecode>) -> Vec<Bytecode> {
    let mut labels: HashMap<String, usize> = HashMap::new();

    // Copy the bytecode without labels but store their (labels) location
    let mut without_labels: Vec<Bytecode> = Vec::new();
    for (idx, ins) in code.into_iter().enumerate() {
        match ins {
            Bytecode::Label(str) => {
                labels.insert(str, idx);
            },
            _ => without_labels.push(ins)
        }
    }

    // Remove the jump to labels with jump to address
    // TODO: Use short, long or normal jump based on distance
    without_labels.into_iter().map(|ins| match ins {
        Bytecode::JmpLabel(label) => {
            let label_index = *labels.get(&label).unwrap();
            Bytecode::Jmp(label_index.try_into().unwrap())
        },
        Bytecode::BranchLabel(label) => {
            let label_index = *labels.get(&label).unwrap();
            Bytecode::Branch(label_index.try_into().unwrap())
        },
        Bytecode::BranchLabelFalse(label) => {
            let label_index = *labels.get(&label).unwrap();
            Bytecode::BranchFalse(label_index.try_into().unwrap())
        },
        _ => ins,
    }).collect()
}

/// Inner compile function, compile the AST into bytecode which is inserted into Code.
/// If the result is not intended to be keeped on stack (param drop is true) then
/// a Drop instruction will be generated at the end.
fn _compile(
    ast: &AST,
    code: &mut Code,
    context: &mut Context,
    current_function: ConstantPoolIndex,
    drop: bool,
) -> Result<(), &'static str> {
    match ast {
        AST::Integer(val) => {
            code.add(current_function, Bytecode::PushInt(*val));
        },
        AST::Float(_) => todo!(),
        AST::Bool(val) => {
            code.add(current_function, Bytecode::PushBool(*val));
        },
        AST::NoneVal => unimplemented!(), // TODO: Think about if we really want null values, some
                                          // kind of Optional class would probably be better.
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
        } => {
            let new_fun_idx = context.constant_pool.add(Object::from(name.clone()));
            code.add_function(new_fun_idx);

            let fun = Object::Function { name: new_fun_idx, parameters_cnt: todo!(), locals: todo!(), range: todo!() };
            context.constant_pool.add(fun);
            // TODO: Also introduce parameters to local environment
            _compile(body, code, context, new_fun_idx, true)?;
        },
        AST::CallFunction { name, arguments } => {
            for arg in arguments {
                _compile(arg, code, context, current_function, false)?;
            }

            let str_index: ConstantPoolIndex = context
                .constant_pool
                .add(Object::from(name.clone()))
                .try_into()
                .expect("Constant pool is full");
            code.add(current_function, Bytecode::CallFunc {
                index: str_index,
                arg_cnt: arguments.len().try_into().unwrap(),
            });
        }
        AST::Top(stmts) => {
            for stmt in stmts {
                _compile(stmt, code, context, current_function, true)?;
            }
        }
        AST::Block(stmts) => {
            let mut it = stmts.iter().peekable();

            while let Some(stmt) = it.next() {
                // Drop everything but the last result
                _compile(stmt, code, context, current_function, !it.peek().is_none())?;
            }
        },
        AST::While { guard, body } => todo!(),
        AST::Conditional {
            guard,
            then_branch,
            else_branch,
        } => {
            _compile(guard, code, context, current_function, false)?;
            // True is fallthrough, false is jump
            code.add(current_function, Bytecode::BranchLabelFalse(String::from("if_false")));
            _compile(&then_branch, code, context, current_function, drop)?;
            code.add(current_function, Bytecode::Label(String::from("if_false")));
            if let Some(else_body) = else_branch {
                _compile(&else_body, code, context, current_function, false)?;
            }
        },
        AST::Operator { op, arguments } => {
            check_operator_arity(op, arguments.len())?;
            for arg in arguments {
                _compile(arg, code, context, current_function, false)?;
            }
            match op {
                Opcode::Add => code.add(current_function, Bytecode::Iadd),
                Opcode::Sub => code.add(current_function, Bytecode::Isub),
                Opcode::Mul => code.add(current_function, Bytecode::Imul),
                Opcode::Div => code.add(current_function, Bytecode::Idiv),
                Opcode::Less => code.add(current_function, Bytecode::Iless),
                Opcode::LessEq => code.add(current_function, Bytecode::Ilesseq),
                Opcode::Greater => code.add(current_function, Bytecode::Igreater),
                Opcode::GreaterEq => code.add(current_function, Bytecode::Igreatereq),
                Opcode::Eq => code.add(current_function, Bytecode::Ieq),
            };
        }
        AST::Return(_) => todo!(),
        AST::String(lit) => {
            let str_index: ConstantPoolIndex = context
                .constant_pool
                .add(Object::from(lit.clone()))
                .try_into()
                .expect("Constant pool is full");
            code.add(current_function, Bytecode::PushLiteral(str_index));
        }
    };
    code.add_cond(current_function, Bytecode::Drop, drop);
    Ok(())
}

pub fn compile(ast: &AST) -> Result<Program, &'static str> {
    let mut context = Context::new();
    let mut code = Code::new();

    let main_fun : ConstantPoolIndex = context.constant_pool.add(Object::from(String::from("#main"))).try_into().unwrap();

    code.add_function(main_fun);
    _compile(ast, &mut code, &mut context, main_fun, false)?;

    code.code = code.code.into_iter().map(|(idx, body)| (idx, jump_pass(body))).collect();

    Ok(Program { code, constant_pool: context.constant_pool })
}
