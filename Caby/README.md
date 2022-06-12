# Caby - The bytecode interpreter

## Bytecode format

Bytecode is split into three parts

### Constant pool

This is a table of constant values that will not change in the program. Following items are located here:
- String literals
- Functions
- Classes, methods and member variables
- Enums
- And whatnot

### Global variables
