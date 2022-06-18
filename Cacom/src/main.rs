#[macro_use] extern crate lalrpop_util;

lalrpop_mod!(pub grammar); // synthesized by LALRPOP

mod ast;
mod tests;
mod compiler;

fn main() {
    println!("Hello, world!");
}
