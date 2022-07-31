# :camel: Camel

Camel is a dynamic scripting language. It is primarily meant as a toy project for learning interpreters and Rust programming language. The project is split into two main parts, the **Ca**mel **Com**piler (Cacom) and the **Ca**mel **By**tecode interpreter (Caby).

The language is very simple, here is an example of calculating factorial:

```
fun fact(x) = {
    if x == 0 {
        x
    } else {
        x * fact(x - 1)
    }
}

print("{}\n", fact(10));
```

And here is another example of finding maximum value in list:

```
fun max(lst) = {
    max = 0
    for x in lst {
        if x > max {
            max = x
        }
    }
    return max
}

lst = [1, 2, 5, 3, 1];
print("Max({}) = {}", lst, max(lst));
```

The language has C-like syntax but supports advanced constructs such as objects, lists, strings and so on. It basically aims to be similar to Python with more C-like syntax and stricter scoping rules.

Following features will hopefully be implemented:
- Algebraic data types -> Pattern matching with enums
- Fully interpolated strings
- Explicit types
- Some other way of managing errors without the use of exceptions (use adt?)
- Closures
- Dictionaries
- Sets
- Error handling
  - Draft: Use ? For signaling that the value can be some Error value and propagate it probably with `?` like Rust.

Following features are considered
- Exceptions.
- Inheritance - I feel like inheritance often makes classes unecessary complex. Use composition instead.
- Operator overloading - Honestly, this goes either way. It certainly compacts code and sometimes improves
                         readibility. But coming from scala, it can be abused.

## Standard library

Language itself should also contain standard library. This is however far future. Many of the language specific things will probably be implemented straight in C (such as string operations).

## Camel Compiler

The language is first compiled into an AST, from which a bytecode is generated. This step does the **Cacom** part of the project, which is an compiler from the Camel source code to bytecode. The compiler is written in Rust.

## Bytecode interpreter

When the compilation is done, an interpreting takes place. This is a task for the bytecode interpreter, which is an **Caby** part of the project. The interpreter is written in C.

## Common format

Bytecode can be serialized into byteformat, which is used as common speaking ground for compiler and interpreter. Compiler serializes data into this format, interpreter then deserializes it and interprets it.

The format can be found in `Caby/README.md`.
