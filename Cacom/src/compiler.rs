use std::collections::HashMap;
use std::fmt;

use crate::ast::{Expr, Opcode, AST};
use crate::bytecode::{Bytecode, Code, ConstantPoolIndex, FrameIndex};
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
    stack_idx: FrameIndex,
}

pub struct Program {
    code: Code,
}

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

    pub fn add_local(&mut self, v: String, idx: FrameIndex) -> Result<(), &'static str> {
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
        Ok(())
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
            stack_idx: 0,
        }
    }

    pub fn inc_depth(&mut self) {
        self.stack_idx += 1;
    }

    pub fn decrease_depth(&mut self) {
        assert!(self.stack_idx > 0);
        self.stack_idx -= 1;
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

fn compile_statements(
    stmts: &[AST],
    code: &mut Code,
    context: &mut Context,
    constant_pool: &mut ConstantPool,
    drop: bool,
) -> Result<(), &'static str> {
    let mut it = stmts.iter().peekable();
    while let Some(stmt) = it.next() {
        // Drop everything but the last result
        // Hovewer if it is statement, do NOT drop it because it does not
        // produces any value. Hovewer, if it is the last value, the block
        // returns none.
        match stmt {
            // Never drop the last value, if it should be dropped then
            // not here, but the parent of this block
            AST::Expression(expr) => {
                compile_expr(expr, code, context, constant_pool, it.peek().is_some())?
            }
            _ => {
                _compile(stmt, code, context, constant_pool, false)?;
                if it.peek().is_none() {
                    code.add(Bytecode::PushNone, context);
                }
            }
        }
    }
    Ok(())
}

fn compile_expr(
    expr: &Expr,
    code: &mut Code,
    context: &mut Context,
    constant_pool: &mut ConstantPool,
    drop: bool,
) -> Result<(), &'static str> {
    match expr {
        Expr::Integer(val) => {
            code.add(Bytecode::PushInt(*val), context);
        }
        Expr::Float(_) => todo!(),
        Expr::Bool(val) => {
            code.add(Bytecode::PushBool(*val), context);
        }
        Expr::NoneVal => {
            code.add(Bytecode::PushNone, context);
        }
        Expr::String(lit) => {
            let str_index: ConstantPoolIndex = constant_pool.add(Object::from(lit.clone()));
            code.add(Bytecode::PushLiteral(str_index), context);
        }
        Expr::Block(stmts) => {
            // match &mut context.loc {
            //     Location::Global => {
            //         context.loc = Location::Local(Environment::new());
            //     },
            //     Location::Local(env) => {
            //         env.enter_scope();
            //     },
            // }
            compile_statements(stmts, code, context, constant_pool, drop)?;
            // match &mut context.loc {
            //     Location::Global => {
            //         unreachable!("Can't leave global environment!");
            //     },
            //     Location::Local(env) => {
            //         if (env.envs.len() == 1) {
            //             context.loc = Location::Global;
            //         }
            //         env.leave_scope();
            //     },
            // }
        }
        Expr::List { size, values } => todo!(),
        Expr::AccessVariable { name } => {
            // TODO: Repeated code!
            if let Location::Local(env) = &mut context.loc {
                if let Some(v) = env.fetch_local(name) {
                    code.add(Bytecode::GetLocal(v), context);
                } else {
                    let idx = constant_pool.add(Object::from(name.clone()));
                    code.add(Bytecode::GetGlobal(idx), context);
                }
            } else {
                let idx = constant_pool.add(Object::from(name.clone()));
                code.add(Bytecode::GetGlobal(idx), context);
            }
        }
        Expr::AccessList { list, index } => todo!(),
        Expr::CallFunction { name, arguments } => {
            for arg in arguments.iter().rev() {
                compile_expr(arg, code, context, constant_pool, false)?;
            }
            // TODO: For nested functions we first need to look into environments, then do this.
            // TODO: Hardcoded native print
            if name == "print" {
                code.add(
                    Bytecode::Print {
                        arg_cnt: arguments.len().try_into().unwrap(),
                    },
                    context,
                );
            } else {
                let str_index: ConstantPoolIndex = constant_pool.add(Object::from(name.clone()));
                code.add(
                    Bytecode::CallFunc {
                        index: str_index,
                        arg_cnt: arguments.len().try_into().unwrap(),
                    },
                    context,
                );
            }
        }
        Expr::Conditional {
            guard,
            then_branch,
            else_branch,
        } => {
            let label_else = context.counter.get_label("if_else");
            let label_end = context.counter.get_label("if_merge");
            compile_expr(guard, code, context, constant_pool, false)?;
            code.add(Bytecode::BranchLabelFalse(label_else.clone()), context);
            compile_expr(then_branch, code, context, constant_pool, drop)?;
            code.add(Bytecode::JmpLabel(label_end.clone()), context);
            code.add(Bytecode::Label(label_else), context);
            if let Some(else_body) = else_branch {
                compile_expr(else_body, code, context, constant_pool, drop)?;
            } else if !drop {
                code.add(Bytecode::PushNone, context);
            }
            code.add(Bytecode::Label(label_end), context);
        }
        Expr::Operator { op, arguments } => {
            check_operator_arity(op, arguments.len())?;
            for arg in arguments.iter().rev() {
                compile_expr(arg, code, context, constant_pool, false)?;
            }
            match op {
                Opcode::Add => code.add(Bytecode::Iadd, context),
                Opcode::Sub => code.add(Bytecode::Isub, context),
                Opcode::Mul => code.add(Bytecode::Imul, context),
                Opcode::Div => code.add(Bytecode::Idiv, context),
                Opcode::Less => code.add(Bytecode::Iless, context),
                Opcode::LessEq => code.add(Bytecode::Ilesseq, context),
                Opcode::Greater => code.add(Bytecode::Igreater, context),
                Opcode::GreaterEq => code.add(Bytecode::Igreatereq, context),
                Opcode::Eq => code.add(Bytecode::Ieq, context),
            };
        }
    }
    code.add_cond(Bytecode::Drop, drop, context);
    Ok(())
}

/// Inner compile function, compile the AST into bytecode which is inserted into Code.
/// If the result is not intended to be keeped on stack (param drop is true) then
/// a Drop instruction will be generated at the end.
fn _compile(
    ast: &AST,
    code: &mut Code,
    context: &mut Context,
    constant_pool: &mut ConstantPool,
    drop: bool,
) -> Result<(), &'static str> {
    match ast {
        AST::Variable {
            name,
            mutable,
            value,
        } => match &mut context.loc {
            Location::Global => {
                compile_expr(value, code, context, constant_pool, false)?;
                if *mutable {
                    code.add(
                        Bytecode::DeclVarGlobal {
                            name: constant_pool.add(Object::from(name.clone())),
                        },
                        context,
                    );
                } else {
                    code.add(
                        Bytecode::DeclValGlobal {
                            name: constant_pool.add(Object::from(name.clone())),
                        },
                        context,
                    );
                }
            }
            Location::Local(env) => {
                env.add_local(name.clone(), context.stack_idx)?;
            }
        },
        AST::AssignVariable { name, value } => {
            compile_expr(value, code, context, constant_pool, false)?;
            // TODO: Repeated code!
            if let Location::Local(env) = &mut context.loc {
                if let Some(v) = env.fetch_local(name) {
                    code.add(Bytecode::SetLocal(v), context);
                } else {
                    let idx = constant_pool.add(Object::from(name.clone()));
                    code.add(Bytecode::SetGlobal(idx), context);
                }
            } else {
                let idx = constant_pool.add(Object::from(name.clone()));
                code.add(Bytecode::SetGlobal(idx), context);
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
                new_env.add_local(par.clone(), context.stack_idx)?;
            }
            let code = compile_fun(body, constant_pool, Location::Local(new_env))?;

            let fun = Object::Function {
                name: new_fun_idx,
                parameters_cnt: parameters.len().try_into().unwrap(),
                body: code,
            };

            let fun_idx = constant_pool.add(fun);

            match &mut context.loc {
                Location::Global => {
                    todo!();
                }
                Location::Local(env) => {
                    todo!("Nested functions are not yet implemented");
                }
            };
        }
        AST::Top(stmts) => compile_statements(stmts, code, context, constant_pool, drop)?,
        AST::While { guard, body } => todo!(),
        AST::Return(_) => todo!(),
        AST::Expression(expr) => compile_expr(expr, code, context, constant_pool, true)?,
    };
    code.add_cond(Bytecode::Drop, drop, context);
    Ok(())
}

fn compile_fun(
    ast: &Expr,
    constant_pool: &mut ConstantPool,
    loc: Location,
) -> Result<Code, &'static str> {
    let mut context = Context {
        loc,
        counter: LabelGenerator::new(),
        stack_idx: 0,
    };
    let mut code = Code::new();

    compile_expr(ast, &mut code, &mut context, constant_pool, false)?;
    if code.code.is_empty() {
        code.add(Bytecode::PushNone, &mut context);
    }
    // Return last statement if return is omitted
    if !matches!(code.code.last().unwrap(), Bytecode::Ret) {
        code.add(Bytecode::Ret, &mut context);
    }
    Ok(code)
}

/// Compiles AST into constant pool and returns tuple (constant pool, entry point, globals)
pub fn compile(ast: &AST) -> Result<(ConstantPool, ConstantPoolIndex), &'static str> {
    let mut constant_pool = ConstantPool::new();
    let idx = constant_pool.add(Object::from(String::from("#main")));

    let mut code = Code::new();
    let mut context = Context {
        loc: Location::Global,
        counter: LabelGenerator::new(),
        stack_idx: 0,
    };
    match ast {
        AST::Top(_) => {
            // TODO: Should this really be true? Maybe return last value as program code
            _compile(ast, &mut code, &mut context, &mut constant_pool, true)?;
            code.add(Bytecode::Ret, &mut context); // Add top level retu, contextrn
        }
        _ => unreachable!(),
    }

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

    Ok((constant_pool, main_fun_idx))
}
