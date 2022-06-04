use crate::ast::{AST};
use crate::grammar::TopLevelParser;

#[test]
fn parse_test() {
    assert!(TopLevelParser::new().parse("1").is_ok());
    assert!(TopLevelParser::new().parse("1 + 2").is_ok());
    assert!(TopLevelParser::new().parse("1 + 2 * 3").is_ok());
    assert!(TopLevelParser::new().parse("1;2").is_ok());
}
