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
                                .value_name("INPUT-FILE")
                                .help("Camel source code");

    // TODO: input file should probably not be passed like that.
    let matches = App::new("Cacom")
            .subcommand_required(true)
            .subcommand(SubCommand::with_name("dump-ast")
                .about("Dumps the AST of the source file")
                .arg(input_file.clone()))
            .subcommand(SubCommand::with_name("compile")
                .about("Compile the source file into Caby bytecode")
                .arg(input_file.clone())
                .arg(Arg::new("output-file")
                    .short('o')
                    .long("output-file")
                    .required(false)
                    .value_name("OUTPUT-FILE")
                    .help("The Caby bytecode output file")))
            .subcommand(SubCommand::with_name("export")
                .about("Compile camel source to bytecode and print it to standard output in human readable format")
                .arg(input_file.clone()));
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

fn export_action(input_file: &String) {
    let f = fs::read_to_string(input_file).expect("Couldn't read file");

    let ast = TopLevelParser::new()
        .parse(&f)
        .expect("Unable to parse file");

    let bytecode = compile(&ast).expect("Compilation error");
    println!("{}", bytecode);
}

fn main() {

    let matches = cli().get_matches();

    match matches.subcommand() {
        Some(("dump-ast", sub_matches)) => {
            let file = sub_matches.get_one::<String>("input-file").unwrap();
            dump_action(file);
        },
        Some(("compile", sub_matches)) => {
            let input_file = sub_matches.get_one::<String>("input-file").unwrap();
            let output_file = sub_matches.get_one::<String>("output-file").unwrap();
            compile_action(input_file, output_file);
        },
        Some(("export", sub_matches)) => {
            let input_file = sub_matches.get_one::<String>("input-file").unwrap();
            export_action(input_file);
        }
        Some((name, _)) => {
            unreachable!("Unsupported subcommand '{}'", name)
        }
        None => {}
    }
}
