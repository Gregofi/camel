use std::collections::HashMap;
use std::fmt;

use crate::ast::{Expr, Opcode, AST};
use crate::bytecode::{Bytecode, Code, ConstantPoolIndex, LocalIndex};
use crate::objects::{ConstantPool, Object};
use crate::utils::{AtomicInt, LabelGenerator};

/// Environment keeps track of indexes of local variables into memory.
/// It consists of multiple environments, each one is a map from
/// name to index of the local variable.
struct Environment {
    atomic_int: AtomicInt,
    envs: Vec<HashMap<String, LocalIndex>>,
}

enum Location {
    Global,
    Local(Environment),
}

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

    pub fn add_local(&mut self, v: String, idx: LocalIndex) -> Result<(), &'static str> {
        let topmost = self
            .envs
            .last_mut()
            .expect("Camel Compiler bug: There is no environment");
        if topmost.contains_key(&v) {
            return Err("Redefinition of local variable");
        }
        let idx: LocalIndex = self
            .atomic_int
            .get_and_inc()
            .try_into()
            .expect("Too many local variables");
        topmost.insert(v, idx);
        Ok(())
    }

    pub fn fetch_local(&self, v: &String) -> Option<LocalIndex> {
        let topmost = self
            .envs
            .last()
            .expect("Camel compiler bug: There is no environment");
        topmost.get(v).copied()
    }
}

struct Compiler {
    constant_pool: ConstantPool,
    location: Location,
    stack_offset: LocalIndex,
    label_generator: LabelGenerator,
}

impl Compiler {
    fn new() -> Self {
        Self {
            constant_pool: ConstantPool::new(),
            location: Location::Global,
            stack_offset: 0,
            label_generator: LabelGenerator::new(),
        }
    }

    fn enter_scope(&mut self) {
        match &mut self.location {
            Location::Global => {
                self.location = Location::Local(Environment::new());
            }
            Location::Local(env) => {
                env.enter_scope();
            }
        };
    }

    fn leave_scope(&mut self) {
        match &mut self.location {
            Location::Global => {
                unreachable!("Internal compiler error: Can't leave scope at global environment");
            }
            Location::Local(env) => {
                env.leave_scope();
                if env.envs.is_empty() {
                    self.location = Location::Global;
                }
            }
        };
    }

    fn add_instruction(&mut self, code: &mut Code, ins: Bytecode) {
        match &ins {
            Bytecode::PushShort(_)
            | Bytecode::PushInt(_)
            | Bytecode::PushLong(_)
            | Bytecode::PushBool(_)
            | Bytecode::PushLiteral(_)
            | Bytecode::PushNone => self.stack_offset += 1,
            Bytecode::Drop => self.stack_offset -= 1,
            _ => (),
        };
        code.add(ins);
    }

    fn compile_expr(
        &mut self,
        expr: &Expr,
        code: &mut Code,
        drop: bool,
    ) -> Result<(), &'static str> {
        match expr {
            Expr::Integer(val) => {
                code.add(Bytecode::PushInt(*val));
            }
            Expr::Float(_) => todo!(),
            Expr::Bool(val) => {
                code.add(Bytecode::PushBool(*val));
            }
            Expr::NoneVal => {
                code.add(Bytecode::PushNone);
            }
            Expr::String(lit) => {
                let str_index: ConstantPoolIndex =
                    self.constant_pool.add(Object::from(lit.clone()));
                code.add(Bytecode::PushLiteral(str_index));
            }
            Expr::Block(stmts) => {
                self.enter_scope();
                self.compile_block(stmts, code)?;
                self.leave_scope();
            }
            Expr::List { size, values } => todo!(),
            Expr::AccessVariable { name } => {
                // TODO: Repeated code!
                if let Location::Local(env) = &mut self.location {
                    if let Some(v) = env.fetch_local(name) {
                        code.add(Bytecode::GetLocal(v));
                    } else {
                        let idx = self.constant_pool.add(Object::from(name.clone()));
                        code.add(Bytecode::GetGlobal(idx));
                    }
                } else {
                    let idx = self.constant_pool.add(Object::from(name.clone()));
                    code.add(Bytecode::GetGlobal(idx));
                }
            }
            Expr::AccessList { list, index } => todo!(),
            Expr::CallFunction { name, arguments } => {
                for arg in arguments.iter().rev() {
                    self.compile_expr(arg, code, false)?;
                }
                // TODO: For nested functions we first need to look into environments, then do this.
                // TODO: Hardcoded native print
                if name == "print" {
                    code.add(Bytecode::Print {
                        arg_cnt: arguments.len().try_into().unwrap(),
                    });
                } else {
                    let str_index: ConstantPoolIndex =
                        self.constant_pool.add(Object::from(name.clone()));
                    code.add(Bytecode::CallFunc {
                        index: str_index,
                        arg_cnt: arguments.len().try_into().unwrap(),
                    });
                }
            }
            Expr::Conditional {
                guard,
                then_branch,
                else_branch,
            } => {
                let label_else = self.label_generator.get_label("if_else");
                let label_end = self.label_generator.get_label("if_merge");
                self.compile_expr(guard, code, false)?;
                code.add(Bytecode::BranchLabelFalse(label_else.clone()));
                self.compile_expr(then_branch, code, drop)?;
                code.add(Bytecode::JmpLabel(label_end.clone()));
                code.add(Bytecode::Label(label_else));
                if let Some(else_body) = else_branch {
                    self.compile_expr(else_body, code, drop)?;
                } else if !drop {
                    code.add(Bytecode::PushNone);
                }
                code.add(Bytecode::Label(label_end));
            }
            Expr::Operator { op, arguments } => {
                check_operator_arity(op, arguments.len())?;
                for arg in arguments.iter().rev() {
                    self.compile_expr(arg, code, false)?;
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
        }
        code.add_cond(Bytecode::Drop, drop);
        Ok(())
    }

    fn compile_block(&mut self, stmts: &[AST], code: &mut Code) -> Result<(), &'static str> {
        let mut it = stmts.iter().peekable();
        while let Some(stmt) = it.next() {
            // Drop everything but the last result
            // Hovewer if it is statement, do NOT drop it because it does not
            // produces any value. Hovewer, if it is the last value, the block
            // returns none.
            match stmt {
                // Never drop the last value, if it should be dropped then
                // not here, but the parent of this block will drop it
                AST::Expression(expr) => self.compile_expr(expr, code, it.peek().is_some())?,
                _ => {
                    self.compile_stmt(stmt, code)?;
                    if it.peek().is_none() {
                        code.add(Bytecode::PushNone);
                    }
                }
            }
        }

        Ok(())
    }

    fn compile_stmt(&mut self, ast: &AST, code: &mut Code) -> Result<(), &'static str> {
        match ast {
            AST::Variable {
                name,
                mutable,
                value,
            } => {
                self.compile_expr(value, code, false)?;
                match &mut self.location {
                    Location::Global => {
                        if *mutable {
                            code.add(Bytecode::DeclVarGlobal {
                                name: self.constant_pool.add(Object::from(name.clone())),
                            });
                        } else {
                            code.add(Bytecode::DeclValGlobal {
                                name: self.constant_pool.add(Object::from(name.clone())),
                            });
                        }
                    }
                    Location::Local(env) => {
                        env.add_local(name.clone(), self.stack_offset)?;
                        code.add(Bytecode::SetLocal(self.stack_offset));
                    }
                }
            }
            AST::AssignVariable { name, value } => {
                self.compile_expr(value, code, false)?;
                // TODO: Repeated code!
                if let Location::Local(env) = &mut self.location {
                    if let Some(v) = env.fetch_local(name) {
                        code.add(Bytecode::SetLocal(v));
                    } else {
                        let idx = self.constant_pool.add(Object::from(name.clone()));
                        code.add(Bytecode::SetGlobal(idx));
                    }
                } else {
                    let idx = self.constant_pool.add(Object::from(name.clone()));
                    code.add(Bytecode::SetGlobal(idx));
                }
            }
            AST::AssignList { list, index, value } => todo!(),
            AST::Function {
                name,
                parameters,
                body,
            } => {
                let new_fun_idx = self.constant_pool.add(Object::from(name.clone()));

                // Create new env and populate it with parameters
                let mut new_env = Environment::new();
                for par in parameters {
                    new_env.add_local(par.clone(), self.stack_offset)?;
                }
                let code = self.compile_fun(body)?;

                let fun = Object::Function {
                    name: new_fun_idx,
                    parameters_cnt: parameters.len().try_into().unwrap(),
                    body: code,
                };

                let fun_idx = self.constant_pool.add(fun);

                match &mut self.location {
                    Location::Global => {
                        todo!();
                    }
                    Location::Local(env) => {
                        todo!("Nested functions are not yet implemented");
                    }
                };
            }
            AST::Top(stmts) => self.compile_block(stmts, code)?,
            AST::While { guard, body } => todo!(),
            AST::Return(_) => todo!(),
            AST::Expression(expr) => self.compile_expr(expr, code, true)?,
        };
        Ok(())
    }

    fn compile_fun(&mut self, ast: &Expr) -> Result<Code, &'static str> {
        let mut code = Code::new();

        self.compile_expr(ast, &mut code, false)?;
        if code.code.is_empty() {
            code.add(Bytecode::PushNone);
        }
        // Return last statement if return is omitted
        if !matches!(code.code.last().unwrap(), Bytecode::Ret) {
            code.add(Bytecode::Ret);
        }
        Ok(code)
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

/// Compiles AST into constant pool and returns tuple (constant pool, entry point, globals)
pub fn compile(ast: &AST) -> Result<(ConstantPool, ConstantPoolIndex), &'static str> {
    let mut compiler = Compiler::new();
    let idx = compiler
        .constant_pool
        .add(Object::from(String::from("#main")));
    let mut code = Code::new();

    match ast {
        AST::Top(_) => {
            compiler.compile_stmt(ast, &mut code)?;
            code.add(Bytecode::Ret); // Add top level return
        }
        _ => unreachable!(),
    }

    let main_fun = Object::Function {
        name: idx,
        parameters_cnt: 0,
        body: code,
    };
    let main_fun_idx = compiler.constant_pool.add(main_fun);

    compiler.constant_pool.data = compiler
        .constant_pool
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

    Ok((compiler.constant_pool, main_fun_idx))
}
