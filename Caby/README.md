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

### Opcodes
- push_short = 0x01 | 1B Num
- push_int   = 0x02 | 3B Num
- push_long  = 0x03 | 7B Num
Used for pushing int literals onto the operand stack.

- push_bool  = 0x04 | 1B boolean
Pushes boolean literal onto the operand stack

- push_literal = 0x05 | 3B Index to constant pool
Used for pushing other values onto the operand stack (references to objects, strings...)

- get_local = 0x06 | 3B Index to local frame
Push the value of a local variable onto the stack

- set_local = 0x07 | 3B Index to local frame
Pop a value from the operand stack and write it into the given local frame

- call_func = 0x08 | 3B Index to constant pool | 1B Arguments count
Calls a function (not an object method) at given constant pool index.
Pops arguments of an operand stack

- return = 0x09 
Exits the function.

- label = 0x00
Does nothing, acts as a helper in dissasembly. When executed in code,
only bumps the IP.

- jump_short = 0x0A | 2B address
- jump = 0x0B | 5B address
- jump_long = 0x0C | 7B address
Unconditional jump, address is **BYTE** offset (not the number of instruction) to which to jump

- branch_short = 0x0D | 2B address
- branch = 0x0E | 5B address
- branch_long = 0x0F | 7B address
Conditional jump, pops value from stack, if it is *truthy*, then the jump will be performed.

- print 0x10 | 3B index to constant pool | 1B argument count
Prints an interpolated string `print "Hello there, {}" "General Kenobi"`
Pops arguments from stack and tries to replace `{}` in the string with it.

#### Arithmetic operations
- iadd 0x30
- isub 0x31
- imul 0x32
- idiv 0x33
- irem 0x34
- iand 0x35
- ior  0x36

Arithmetic operations on integers, pop two from the stack, perform operation, push the result.

NOTE: Bitwise operations will be added later.
