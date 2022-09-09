use std::fmt;

#[derive(Debug, Clone, Copy)]
pub enum Opcode {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Less,
    LessEq,
    Greater,
    GreaterEq,
    Eq,
    Neq,
    Negate,
}

/// Represents top-level statements, these do not leave anything on the stack
#[derive(Debug, Clone)]
pub enum StmtType {
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

    Top(Vec<StmtType>),
    While {
        guard: Expr,
        body: Box<StmtType>,
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

    Block(Vec<StmtType>, Box<Expr>),

    List {
        size: Box<Expr>,
        values: Vec<StmtType>,
    },

    AccessVariable {
        name: String,
    },
    AccessList {
        list: Box<StmtType>,
        index: Box<StmtType>,
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
                Opcode::Sub | Opcode::Negate => "-",
                Opcode::Mul => "*",
                Opcode::Div => "/",
                Opcode::Mod => "%",
                Opcode::Less => "<",
                Opcode::LessEq => "<=",
                Opcode::Greater => ">",
                Opcode::GreaterEq => ">=",
                Opcode::Eq => "==",
                Opcode::Neq => "!=",
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
            Expr::AccessVariable { name } => println!("AccessVariable: {}\n", name),
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
            Expr::Block(vals, expr) => {
                for stmt in vals {
                    stmt.dump(prefix.clone());
                }
                expr.dump(prefix.clone());
            }
        }
    }
}

impl StmtType {
    pub fn dump(&self, prefix: String) {
        print!("{}", prefix);
        match self {
            StmtType::Variable {
                name,
                mutable,
                value,
            } => {
                println!("{}: {}", if *mutable { "var" } else { "val" }, name);
                value.dump(prefix + " ");
            }
            StmtType::AssignVariable { name, value } => todo!(),
            StmtType::AssignList { list, index, value } => todo!(),
            StmtType::Function {
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
            StmtType::Top(vals) => {
                for stmt in vals {
                    stmt.dump(String::from(""));
                }
            }
            StmtType::While { guard, body } => {
                println!("While: ");
                guard.dump(prefix.clone() + " ");
                body.dump(prefix + " ")
            }
            StmtType::Return(expr) => {
                println!("Return: ");
                expr.dump(prefix + " ");
            }
            StmtType::Expression(expr) => expr.dump(prefix),
        }
    }
}
