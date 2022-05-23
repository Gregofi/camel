# Camel

Camel is a dynamic scripting language. It is primarily meant as a toy project for learning interpreters and Rust programming language. The project is split into two main parts, the Camel Compiler (Cacom) and the Camel Bytecode interpreter (Caby).

The language is very simple, here is an example of calculating factorial:

```
fun fact(x) {
    if x == 0 {
        x
    } else {
        x * fact(x - 1)
    }
}

println("{}", fact(10))
```

And here is another example of finding maximum value in list:

```
fun max(lst) {
    if (lst.len() == 0) {
        throw RuntimeException("Wrong list size");
    }
    max = lst[0]
    for x in lst {
        if (x > max) {
            max = x
        }
    }
    return max
}

println("", max([1, 2, 5, 3, 1]));
```

The language has C-like syntax but supports advanced constructs such as objects, inheritance, exceptions, lists, strings and so on. It basically aims to be a more C-like Python with better scoping rules.

## Camel Compiler

The language is first compiled into an AST, from which a bytecode is generated. This step does the **Cacom** part of the project, which is an compiler from the Camel source code to bytecode. The compiler is written in Rust.

## Bytecode interpreter

When the compilation is done, an interpreting takes place. This is a task for the bytecode interpreter, which is an **Caby** part of the project. The interpreter is written in the C language, primarily to play around with it (and also to make it fast :)).
