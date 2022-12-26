use std::collections::HashMap;
use std::fmt;

use lalrpop_util::state_machine;

use crate::ast::{Expr, ExprType, Opcode, Stmt, StmtType};
use crate::bytecode::{Bytecode, BytecodeType, Code, ConstantPoolIndex, LocalIndex};
use crate::objects::{ConstantPool, Object};
use crate::utils::Location as CodeLocation;
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
    Class {
        env: Environment,
        members: HashMap<String, Local>,
    }
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
    // Current local variables that are active
    local_count: LocalIndex,
    /// Tracks maximum number of local slots needed by function
    local_max: LocalIndex,
}

impl Compiler {
    fn new() -> Self {
        Self {
            constant_pool: ConstantPool::new(),
            location: Location::Global,
            label_generator: LabelGenerator::new(),
            local_count: 0,
            local_max: 0,
        }
    }

    fn add_locals(&mut self, cnt: LocalIndex) {
        self.local_count += cnt;
        self.local_max = std::cmp::max(self.local_count, self.local_max);
    }

    fn reset_locals(&mut self) -> (LocalIndex, LocalIndex) {
        let backup = (self.local_count, self.local_max);
        self.local_count = 0;
        self.local_max = 0;
        backup
    }

    fn restore_locals(&mut self, backup: (LocalIndex, LocalIndex)) {
        self.local_count = backup.0;
        self.local_max = backup.1;
    }

    fn enter_scope(&mut self) {
        match &mut self.location {
            Location::Global => {
                self.location = Location::Local(Environment::new());
            }
            Location::Local(env) => {
                env.enter_scope();
            }
            Location::Class { env, members } => {
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
            Location::Class { env, members } => {
                let var_cnt: u16 = env.leave_scope().try_into().unwrap();
                // We can't creep into global environment here
                self.local_count -= var_cnt;
            }
        };
    }

    fn add_instruction(&mut self, code: &mut Code, instr: BytecodeType, location: CodeLocation) {
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
        code.add(Bytecode { instr, location });
    }

    fn compile_expr(
        &mut self,
        expr: &Expr,
        code: &mut Code,
        drop: bool,
    ) -> Result<(), &'static str> {
        match &expr.node {
            ExprType::Integer(val) => {
                self.add_instruction(code, BytecodeType::PushInt(*val), expr.location);
            }
            ExprType::Float(_) => todo!(),
            ExprType::Bool(val) => {
                self.add_instruction(code, BytecodeType::PushBool(*val), expr.location);
            }
            ExprType::NoneVal => {
                self.add_instruction(code, BytecodeType::PushNone, expr.location);
            }
            ExprType::String(lit) => {
                let str_index: ConstantPoolIndex =
                    self.constant_pool.add(Object::from(lit.clone()));
                self.add_instruction(code, BytecodeType::PushLiteral(str_index), expr.location);
            }
            ExprType::Block(stmts, expr) => {
                self.enter_scope();
                self.compile_block(stmts, code)?;
                self.compile_expr(expr, code, false)?;
                self.leave_scope();
            }
            ExprType::List { size, values } => todo!(),
            ExprType::AccessVariable { name } => {
                // TODO: Lots of repeated code!
                match &mut self.location {
                    Location::Global => {
                        let idx = self.constant_pool.add(Object::from(name.clone()));
                        self.add_instruction(code, BytecodeType::GetGlobal(idx), expr.location);
                    }
                    Location::Local(env) => {
                        if let Some(v) = env.fetch_local(name) {
                            self.add_instruction(code, BytecodeType::GetLocal(v.idx), expr.location);
                        } else {
                            let idx = self.constant_pool.add(Object::from(name.clone()));
                            self.add_instruction(code, BytecodeType::GetGlobal(idx), expr.location);
                        }
                    }
                    Location::Class { env, members } => {
                        if let Some(v) = env.fetch_local(name) {
                            self.add_instruction(code, BytecodeType::GetLocal(v.idx), expr.location)
                        } else if let Some(v) = members.get(name) {
                            todo!()
                        } else {
                            let idx = self.constant_pool.add(Object::from(name.clone()));
                            self.add_instruction(code, BytecodeType::GetGlobal(idx), expr.location);
                        }
                    }
                }
            }
            ExprType::AccessList { list, index } => todo!(),
            ExprType::CallFunction { name, arguments } => {
                for arg in arguments.iter().rev() {
                    self.compile_expr(arg, code, false)?;
                }
                // TODO: For nested functions we first need to look into environments, then do this.
                // TODO: Hardcoded native print
                if name == "print" {
                    self.add_instruction(
                        code,
                        BytecodeType::Print {
                            arg_cnt: arguments.len().try_into().unwrap(),
                        },
                        expr.location,
                    );
                } else {
                    let cp_idx = self.constant_pool.add(Object::from(name.clone()));
                    self.add_instruction(code, BytecodeType::GetGlobal(cp_idx), expr.location);
                    self.add_instruction(
                        code,
                        BytecodeType::CallFunc {
                            arg_cnt: arguments.len().try_into().unwrap(),
                        },
                        expr.location,
                    );
                }
            }
            ExprType::Conditional {
                guard,
                then_branch,
                else_branch,
            } => {
                let label_else = self.label_generator.get_label("if_else");
                let label_end = self.label_generator.get_label("if_merge");
                self.compile_expr(guard, code, false)?;
                self.add_instruction(
                    code,
                    BytecodeType::BranchLabelFalse(label_else.clone()),
                    expr.location,
                );
                self.compile_expr(then_branch, code, drop)?;
                self.add_instruction(
                    code,
                    BytecodeType::JmpLabel(label_end.clone()),
                    expr.location,
                );
                self.add_instruction(code, BytecodeType::Label(label_else), expr.location);
                if let Some(else_body) = else_branch {
                    self.compile_expr(else_body, code, drop)?;
                } else if !drop {
                    self.add_instruction(code, BytecodeType::PushNone, expr.location);
                }
                self.add_instruction(code, BytecodeType::Label(label_end), expr.location);
            }
            ExprType::Operator { op, arguments } => {
                check_operator_arity(op, arguments.len())?;
                for arg in arguments.iter().rev() {
                    self.compile_expr(arg, code, false)?;
                }
                self.add_instruction(code, (*op).into(), expr.location);
            }
        }
        code.add_cond(
            Bytecode {
                instr: BytecodeType::Drop,
                location: expr.location,
            },
            drop,
        );
        Ok(())
    }

    fn compile_block(&mut self, stmts: &[Stmt], code: &mut Code) -> Result<(), &'static str> {
        let mut it = stmts.iter().peekable();
        while let Some(stmt) = it.next() {
            // Drop everything but the last result
            // Hovewer if it is statement, do NOT drop it because it does not
            // produces any value. Hovewer, if it is the last value, the block
            // returns none.
            match &stmt.node {
                // Never drop the last value, if it should be dropped then
                // not here, but the parent of this block will drop it
                StmtType::Expression(expr) => self.compile_expr(expr, code, it.peek().is_some())?,
                _ => {
                    self.compile_stmt(stmt, code)?;
                    if it.peek().is_none() {
                        self.add_instruction(code, BytecodeType::PushNone, stmt.location);
                    }
                }
            }
        }

        Ok(())
    }

    fn compile_stmt(&mut self, ast: &Stmt, code: &mut Code) -> Result<(), &'static str> {
        match &ast.node {
            StmtType::Variable {
                name,
                mutable,
                value,
            } => {
                self.compile_expr(value, code, false)?;
                match &mut self.location {
                    Location::Global => {
                        let name_idx = self.constant_pool.add(Object::from(name.clone()));
                        if *mutable {
                            self.add_instruction(
                                code,
                                BytecodeType::DeclVarGlobal { name: name_idx },
                                ast.location,
                            );
                        } else {
                            self.add_instruction(
                                code,
                                BytecodeType::DeclValGlobal { name: name_idx },
                                ast.location,
                            );
                        }
                    }
                    Location::Local(env) => {
                        env.add_local(name.clone(), self.local_count, *mutable)?;
                        self.add_instruction(
                            code,
                            BytecodeType::SetLocal(self.local_count),
                            ast.location,
                        );
                        self.add_locals(1);
                    }
                    Location::Class { env, members } => todo!(),
                }
            }
            StmtType::AssignVariable { name, value } => {
                self.compile_expr(value, code, false)?;
                // TODO: Repeated code!
                if let Location::Local(env) = &mut self.location {
                    if let Some(Local { idx, mutable }) = env.fetch_local(name) {
                        if !mutable {
                            return Err("Variable is declared immutable.");
                        }
                        self.add_instruction(code, BytecodeType::SetLocal(idx), value.location);
                    } else {
                        let idx = self.constant_pool.add(Object::from(name.clone()));
                        self.add_instruction(code, BytecodeType::SetGlobal(idx), value.location);
                    }
                } else {
                    let idx = self.constant_pool.add(Object::from(name.clone()));
                    self.add_instruction(code, BytecodeType::SetGlobal(idx), value.location);
                }
            }
            StmtType::AssignList { list, index, value } => todo!(),
            StmtType::Function {
                name,
                parameters,
                body,
            } => {
                let locals_backup = self.reset_locals();

                let new_fun_name_idx = self.constant_pool.add(Object::from(name.clone()));

                // For global function this behaves as expected,
                // The function just acts as another scope.
                // for nested it allows for closures (althrough
                // the indexes won't match).
                self.enter_scope();
                let mut new_code = Code::new();
                for (idx, par) in parameters.iter().enumerate() {
                    // Since we just added scope it will always be local
                    if let Location::Local(env) = &mut self.location {
                        env.add_local(par.clone(), self.local_count, false)?;
                        self.add_instruction(
                            &mut new_code,
                            BytecodeType::SetLocal(idx.try_into().unwrap()),
                            ast.location,
                        );
                        self.add_locals(1);
                    }
                }
                self.compile_fun(&mut new_code, body)?;
                self.leave_scope();

                let fun = Object::Function {
                    name: new_fun_name_idx,
                    parameters_cnt: parameters.len().try_into().unwrap(),
                    locals_cnt: self.local_max,
                    body: new_code,
                };

                let fun_idx = self.constant_pool.add(fun);
                self.add_instruction(code, BytecodeType::PushLiteral(fun_idx), ast.location);
                match &mut self.location {
                    Location::Global => {
                        self.add_instruction(
                            code,
                            BytecodeType::DeclValGlobal {
                                name: new_fun_name_idx,
                            },
                            ast.location,
                        );
                    }
                    Location::Local(env) => {
                        todo!("Nested functions are not yet implemented");
                    }
                    Location::Class { env, members } => todo!(),
                };

                self.restore_locals(locals_backup);
            }
            StmtType::Top(stmts) => self.compile_block(stmts, code)?,
            StmtType::While { guard, body } => todo!(),
            StmtType::Return(_) => todo!(),
            StmtType::Expression(expr) => self.compile_expr(expr, code, true)?,
            StmtType::Class { name, statements } => {
                let mut constructor = Code::new();
                let mut members: HashMap<String, Local> = HashMap::new();
                let mut members_cnt = 0;
                for stmt in statements {
                    match &stmt.node {
                        StmtType::Class { name, statements } => todo!("Nested classes are not yet supported"),
                        StmtType::Function { name, parameters, body } => todo!(),
                        StmtType::Variable { name, mutable, value } => {
                            self.compile_expr(value, &mut constructor, false)?;
                            if members.contains_key(name) {
                                Err("Member variable is already declared")
                            } else {
                                members.insert(name.clone(), Local { idx: members_cnt, mutable: *mutable });
                                members_cnt += 1;
                                Ok(())
                            }
                        },
                        _ => self.compile_stmt(stmt, code)
                    }?;
                }
            }
        };
        Ok(())
    }

    fn compile_fun(&mut self, code: &mut Code, ast: &Expr) -> Result<(), &'static str> {
        self.compile_expr(ast, code, false)?;
        if code.code.is_empty() {
            self.add_instruction(code, BytecodeType::PushNone, ast.location);
        }
        // Return last statement if return is omitted
        if !matches!(code.code.last().unwrap().instr, BytecodeType::Ret) {
            self.add_instruction(code, BytecodeType::Ret, ast.location);
        }
        Ok(())
    }
}

fn check_operator_arity(op: &Opcode, len: usize) -> Result<(), &'static str> {
    let arity = match op {
        Opcode::Add => 2,
        Opcode::Sub => 2,
        Opcode::Mul => 2,
        Opcode::Div => 2,
        Opcode::Mod => 2,
        Opcode::Less => 2,
        Opcode::LessEq => 2,
        Opcode::Greater => 2,
        Opcode::GreaterEq => 2,
        Opcode::Eq => 2,
        Opcode::Neq => 2,
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
        match &ins.instr {
            BytecodeType::Label(str) => {
                labels.insert(str.clone(), offset);
            }
            _ => without_labels.push(ins),
        }
        offset += ins_size;
    }

    // Remove the jump to labels with jump to address
    // TODO: Use short, long or normal jump based on distance
    without_labels
        .into_iter()
        .map(|ins| {
            let instr = match ins.instr {
                BytecodeType::JmpLabel(label) => {
                    let label_index = *labels.get(&label).unwrap();
                    BytecodeType::Jmp(label_index.try_into().unwrap())
                }
                BytecodeType::BranchLabel(label) => {
                    let label_index = *labels.get(&label).unwrap();
                    BytecodeType::Branch(label_index.try_into().unwrap())
                }
                BytecodeType::BranchLabelFalse(label) => {
                    let label_index = *labels.get(&label).unwrap();
                    BytecodeType::BranchFalse(label_index.try_into().unwrap())
                }
                _ => ins.instr,
            };
            Bytecode {
                instr,
                location: ins.location,
            }
        })
        .collect()
}

/// Compiles StmtType into constant pool and returns tuple (constant pool, entry point, globals)
pub fn compile(ast: &Stmt) -> Result<(ConstantPool, ConstantPoolIndex), &'static str> {
    let mut compiler = Compiler::new();
    let idx = compiler
        .constant_pool
        .add(Object::from(String::from("#main")));
    let mut code = Code::new();

    match ast.node {
        StmtType::Top(_) => {
            compiler.compile_stmt(ast, &mut code)?;
            compiler.add_instruction(&mut code, BytecodeType::Ret, ast.location);
            // Add top level return
        }
        _ => unreachable!(),
    }

    let main_fun = Object::Function {
        name: idx,
        parameters_cnt: 0,
        locals_cnt: compiler.local_max,
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
