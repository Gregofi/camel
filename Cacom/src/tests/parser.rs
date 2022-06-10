use crate::ast::{AST};
use crate::grammar::TopLevelParser;

#[test]
fn parse_test() {
    assert!(TopLevelParser::new().parse("1").is_ok());
    assert!(TopLevelParser::new().parse("1 + 2").is_ok());
    assert!(TopLevelParser::new().parse("1 + 2 + 3").is_ok());
    assert!(TopLevelParser::new().parse("1 + 2 * 3").is_ok());
    assert!(TopLevelParser::new().parse("1 + (1 + 2)").is_ok());

    assert!(TopLevelParser::new().parse("1;2").is_ok());
    assert!(TopLevelParser::new().parse("1;").is_ok());

    assert!(TopLevelParser::new().parse("{}").is_ok());
    assert!(TopLevelParser::new().parse("{1}").is_ok());
    assert!(TopLevelParser::new().parse("{1;1;1}").is_ok());
    assert!(TopLevelParser::new().parse("1;{1;1;1};1").is_ok());

    assert!(TopLevelParser::new().parse("def foo(a, b) = 1").is_ok());
    assert!(TopLevelParser::new().parse("def foo(a, b) = {1; 3}").is_ok());
    assert!(TopLevelParser::new().parse("def foo(a, b) = 1; 1").is_ok());

    assert!(TopLevelParser::new().parse("1 + {1}").is_ok());
    assert!(TopLevelParser::new().parse("{1;2} + {3;4} * {5}").is_ok());
    assert!(TopLevelParser::new().parse("1 + def foo(a, b)").is_err());
    assert!(TopLevelParser::new().parse("if (1) { 1 }").is_ok());
    assert!(TopLevelParser::new().parse("if (1) { 1 } else { 2 }").is_ok());

}
