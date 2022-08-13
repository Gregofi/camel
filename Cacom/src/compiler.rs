use std::collections::HashMap;
use std::fmt;

use crate::ast::{Expr, Opcode, AST};
use crate::bytecode::{Bytecode, Code, ConstantPoolIndex, LocalIndex};
use crate::objects::{ConstantPool, Object};
use crate::utils::{AtomicInt, LabelGenerator};

#[derive(Clone, Copy)]
struct Local {
    idx: LocalIndex,
    mutable: bool,
}

/// Environment keeps track of indexes of local variables into memory.
/// It consists of multiple environments, each one is a map from
/// name to index of the local variable.
struct Environment {
    envs: Vec<HashMap<String, Local>>,
}

enum Location {
    Global,
    Local(Environment),
}

impl Environment {
    /// Returns environment with one empty environment present
    pub fn new() -> Self {
        Self {
            envs: vec![HashMap::new()],
        }
    }

    /// Adds one more inner scope to environment
    pub fn enter_scope(&mut self) {
        self.envs.push(HashMap::new());
    }

    /// Removes the topmost environment and returns number of local variables in that environment
    pub fn leave_scope(&mut self) -> usize {
        let size = self.envs.last().unwrap().len();
        self.envs.pop();
        size
    }

    pub fn add_local(
        &mut self,
        v: String,
        idx: LocalIndex,
        mutable: bool,
    ) -> Result<(), &'static str> {
        let topmost = self
            .envs
            .last_mut()
            .expect("Camel Compiler bug: There is no environment");
        if topmost.contains_key(&v) {
            return Err("Redefinition of local variable");
        }
        topmost.insert(v, Local { idx, mutable });
        Ok(())
    }

    /// Traverses through environments, starting from the topmost
    /// If the local is not defined in any of them returns None.
    pub fn fetch_local(&self, v: &String) -> Option<Local> {
        for env in self.envs.iter().rev() {
            if let Some(v) = env.get(v) {
                return Some(*v);
            }
        }
        None
    }
}

struct Compiler {
    constant_pool: ConstantPool,
    location: Location,
    label_generator: LabelGenerator,
    local_count: LocalIndex,
}

impl Compiler {
    fn new() -> Self {
        Self {
            constant_pool: ConstantPool::new(),
            location: Location::Global,
            label_generator: LabelGenerator::new(),
            local_count: 0,
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
                let var_cnt: u16 = env.leave_scope().try_into().unwrap();
                if env.envs.is_empty() {
                    self.location = Location::Global;
                }
                self.local_count -= var_cnt;
            }
        };
    }

    fn add_instruction(&mut self, code: &mut Code, ins: Bytecode) {
        // match &ins {
        //     Bytecode::PushShort(_)
        //     | Bytecode::PushInt(_)
        //     | Bytecode::PushLong(_)
        //     | Bytecode::PushBool(_)
        //     | Bytecode::PushLiteral(_)
        //     | Bytecode::PushNone
        //     | Bytecode::GetGlobal(_)
        //     | Bytecode::Dup => self.stack_offset += 1,
        //     Bytecode::GetLocal(_)
        //     | Bytecode::SetLocal(_)
        //     | Bytecode::Label(_)
        //     | Bytecode::JmpLabel(_)
        //     | Bytecode::JmpShort(_)
        //     | Bytecode::Jmp(_)
        //     | Bytecode::DeclValGlobal {..}
        //     | Bytecode::DeclVarGlobal {..}
        //     | Bytecode::JmpLong(_) => (),
        //     Bytecode::CallFunc { index, arg_cnt } => todo!(),
        //     Bytecode::SetGlobal(_)
        //     | Bytecode::Ret
        //     | Bytecode::BranchLabel(_)
        //     | Bytecode::BranchLabelFalse(_)
        //     | Bytecode::BranchShort(_)
        //     | Bytecode::Branch(_)
        //     | Bytecode::BranchLong(_)
        //     | Bytecode::BranchShortFalse(_)
        //     | Bytecode::BranchFalse(_)
        //     | Bytecode::BranchLongFalse(_)
        //     | Bytecode::Drop => self.stack_offset -= 1,
        //     Bytecode::Iadd
        //     | Bytecode::Isub
        //     | Bytecode::Imul
        //     | Bytecode::Idiv
        //     | Bytecode::Iand
        //     | Bytecode::Ior
        //     | Bytecode::Iless
        //     | Bytecode::Ilesseq
        //     | Bytecode::Igreater
        //     | Bytecode::Igreatereq
        //     | Bytecode::Ieq => self.stack_offset -= 2,
        //     Bytecode::Print { arg_cnt } => self.stack_offset -= *arg_cnt as u16,
        //     Bytecode::Dropn(cnt) => self.stack_offset -= *cnt as u16,
        // };
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
                self.add_instruction(code, Bytecode::PushInt(*val));
            }
            Expr::Float(_) => todo!(),
            Expr::Bool(val) => {
                self.add_instruction(code, Bytecode::PushBool(*val));
            }
            Expr::NoneVal => {
                self.add_instruction(code, Bytecode::PushNone);
            }
            Expr::String(lit) => {
                let str_index: ConstantPoolIndex =
                    self.constant_pool.add(Object::from(lit.clone()));
                self.add_instruction(code, Bytecode::PushLiteral(str_index));
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
                        self.add_instruction(code, Bytecode::GetLocal(v.idx));
                    } else {
                        let idx = self.constant_pool.add(Object::from(name.clone()));
                        self.add_instruction(code, Bytecode::GetGlobal(idx));
                    }
                } else {
                    let idx = self.constant_pool.add(Object::from(name.clone()));
                    self.add_instruction(code, Bytecode::GetGlobal(idx));
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
                    self.add_instruction(
                        code,
                        Bytecode::Print {
                            arg_cnt: arguments.len().try_into().unwrap(),
                        },
                    );
                    self.add_instruction(code, Bytecode::PushNone);
                } else {
                    let str_index: ConstantPoolIndex =
                        self.constant_pool.add(Object::from(name.clone()));
                    self.add_instruction(
                        code,
                        Bytecode::CallFunc {
                            index: str_index,
                            arg_cnt: arguments.len().try_into().unwrap(),
                        },
                    );
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
                self.add_instruction(code, Bytecode::BranchLabelFalse(label_else.clone()));
                self.compile_expr(then_branch, code, drop)?;
                self.add_instruction(code, Bytecode::JmpLabel(label_end.clone()));
                self.add_instruction(code, Bytecode::Label(label_else));
                if let Some(else_body) = else_branch {
                    self.compile_expr(else_body, code, drop)?;
                } else if !drop {
                    self.add_instruction(code, Bytecode::PushNone);
                }
                self.add_instruction(code, Bytecode::Label(label_end));
            }
            Expr::Operator { op, arguments } => {
                check_operator_arity(op, arguments.len())?;
                for arg in arguments.iter().rev() {
                    self.compile_expr(arg, code, false)?;
                }
                self.add_instruction(code, (*op).into());
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
                        self.add_instruction(code, Bytecode::PushNone);
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
                        let name_idx = self.constant_pool.add(Object::from(name.clone()));
                        if *mutable {
                            self.add_instruction(code, Bytecode::DeclVarGlobal { name: name_idx });
                        } else {
                            self.add_instruction(code, Bytecode::DeclValGlobal { name: name_idx });
                        }
                    }
                    Location::Local(env) => {
                        env.add_local(name.clone(), self.local_count, *mutable)?;
                        self.add_instruction(code, Bytecode::SetLocal(self.local_count));
                        self.local_count += 1;
                    }
                }
            }
            AST::AssignVariable { name, value } => {
                self.compile_expr(value, code, false)?;
                // TODO: Repeated code!
                if let Location::Local(env) = &mut self.location {
                    if let Some(Local { idx, mutable }) = env.fetch_local(name) {
                        if !mutable {
                            return Err("Variable is declared immutable.");
                        }
                        self.add_instruction(code, Bytecode::SetLocal(idx));
                    } else {
                        let idx = self.constant_pool.add(Object::from(name.clone()));
                        self.add_instruction(code, Bytecode::SetGlobal(idx));
                    }
                } else {
                    let idx = self.constant_pool.add(Object::from(name.clone()));
                    self.add_instruction(code, Bytecode::SetGlobal(idx));
                }
            }
            AST::AssignList { list, index, value } => todo!(),
            AST::Function {
                name,
                parameters,
                body,
            } => {
                // let locals_backup = self.local_count;
                // self.local_count = 0;
                let new_fun_idx = self.constant_pool.add(Object::from(name.clone()));

                // Create new env and populate it with parameters
                let mut new_env = Environment::new();
                for (idx, par) in parameters.iter().enumerate() {
                    new_env.add_local(par.clone(), self.local_count, false)?;
                    self.add_instruction(code, Bytecode::SetLocal(idx.try_into().unwrap()));
                    self.local_count += 1;
                }
                let code = self.compile_fun(body)?;

                let fun = Object::Function {
                    name: new_fun_idx,
                    parameters_cnt: parameters.len().try_into().unwrap(),
                    locals_cnt: self.local_count,
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

                // self.local_count = locals_backup;
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
            self.add_instruction(&mut code, Bytecode::PushNone);
        }
        // Return last statement if return is omitted
        if !matches!(code.code.last().unwrap(), Bytecode::Ret) {
            self.add_instruction(&mut code, Bytecode::Ret);
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
        Opcode::Negate => 1,
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
            compiler.add_instruction(&mut code, Bytecode::Ret); // Add top level return
        }
        _ => unreachable!(),
    }

    let main_fun = Object::Function {
        name: idx,
        parameters_cnt: 0,
        locals_cnt: compiler.local_count,
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
                locals_cnt,
            } => Object::Function {
                body: Code {
                    code: jump_pass(body.code),
                },
                name,
                parameters_cnt,
                locals_cnt,
            },
            _ => f,
        })
        .collect();

    Ok((compiler.constant_pool, main_fun_idx))
}
