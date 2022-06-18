use crate::ast::{AST};

fn compile(ast: AST) {
    match ast {
        AST::Integer(_) => todo!(),
        AST::Float(_) => todo!(),
        AST::Bool(_) => todo!(),
        AST::NoneVal => todo!(),
        AST::Variable { name, value } => todo!(),
        AST::List { size, values } => todo!(),
        AST::AccessVariable { name } => todo!(),
        AST::AccessList { list, index } => todo!(),
        AST::AssignVariable { name, value } => todo!(),
        AST::AssignList { list, index, value } => todo!(),
        AST::Function { name, parameters, body } => todo!(),
        AST::CallFunction { name, arguments } => todo!(),
        AST::Top(_) => todo!(),
        AST::Block(_) => todo!(),
        AST::While { guard, body } => todo!(),
        AST::Conditional { guard, then_branch, else_branch } => todo!(),
        AST::Print { format, arguments } => todo!(),
        AST::Operator { op, arguments } => todo!(),
        AST::Return(_) => todo!(),
    }
}