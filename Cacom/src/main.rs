#[macro_use]
extern crate lalrpop_util;
extern crate clap;

use std::fs;

use clap::{Arg, App, SubCommand, Command};

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

fn cli() -> Command<'static> {
    let input_file = Arg::new("input-file")
                                .short('i')
                                .long("input-file")
                                .required(true)
                                .value_name("INPUTFILE")
                                .help("Camel source code");

    // TODO: input file should probably not be passed like that.
    let matches = App::new("Cacom")
            .subcommand_required(true)
            .subcommand(SubCommand::with_name("dump-ast")
                .about("Dumps the AST of the source file")
                .arg(input_file.clone()))
            .subcommand(SubCommand::with_name("compile")
                .about("Compile the source file into Caby bytecode")
                .arg(input_file)
                .arg(Arg::new("output-file")
                    .short('o')
                    .long("output-file")
                    .required(false)
                    .value_name("OUTPUTFILE")
                    .help("The Caby bytecode output file"))
            );
    matches
}

fn compile_action(input_file: &String, output_file: &String) {
    let f = fs::read_to_string(input_file).expect("Couldn't read file");
    let mut out_f = fs::File::create(output_file).expect("Cannot open output file");

    let ast = TopLevelParser::new()
        .parse(&f)
        .expect("Unable to parse file");

    let bytecode = compile(&ast).expect("Compilation error");

    bytecode
        .serialize(&mut out_f)
        .expect("Unable to write to output file");
}

fn dump_action(input_file: &String) {
    let f = fs::read_to_string(input_file).expect("Couldn't read file");

    let ast = TopLevelParser::new()
        .parse(&f)
        .expect("Unable to parse file");

    ast.dump();
}

fn main() {

    let matches = cli().get_matches();

    match matches.subcommand() {
        Some(("dump-ast", sub_matches)) => {
            let file = sub_matches.get_one::<String>("INPUTFILE").unwrap();
            dump_action(file);
        },
        Some(("compile", sub_matches)) => {
            let input_file = sub_matches.get_one::<String>("INPUTFILE").unwrap();
            let output_file = sub_matches.get_one::<String>("OUTPUTFILE").unwrap();
            compile_action(input_file, output_file);
        },
        Some((name, _)) => {
            unreachable!("Unsupported subcommand '{}'", name)
        }
        None => {}
    }
}
