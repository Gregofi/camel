

#[derive(Debug, Clone)]
pub enum Opcode {
    Add,
    Sub,
    Mul,
    Div,
}

#[derive(Debug, Clone)]
pub enum AST {
    Integer(i32),
    Float(f32),
    Bool(bool),
    NoneVal,

    Variable { name: String, value: Box<AST> },
    List { size: Box<AST>, values: Vec<Box<AST>> },

    AccessVariable { name: String },
    AccessList { list: Box<AST>, index: Box<AST> },

    AssignVariable { name: String, value: Box<AST> },
    AssignList { list: Box<AST>, index: Box<AST>, value: Box<AST> },

    Function { name: String, parameters: Vec<String>, body: Box<AST> },

    CallFunction { name: String, arguments: Vec<Box<AST>> },

    Top(Vec<Box<AST>>),
    Block(Vec<Box<AST>>),
    While { guard: Box<AST>, body: Box<AST> },

    Conditional { guard: Box<AST>, then_branch: Box<AST>, else_branch: Option<Box<AST>> },

    Print { format: String, arguments: Vec<Box<AST>> },
    Operator { op: Opcode, arguments: Vec<Box<AST>> },

    Return(Box<AST>),
}

pub trait IntoBoxed {
    type Into;
    fn into_boxed(self) -> Self::Into;
}

impl IntoBoxed for AST {
    type Into = Box<Self>;
    fn into_boxed(self) -> Self::Into {
        Box::new(self)
    }
}

impl IntoBoxed for Vec<AST> {
    type Into = Vec<Box<AST>>;
    fn into_boxed(self) -> Self::Into {
        self.into_iter().map(|ast| ast.into_boxed()).collect()
    }
}

impl IntoBoxed for Option<AST> {
    type Into = Option<Box<AST>>;
    fn into_boxed(self) -> Self::Into {
        self.map(|ast| ast.into_boxed())
    }
}
