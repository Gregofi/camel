use std::fmt;

#[derive(Debug, Clone)]
pub enum Opcode {
    Add,
    Sub,
    Mul,
    Div,
    Less,
    LessEq,
    Greater,
    GreaterEq,
    Eq,
}

#[derive(Debug, Clone)]
pub enum AST {
    Integer(i32),
    Float(f32),
    Bool(bool),
    NoneVal,
    String(String),

    Variable {
        name: String,
        value: Box<AST>,
    },
    List {
        size: Box<AST>,
        values: Vec<AST>,
    },

    AccessVariable {
        name: String,
    },
    AccessList {
        list: Box<AST>,
        index: Box<AST>,
    },

    AssignVariable {
        name: String,
        value: Box<AST>,
    },
    AssignList {
        list: Box<AST>,
        index: Box<AST>,
        value: Box<AST>,
    },

    Function {
        name: String,
        parameters: Vec<String>,
        body: Box<AST>,
    },

    CallFunction {
        name: String,
        arguments: Vec<AST>,
    },

    Top(Vec<AST>),
    Block(Vec<AST>),
    While {
        guard: Box<AST>,
        body: Box<AST>,
    },

    Conditional {
        guard: Box<AST>,
        then_branch: Box<AST>,
        else_branch: Option<Box<AST>>,
    },

    Operator {
        op: Opcode,
        arguments: Vec<AST>,
    },

    Return(Box<AST>),
}

impl fmt::Display for Opcode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "{}",
            match self {
                Opcode::Add => "+",
                Opcode::Sub => "-",
                Opcode::Mul => "*",
                Opcode::Div => "/",
                Opcode::Less => "<",
                Opcode::LessEq => "<=",
                Opcode::Greater => ">",
                Opcode::GreaterEq => ">=",
                Opcode::Eq => "==",
            }
        )
    }
}

impl AST {
    pub fn dump(&self) {
        fn _dump(ast: &AST, offset: String) {
            print!("{}", offset);
            match ast {
                AST::Integer(val) => println!("Integer: {}", val),
                AST::Float(val) => println!("Float: {}", val),
                AST::Bool(val) => println!("Bool: {}", val),
                AST::NoneVal => println!("Unit"),
                AST::String(val) => println!("String: {}", val),
                AST::Variable { name, value } => {
                    println!("Variable: {}", name);
                    _dump(value, offset + " ");
                }
                AST::List { size, values } => {
                    println!("List:");
                    _dump(size, offset.clone() + " ");
                    for val in values {
                        _dump(val, offset.clone() + " ");
                    }
                }
                AST::AccessVariable { name } => todo!(),
                AST::AccessList { list, index } => todo!(),
                AST::AssignVariable { name, value } => todo!(),
                AST::AssignList { list, index, value } => todo!(),
                AST::Function {
                    name,
                    parameters,
                    body,
                } => {
                    print!("Function: {} [", &name);
                    for param in parameters {
                        print!("{} ", param);
                    }
                    println!("]");
                    _dump(body, offset + " ");
                }
                AST::CallFunction { name, arguments } => {
                    println!("Call: {}", name);
                    for arg in arguments {
                        _dump(arg, offset.clone() + " ");
                    }
                }
                AST::Top(vals) => {
                    for stmt in vals {
                        _dump(stmt, String::from(""));
                    }
                }
                AST::Block(vals) => {
                    for stmt in vals {
                        _dump(stmt, offset.clone());
                    }
                }
                AST::While { guard, body } => {
                    println!("While: ");
                    _dump(guard, offset.clone() + " ");
                    _dump(body, offset + " ")
                }
                AST::Conditional {
                    guard,
                    then_branch,
                    else_branch,
                } => {
                    println!("If: ");
                    _dump(guard, offset.clone() + " ");
                    _dump(then_branch, offset.clone() + " ");
                    if else_branch.is_some() {
                        _dump(else_branch.as_ref().unwrap(), offset + " ");
                    }
                }
                AST::Operator { op, arguments } => {
                    println!("Operator: {}", op);
                    for arg in arguments {
                        _dump(arg, offset.clone() + " ");
                    }
                }
                AST::Return(expr) => {
                    println!("Return: ");
                    _dump(expr, offset + " ");
                }
            }
        }
        _dump(self, String::from(""));
    }
}
