#[macro_use] extern crate lalrpop_util;

lalrpop_mod!(pub grammar); // synthesized by LALRPOP

mod parser;
mod ast;
mod tests;

fn main() {
    println!("Hello, world!");
}
