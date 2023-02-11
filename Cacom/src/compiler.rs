use std::collections::HashMap;
use std::fmt;

use lalrpop_util::state_machine;

use crate::ast::{Expr, ExprType, Opcode, Stmt, StmtType};
use crate::bytecode::{Bytecode, BytecodeType, Code, ConstantPoolIndex, LocalIndex};
use crate::objects::{ConstantPool, Function, Object};
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
    Class(Environment),
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

    // TODO: Is it really necessary to receive the index when its stored in self?
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
            Location::Class(env) => {
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
            Location::Class(env) => {
                let var_cnt: u16 = env.leave_scope().try_into().unwrap();
                // We can't creep into global environment here
                self.local_count -= var_cnt;
            }
        };
    }

    fn add_instruction(&mut self, code: &mut Code, instr: BytecodeType, location: CodeLocation) {
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
                // TODO: some repeated code
                match &mut self.location {
                    Location::Global => {
                        let idx = self.constant_pool.add(Object::from(name.clone()));
                        self.add_instruction(code, BytecodeType::GetGlobal(idx), expr.location);
                    }
                    Location::Local(env) | Location::Class(env) => {
                        if let Some(v) = env.fetch_local(name) {
                            self.add_instruction(
                                code,
                                BytecodeType::GetLocal(v.idx),
                                expr.location,
                            );
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
            ExprType::MethodCall {
                left,
                name,
                arguments,
            } => {
                for arg in arguments.iter().rev() {
                    self.compile_expr(arg, code, false)?;
                }

                self.compile_expr(left, code, false)?;
                let cp_idx = self.constant_pool.add(Object::from(name.clone()));
                self.add_instruction(
                    code,
                    BytecodeType::DispatchMethod {
                        name: cp_idx,
                        arg_cnt: arguments.len().try_into().unwrap(),
                    },
                    expr.location,
                )
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
            ExprType::MemberRead { left, right } => {
                self.compile_expr(left, code, false)?;
                let member_idx = self.constant_pool.add(Object::from(right.clone()));
                self.add_instruction(code, BytecodeType::GetMember(member_idx), expr.location);
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
                        // TODO: Caused two consequentive push none when last value in block was statement.
                        // Left for the time being in case it triggers error.
                        // self.add_instruction(code, BytecodeType::PushNone, stmt.location);
                    }
                }
            }
        }

        Ok(())
    }

    fn emit_function_def(
        &mut self,
        name: ConstantPoolIndex,
        fun: ConstantPoolIndex,
        location: CodeLocation,
        code: &mut Code,
    ) {
        self.add_instruction(code, BytecodeType::PushLiteral(fun), location);
        match &mut self.location {
            Location::Global => {
                self.add_instruction(code, BytecodeType::DeclValGlobal { name }, location);
            }
            Location::Local(env) => {
                todo!("Nested functions are not yet implemented");
            }
            Location::Class(env) => unreachable!(),
        };
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
                        // TODO: Repeated code!
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
                    Location::Class(env) => todo!(),
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
                let fun = self.compile_fun(new_fun_name_idx, parameters, body)?;
                let fun_idx = self.constant_pool.add(Object::Function(fun));
                self.emit_function_def(new_fun_name_idx, fun_idx, ast.location, code);

                self.restore_locals(locals_backup);
            }
            StmtType::Top(stmts) => self.compile_block(stmts, code)?,
            StmtType::While { guard, body } => todo!(),
            StmtType::Return(_) => todo!(),
            StmtType::Expression(expr) => self.compile_expr(expr, code, true)?,
            StmtType::Class { name, statements } => {
                let name_idx = self.constant_pool.add(Object::from(name.clone()));

                let mut methods: Vec<ConstantPoolIndex> = vec![];

                // TODO: This should be handled at the grammar level
                for stmt in statements {
                    match &stmt.node {
                        StmtType::Class { name, statements } => {
                            todo!("Nested classes are not yet supported")
                        }
                        StmtType::Function {
                            name,
                            parameters,
                            body,
                        } => {
                            let name = self.constant_pool.add(Object::from(name.clone()));
                            let method = self.compile_fun(name, parameters, body)?;
                            let method_idx = self.constant_pool.add(Object::Function(method));
                            methods.push(method_idx);
                        }
                        _ => panic!("Class can only contain method or member definitions."),
                    };
                }
                let class_idx = self.constant_pool.add(Object::Class {
                    name: name_idx,
                    methods,
                });

                // Generate default constructor
                let mut constructor = Code::new();
                constructor.add(Bytecode {
                    instr: BytecodeType::NewObject(class_idx),
                    location: CodeLocation(0, 0),
                });
                constructor.add(Bytecode {
                    instr: BytecodeType::SetLocal(0),
                    location: CodeLocation(0, 0),
                });
                constructor.add(Bytecode {
                    instr: BytecodeType::GetLocal(0),
                    location: CodeLocation(0, 0),
                });
                constructor.add(Bytecode {
                    instr: BytecodeType::Ret,
                    location: CodeLocation(0, 0),
                });

                let cons_fun = Function {
                    name: name_idx,
                    parameters_cnt: 0,
                    locals_cnt: 0,
                    body: constructor,
                };
                let fun_idx = self.constant_pool.add(Object::Function(cons_fun));
                self.emit_function_def(name_idx, fun_idx, ast.location, code);
            }
            StmtType::MemberStore { left, right, val } => {
                self.compile_expr(val, code, false)?;
                self.compile_expr(left, code, false)?;
                let member_idx = self.constant_pool.add(Object::from(right.clone()));
                self.add_instruction(code, BytecodeType::SetMember(member_idx), ast.location);
            }
        };
        Ok(())
    }

    fn compile_parameters(
        &mut self,
        code: &mut Code,
        parameters: &[String],
        location: CodeLocation,
    ) -> Result<(), &'static str> {
        for (idx, par) in parameters.iter().enumerate() {
            // Since we just added scope it will always be local
            if let Location::Local(env) = &mut self.location {
                env.add_local(par.clone(), self.local_count, false)?;
                self.add_instruction(
                    code,
                    BytecodeType::SetLocal(idx.try_into().unwrap()),
                    location,
                );
                self.add_locals(1);
            }
        }
        Ok(())
    }

    fn compile_fun(
        &mut self,
        name: ConstantPoolIndex,
        parameters: &Vec<String>,
        body: &Expr,
    ) -> Result<Function, &'static str> {
        // For global function this behaves as expected,
        // The function just acts as another scope.
        // for nested it allows for closures (althrough
        // the indexes won't match).
        self.enter_scope();
        let mut code = Code::new();
        self.compile_parameters(&mut code, parameters, body.location)?;
        self.compile_expr(body, &mut code, false)?;
        if code.code.is_empty() {
            self.add_instruction(&mut code, BytecodeType::PushNone, body.location);
        }
        // Return last statement if return is omitted
        if !matches!(code.code.last().unwrap().instr, BytecodeType::Ret) {
            self.add_instruction(&mut code, BytecodeType::Ret, body.location);
        }
        self.leave_scope();

        Ok(Function {
            name,
            parameters_cnt: parameters.len().try_into().unwrap(),
            locals_cnt: self.local_max,
            body: code,
        })
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

    let main_fun = Object::Function(Function {
        name: idx,
        parameters_cnt: 0,
        locals_cnt: compiler.local_max,
        body: code,
    });
    let main_fun_idx = compiler.constant_pool.add(main_fun);
    // TODO: Consider rewritting this into some prettier form
    compiler.constant_pool.data = compiler
        .constant_pool
        .data
        .into_iter()
        .map(|f| match f {
            Object::Function(Function {
                body,
                name,
                parameters_cnt,
                locals_cnt,
            }) => Object::Function(Function {
                body: Code {
                    code: jump_pass(body.code),
                },
                name,
                parameters_cnt,
                locals_cnt,
            }),
            _ => f,
        })
        .collect();

    Ok((compiler.constant_pool, main_fun_idx))
}
