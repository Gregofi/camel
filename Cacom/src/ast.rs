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

/// Represents top-level statements, these do not leave anything on the stack
#[derive(Debug, Clone)]
pub enum AST {
    Variable {
        name: String,
        mutable: bool,
        value: Expr,
    },
    AssignVariable {
        name: String,
        value: Expr,
    },
    AssignList {
        list: Expr,
        index: Expr,
        value: Expr,
    },

    Function {
        name: String,
        parameters: Vec<String>,
        body: Expr,
    },

    Top(Vec<AST>),
    While {
        guard: Expr,
        body: Box<AST>,
    },

    Return(Expr),
    Expression(Expr),
}

/// Expressions always leave some value on the stack
///
/// For example, if statement always leaves resulting value
/// even if else is omitted (value is none).
///
/// But for, while, assignment and so on does not leave any
/// value on the stack.
///
/// Blocks of statements are also expression, the value
/// is either the one of last statement or none if the
/// block is empty.
#[derive(Debug, Clone)]
pub enum Expr {
    Integer(i32),
    Float(f32),
    Bool(bool),
    NoneVal,
    String(String),

    Block(Vec<AST>),

    List {
        size: Box<Expr>,
        values: Vec<AST>,
    },

    AccessVariable {
        name: String,
    },
    AccessList {
        list: Box<AST>,
        index: Box<AST>,
    },
    CallFunction {
        name: String,
        arguments: Vec<Expr>,
    },
    Conditional {
        guard: Box<Expr>,
        then_branch: Box<Expr>,
        else_branch: Option<Box<Expr>>,
    },

    Operator {
        op: Opcode,
        arguments: Vec<Expr>,
    },
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

impl Expr {
    pub fn dump(&self, prefix: String) {
        print!("{}", prefix);
        match self {
            Expr::Integer(val) => println!("Integer: {}", val),
            Expr::Float(val) => println!("Float: {}", val),
            Expr::Bool(val) => println!("Bool: {}", val),
            Expr::NoneVal => println!("Unit"),
            Expr::String(val) => println!("String: {}", val),
            Expr::List { size, values } => {
                println!("List:");
                size.dump(prefix.clone() + " ");
                for val in values {
                    val.dump(prefix.clone() + " ");
                }
            }
            Expr::AccessVariable { name } => todo!(),
            Expr::AccessList { list, index } => todo!(),
            Expr::CallFunction { name, arguments } => {
                println!("Call: {}", name);
                for arg in arguments {
                    arg.dump(prefix.clone() + " ");
                }
            }
            Expr::Conditional {
                guard,
                then_branch,
                else_branch,
            } => {
                println!("If: ");
                guard.dump(prefix.clone() + " ");
                then_branch.dump(prefix.clone() + " ");
                if else_branch.is_some() {
                    else_branch.as_ref().unwrap().dump(prefix + " ");
                }
            }
            Expr::Operator { op, arguments } => {
                println!("Operator: {}", op);
                for arg in arguments {
                    arg.dump(prefix.clone() + " ");
                }
            }
            Expr::Block(vals) => {
                for stmt in vals {
                    stmt.dump(prefix.clone());
                }
            }
        }
    }
}

impl AST {
    pub fn dump(&self, prefix: String) {
        print!("{}", prefix);
        match self {
            AST::Variable {
                name,
                mutable,
                value,
            } => {
                println!("{}: {}", if *mutable { "var" } else { "val" }, name);
                value.dump(prefix + " ");
            }
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
                body.dump(prefix + " ");
            }
            AST::Top(vals) => {
                for stmt in vals {
                    stmt.dump(String::from(""));
                }
            }
            AST::While { guard, body } => {
                println!("While: ");
                guard.dump(prefix.clone() + " ");
                body.dump(prefix + " ")
            }
            AST::Return(expr) => {
                println!("Return: ");
                expr.dump(prefix + " ");
            }
            AST::Expression(expr) => expr.dump(prefix),
        }
    }
}
