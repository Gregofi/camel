#[cfg(test)]
mod parser_tests {
    use crate::grammar::TopLevelParser;

    #[test]
    fn expressions_test() {
        assert!(TopLevelParser::new().parse("1").is_ok());
        assert!(TopLevelParser::new().parse("1 * 2 * 3").is_ok());
        assert!(TopLevelParser::new().parse("1 + 2").is_ok());
        assert!(TopLevelParser::new().parse("1 + 2 + 3").is_ok());
        assert!(TopLevelParser::new().parse("1 + 2 * 3").is_ok());
        assert!(TopLevelParser::new().parse("1 + (1 + 2)").is_ok());
        assert!(TopLevelParser::new().parse("- 1").is_ok());
        assert!(TopLevelParser::new().parse("-2").is_ok());
        assert!(TopLevelParser::new().parse("1 - 1").is_ok());
        assert!(TopLevelParser::new().parse("1 -1").is_ok());
        assert!(TopLevelParser::new().parse("1 - -1").is_ok());
        assert!(TopLevelParser::new().parse("1 - --1").is_ok());
        assert!(TopLevelParser::new().parse("1 % 2").is_ok());
        assert!(TopLevelParser::new().parse("1 % +").is_err());
        assert!(TopLevelParser::new().parse("none + 2").is_ok());
        assert!(TopLevelParser::new().parse("true == none").is_ok());
    }

    #[test]
    fn blocks_test() {
        assert!(TopLevelParser::new().parse("{}").is_ok());
        assert!(TopLevelParser::new().parse("{1}").is_ok());
        assert!(TopLevelParser::new().parse("{1;}").is_ok());
        assert!(TopLevelParser::new().parse("{1;1;1}").is_ok());
        assert!(TopLevelParser::new().parse("1;{1;1;1};1").is_ok());
        assert!(TopLevelParser::new().parse("1;").is_ok());
        assert!(TopLevelParser::new().parse("1;2").is_ok());
        assert!(TopLevelParser::new().parse("1 + {1}").is_ok());
        assert!(TopLevelParser::new().parse("{1;2} + {3;4} * {5}").is_ok());
        assert!(TopLevelParser::new().parse("{val x = 5}").is_err());
        assert!(TopLevelParser::new().parse("{val x = 5;}").is_ok());
        assert!(TopLevelParser::new().parse("{val x = 5;1}").is_ok());
    }

    #[test]
    fn functions_test() {
        assert!(TopLevelParser::new().parse("def foo(a, b) = 1").is_ok());
        assert!(TopLevelParser::new()
            .parse("def foo(a, b) = {1; 3}")
            .is_ok());
        assert!(TopLevelParser::new().parse("def foo(a, b) = 1; 1").is_ok());

        // assert!(TopLevelParser::new().parse("return 1").is_err());
        assert!(TopLevelParser::new()
            .parse("def foo(a, b) = return 1")
            .is_err());
        assert!(TopLevelParser::new()
            .parse("def foo(a, b) = { return 1; }")
            .is_ok());
        assert!(TopLevelParser::new().parse("1 + def foo(a, b)").is_err());
    }

    #[test]
    fn conditional_test() {
        assert!(TopLevelParser::new().parse("if (1) { 1 }").is_ok());
        assert!(TopLevelParser::new()
            .parse("if (1) { 1 } else { 2 }")
            .is_ok());
        assert!(TopLevelParser::new()
            .parse("if (1 <= 2) { 1 } else { 2 }")
            .is_ok());
        assert!(TopLevelParser::new().parse("if(1) 1").is_err());
        assert!(TopLevelParser::new()
            .parse("def foo(a) = if (1 < 2) {1} else {2}")
            .is_ok());
        assert!(TopLevelParser::new()
            .parse("def foo(a) = if (1 < 2) {1} else {2}; foo(1);")
            .is_ok());
        assert!(TopLevelParser::new()
            .parse("def foo(a) = if (1 < 2) {1} elif 2 > 3 {2} else {3}; foo(1);")
            .is_ok());
    }

    #[test]
    fn calls_test() {
        assert!(TopLevelParser::new().parse("foo();").is_ok());
        assert!(TopLevelParser::new().parse("foo(1, 2);").is_ok());
        assert!(TopLevelParser::new().parse("print(\"Hi\");").is_ok());
        assert!(TopLevelParser::new()
            .parse("print(\"{} + {} = {}\", 1, 2, 1 + 2);")
            .is_ok());
    }

    #[test]
    fn identifiers_test() {
        assert!(TopLevelParser::new().parse("def foo(n) = n + 1").is_ok());
    }

    #[test]
    fn string_literals_test() {
        assert!(TopLevelParser::new().parse("\"Hello, World!\"").is_ok());
        assert!(TopLevelParser::new()
            .parse("\"Hello\" + \" World!\"")
            .is_ok());
    }

    #[test]
    fn var_decl() {
        assert!(TopLevelParser::new().parse("val x = 1").is_ok());
        assert!(TopLevelParser::new().parse("var x = 1").is_ok());
        assert!(TopLevelParser::new().parse("val x").is_err());
        assert!(TopLevelParser::new().parse("var x").is_err());
        assert!(TopLevelParser::new()
            .parse("if (val x = 1) { 1 } else { 2 }")
            .is_err());
        assert!(TopLevelParser::new().parse("val x = 1; val y = 2;").is_ok());
        assert!(TopLevelParser::new()
            .parse("val x = 1; { val x = 2; val y = 2;};")
            .is_ok());
        assert!(TopLevelParser::new().parse("val x = {1}").is_ok());
    }

    #[test]
    fn class_decl() {
        assert!(TopLevelParser::new().parse("class foo { };").is_ok());
        assert!(TopLevelParser::new()
            .parse(
                "class foo {
            def foo() = 1;
        };"
            )
            .is_ok());
    }

    #[test]
    fn member_access() {
        assert!(TopLevelParser::new().parse("x.y").is_ok());
        assert!(TopLevelParser::new().parse("\"Hello\".y").is_ok());
        assert!(TopLevelParser::new().parse("x.y + 2 * 3").is_ok());
        assert!(TopLevelParser::new().parse("(x.y + 2) * 3").is_ok());
        assert!(TopLevelParser::new().parse("(x.y).z").is_ok());
        assert!(TopLevelParser::new().parse("x.y.z").is_ok());
    }

    #[test]
    fn member_store() {
        assert!(TopLevelParser::new().parse("x.y = 1").is_ok());
        assert!(TopLevelParser::new().parse("\"Hello\".y = 1").is_ok());
        assert!(TopLevelParser::new().parse("x.y = 2 * 3 + x.y").is_ok());
        assert!(TopLevelParser::new().parse("(x.y).z = 1").is_ok());
        assert!(TopLevelParser::new().parse("x.y.z = 3").is_ok());
    }

    #[test]
    fn method_call() {
        assert!(TopLevelParser::new().parse("x.foo();").is_ok());
        assert!(TopLevelParser::new().parse("1.foo();").is_ok());
        assert!(TopLevelParser::new().parse("\"Hello\".foo();").is_ok());
    }
}
