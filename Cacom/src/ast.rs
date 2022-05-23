use serde::{Serialize, Deserialize};

#[derive(Debug, Serialize, Deserialize, Clone)]
pub enum AST {
    Integer(i32),
    Float(f32),
    Unitialized,

    Variable { name: Identifier, value: Box<AST> },
    List { size: Box<AST>, values: Vec<Box<AST>> },

    AccessVariable { name: Identifier },
    AccessList { list: Box<AST>, index: Box<AST> },

    AssignVariable { name: Identifier, value: Box<AST> },
    AssignList { list: Box<AST>, index: Box<AST>, value: Box<AST> },

    Function { name: Identifier, parameters: Vec<Identifier>, body: Box<AST> },

    CallFunction { name: Identifier, arguments: Vec<Box<AST>> },

    Top(Vec<Box<AST>>),
    Block(Vec<Box<AST>>),
    While { guard: Box<AST>, body: Box<AST> },

    Conditional { guard: Box<AST>, consequent: Box<AST>, alternative: Box<AST> },

    Print { format: String, arguments: Vec<Box<AST>> },
}
