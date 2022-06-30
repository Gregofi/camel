use std::env;
use std::fs;
use std::io;

use clap::Parser;
use clap::Clap;

#[macro_use]
extern crate lalrpop_util;

use crate::compiler::compile;
use crate::grammar::TopLevelParser;
use crate::serializable::Serializable;

lalrpop_mod!(pub grammar); // synthesized by LALRPOP

mod ast;
mod bytecode;
mod compiler;
mod objects;
mod serializable;
mod tests;

#[derive(Clap, Debug)]
#[clap(version = crate_version!(), author = crate_authors!())]
enum Action {
    Compile(CompileAction),
    ASTDump(ASTDumpAction),
    Disassemble(DisassembleAction),
}

impl Action {
    pub fn act(&self) {
        match self {
            Action::Compile(action) => action.compile(),
            Action::ASTDump(action) => action.dump(),
            Action::Disassemble(action) => action.dissasemble(),
        }
    }
}
struct CompileAction {}

impl CompileAction {
    fn compile(&self) {}
}

struct ASTDumpAction {}

impl ASTDumpAction {
    fn dump(&self) {}
}

struct DisassembleAction {}

impl DisassembleAction {
    fn disassemble(&self) {}
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let mut file_name: Option<String> = None;
    let mut output_name: String = String::from("a.caby");

    // TODO: Create proper parser with actions
    for arg in args {
        match arg {
            _ => file_name = Some(arg),
        }
    }

    if file_name == None {
        panic!("No file to compile!");
    }

    let mut out_f = fs::File::create(output_name).expect("Cannot open output file");

    let f = fs::read_to_string(file_name.unwrap()).expect("Couldn't read file");

    let ast = TopLevelParser::new()
        .parse(&f)
        .expect("Unable to parse file");

    let bytecode = compile(&ast).expect("Compilation error");

    bytecode
        .serialize(&mut out_f)
        .expect("Unable to write to output file");
}
