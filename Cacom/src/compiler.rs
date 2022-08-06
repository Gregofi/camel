use std::collections::HashMap;
use std::fmt;

use crate::ast::{Opcode, AST};
use crate::bytecode::{Bytecode, Code, ConstantPoolIndex};
use crate::objects::{ConstantPool, Object};
use crate::serializable::Serializable;
use crate::utils::{AtomicInt, LabelGenerator};

/// Environment keeps track of indexes of local variables into memory.
/// It consists of multiple environments, each one is a map from
/// name to index of the local variable.
struct Environment {
    atomic_int: AtomicInt,
    envs: Vec<HashMap<String, LocalVarIndex>>,
}

enum Location {
    Global,
    Local(Environment),
}

pub struct Context {
    loc: Location,
    counter: LabelGenerator,
}

pub struct Program {
    code: Code,
}

type Globals = HashMap<String, ConstantPoolIndex>;

type LocalVarIndex = u16;

impl Environment {
    /// Returns environment with one empty environment present
    pub fn new() -> Self {
        Self {
            atomic_int: AtomicInt::new(),
            envs: vec![HashMap::new()],
        }
    }

    /// Adds one more inner scope to environment
    pub fn enter_scope(&mut self) {
        self.envs.push(HashMap::new());
    }

    /// Removes the topmost environment
    pub fn leave_scope(&mut self) {
        self.envs.pop();
    }

    pub fn add_local(&mut self, v: String) -> Result<LocalVarIndex, &'static str> {
        let topmost = self
            .envs
            .last_mut()
            .expect("Camel Compiler bug: There is no environment");
        if topmost.contains_key(&v) {
            return Err("Redefinition of local variable");
        }
        let idx: LocalVarIndex = self
            .atomic_int
            .get_and_inc()
            .try_into()
            .expect("Too many local variables");
        topmost.insert(v, idx);
        Ok(idx)
    }

    pub fn fetch_local(&self, v: &String) -> Option<LocalVarIndex> {
        let topmost = self
            .envs
            .last()
            .expect("Camel compiler bug: There is no environment");
        topmost.get(v).copied()
    }
}

impl Context {
    pub fn new() -> Self {
        Context {
            loc: Location::Global,
            counter: LabelGenerator::new(),
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
        // self.constant_pool.fmt(f)?;
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
/// TODO: Currently, the computed offset takes all jump instructions
/// as 4B, this is not necessarily true if we use short and long jmps.
fn jump_pass(code: Vec<Bytecode>) -> Vec<Bytecode> {
    let mut labels: HashMap<String, usize> = HashMap::new();

    let mut offset: usize = 0;
    // Copy the bytecode without labels but store their (labels) location
    let mut without_labels: Vec<Bytecode> = Vec::new();
    for ins in code {
        let ins_size = ins.size();
        match ins {
            Bytecode::Label(str) => {
                labels.insert(str, offset);
            }
            _ => without_labels.push(ins),
        }
        offset += ins_size;
    }

    // Remove the jump to labels with jump to address
    // TODO: Use short, long or normal jump based on distance
    without_labels
        .into_iter()
        .map(|ins| match ins {
            Bytecode::JmpLabel(label) => {
                let label_index = *labels.get(&label).unwrap();
                Bytecode::Jmp(label_index.try_into().unwrap())
            }
            Bytecode::BranchLabel(label) => {
                let label_index = *labels.get(&label).unwrap();
                Bytecode::Branch(label_index.try_into().unwrap())
            }
            Bytecode::BranchLabelFalse(label) => {
                let label_index = *labels.get(&label).unwrap();
                Bytecode::BranchFalse(label_index.try_into().unwrap())
            }
            _ => ins,
        })
        .collect()
}

/// Inner compile function, compile the AST into bytecode which is inserted into Code.
/// If the result is not intended to be keeped on stack (param drop is true) then
/// a Drop instruction will be generated at the end.
fn _compile(
    ast: &AST,
    code: &mut Code,
    context: &mut Context,
    constant_pool: &mut ConstantPool,
    globals: &mut HashMap<String, ConstantPoolIndex>,
    drop: bool,
) -> Result<(), &'static str> {
    match ast {
        AST::Integer(val) => {
            code.add(Bytecode::PushInt(*val));
        }
        AST::Float(_) => todo!(),
        AST::Bool(val) => {
            code.add(Bytecode::PushBool(*val));
        }
        AST::NoneVal => unimplemented!(), // TODO: Think about if we really want null values, some
        // kind of Optional class would probably be better.
        AST::Variable { name, value } => match &mut context.loc {
            Location::Global => {
                todo!("Global variables are not yet implemeted");
            }
            Location::Local(env) => {
                env.add_local(name.clone())?;
            }
        },
        AST::List { size, values } => todo!(),
        AST::AccessVariable { name } => {
            // TODO: Repeated code!
            if let Location::Local(env) = &mut context.loc {
                if let Some(v) = env.fetch_local(name) {
                    code.add(Bytecode::GetLocal(v));
                } else {
                    let idx = constant_pool.add(Object::from(name.clone()));
                    code.add(Bytecode::GetGlobal(idx));
                }
            } else {
                let idx = constant_pool.add(Object::from(name.clone()));
                code.add(Bytecode::GetGlobal(idx));
            }
        }
        AST::AccessList { list, index } => todo!(),
        AST::AssignVariable { name, value } => {
            _compile(value, code, context, constant_pool, globals, false)?;
            // TODO: Repeated code!
            if let Location::Local(env) = &mut context.loc {
                if let Some(v) = env.fetch_local(name) {
                    code.add(Bytecode::SetLocal(v));
                } else {
                    let idx = constant_pool.add(Object::from(name.clone()));
                    code.add(Bytecode::SetGlobal(idx));
                }
            } else {
                let idx = constant_pool.add(Object::from(name.clone()));
                code.add(Bytecode::SetGlobal(idx));
            }
        }
        AST::AssignList { list, index, value } => todo!(),
        AST::Function {
            name,
            parameters,
            body,
        } => {
            let new_fun_idx = constant_pool.add(Object::from(name.clone()));

            // Create new env and populate it with parameters
            let mut new_env = Environment::new();
            for par in parameters {
                new_env.add_local(par.clone())?;
            }
            let code = compile_fun(body, constant_pool, Location::Local(new_env), globals)?;

            let fun = Object::Function {
                name: new_fun_idx,
                parameters_cnt: parameters.len().try_into().unwrap(),
                body: code,
            };

            let fun_idx = constant_pool.add(fun);

            match &mut context.loc {
                Location::Global => {
                    globals.insert(name.clone(), fun_idx);
                }
                Location::Local(env) => {
                    todo!("Nested functions are not yet implemented");
                }
            };
        }
        AST::CallFunction { name, arguments } => {
            for arg in arguments.iter().rev() {
                _compile(arg, code, context, constant_pool, globals, false)?;
            }
            // TODO: For nested functions we first need to look into environments, then do this.
            // TODO: Hardcoded native print
            if name == "print" {
                code.add(Bytecode::Print {
                    arg_cnt: arguments.len().try_into().unwrap(),
                });
            } else {
                let str_index: ConstantPoolIndex = constant_pool.add(Object::from(name.clone()));
                code.add(Bytecode::CallFunc {
                    index: str_index,
                    arg_cnt: arguments.len().try_into().unwrap(),
                });
            }
        }
        AST::Top(stmts) => {
            for stmt in stmts {
                _compile(stmt, code, context, constant_pool, globals, true)?;
            }
        }
        AST::Block(stmts) => {
            let mut it = stmts.iter().peekable();

            while let Some(stmt) = it.next() {
                // Drop everything but the last result
                _compile(
                    stmt,
                    code,
                    context,
                    constant_pool,
                    globals,
                    it.peek().is_some(),
                )?;
            }
        }
        AST::While { guard, body } => todo!(),
        AST::Conditional {
            guard,
            then_branch,
            else_branch,
        } => {
            let label_else = context.counter.get_label("if_else");
            let label_end = context.counter.get_label("if_merge");
            _compile(guard, code, context, constant_pool, globals, false)?;
            code.add(Bytecode::BranchLabelFalse(label_else.clone()));
            _compile(then_branch, code, context, constant_pool, globals, drop)?;
            code.add(Bytecode::JmpLabel(label_end.clone()));
            code.add(Bytecode::Label(label_else));
            if let Some(else_body) = else_branch {
                _compile(else_body, code, context, constant_pool, globals, drop)?;
            } else if !drop {
                code.add(Bytecode::PushNone);
            }
            code.add(Bytecode::Label(label_end));
        }
        AST::Operator { op, arguments } => {
            check_operator_arity(op, arguments.len())?;
            for arg in arguments.iter().rev() {
                _compile(arg, code, context, constant_pool, globals, false)?;
            }
            match op {
                Opcode::Add => code.add(Bytecode::Iadd),
                Opcode::Sub => code.add(Bytecode::Isub),
                Opcode::Mul => code.add(Bytecode::Imul),
                Opcode::Div => code.add(Bytecode::Idiv),
                Opcode::Less => code.add(Bytecode::Iless),
                Opcode::LessEq => code.add(Bytecode::Ilesseq),
                Opcode::Greater => code.add(Bytecode::Igreater),
                Opcode::GreaterEq => code.add(Bytecode::Igreatereq),
                Opcode::Eq => code.add(Bytecode::Ieq),
            };
        }
        AST::Return(_) => todo!(),
        AST::String(lit) => {
            let str_index: ConstantPoolIndex = constant_pool.add(Object::from(lit.clone()));
            code.add(Bytecode::PushLiteral(str_index));
        }
    };
    code.add_cond(Bytecode::Drop, drop);
    Ok(())
}

fn compile_fun(
    ast: &AST,
    constant_pool: &mut ConstantPool,
    loc: Location,
    globals: &mut HashMap<String, ConstantPoolIndex>,
) -> Result<Code, &'static str> {
    let mut context = Context { loc, counter: LabelGenerator::new() };
    let mut code = Code::new();

    _compile(ast, &mut code, &mut context, constant_pool, globals, false)?;
    if code.code.is_empty() {
        code.add(Bytecode::PushNone);
    }
    // Return last statement if return is omitted
    if !matches!(code.code.last().unwrap(), Bytecode::Ret) {
        code.add(Bytecode::Ret);
    }
    Ok(code)
}

/// Compiles AST into constant pool and returns tuple (constant pool, entry point, globals)
pub fn compile(ast: &AST) -> Result<(ConstantPool, ConstantPoolIndex, Globals), &'static str> {
    let mut constant_pool = ConstantPool::new();
    let mut globals = Globals::new();
    let idx = constant_pool.add(Object::from(String::from("#main")));
    let code = compile_fun(ast, &mut constant_pool, Location::Global, &mut globals)?;

    let main_fun = Object::Function {
        name: idx,
        parameters_cnt: 0,
        body: code,
    };
    let main_fun_idx = constant_pool.add(main_fun);

    constant_pool.data = constant_pool
        .data
        .into_iter()
        .map(|f| match f {
            Object::Function {
                body,
                name,
                parameters_cnt,
            } => Object::Function {
                body: Code {
                    code: jump_pass(body.code),
                },
                name,
                parameters_cnt,
            },
            _ => f,
        })
        .collect();

    globals.insert(String::from("#main"), idx);

    Ok((constant_pool, main_fun_idx, globals))
}
