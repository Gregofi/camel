use std::env;
use std::io;
use std::fs;

#[macro_use] extern crate lalrpop_util;

use crate::grammar::TopLevelParser;
use crate::compiler::compile;
use crate::serializable::Serializable;

lalrpop_mod!(pub grammar); // synthesized by LALRPOP

mod ast;
mod tests;
mod compiler;
mod bytecode;
mod objects;
mod serializable;

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

    let ast = TopLevelParser::new().parse(&f).expect("Unable to parse file");

    let bytecode = compile(&ast).expect("Compilation error");

    bytecode.serialize(&mut out_f).expect("Unable to write to output file");
}
