# Caby - The bytecode interpreter

## Bytecode format

Bytecode is split into three parts

### Constant pool

This is a table of constant values that will not change in the program. Following items are located here:
#### String literals
`0x01 | length - 4 bytes | the string`
The string is NOT zero terminated.
- Functions
`0x00 | name - 4 bytes index to constant pool | parameters count - 1 byte | code length (in instructions) - 4 bytes | code ...`
There probably should be serialized local variables...
- Classes, methods and member variables
- Enums
- And whatnot

Size of constant pool is 2^32 (so it can be indexed by 32bit int).

### Global variables

### Entry point

Constant pool index (4 bytes) at which the global function body is located.

## Bytecode oppcodes
- push_short = 0x01 | 2B Num
- push_int   = 0x02 | 4B Num
- push_long  = 0x03 | 8B Num  
Used for pushing int literals onto the operand stack.

- push_bool  = 0x04 | 1B boolean  
Pushes boolean literal onto the operand stack

- push_literal = 0x05 | 4B Index to constant pool  
Used for pushing other values onto the operand stack (references to objects, strings...)

- get_local = 0x06 | 2B Index to local frame  
Push the value of a local variable onto the stack

- set_local = 0x07 | 2B Index to local frame  
Pop a value from the operand stack and write it into the given local frame

- call_func = 0x08 | 4B Index to constant pool | 1B Arguments count  
Calls a function (not an object method) at given constant pool index.
Pops arguments of an operand stack

- ret = 0x09  
Exits the function.

- label = 0x00  
Does nothing, acts as a helper in dissasembly. When executed in code,
only bumps the IP.

- jmp_short = 0x0A | 2B address
- jmp = 0x0B | 4B address
- jmp_long = 0x0C | 8B address  
Unconditional jump, address is **BYTE** offset (not the number of instruction) to which to jump

- branch_short = 0x0D | 2B address
- branch = 0x0E | 4B address
- branch_long = 0x0F | 8B address  
Conditional jump, pops value from stack, if it is *truthy*, then the jump will be performed.

- print 0x10 | 1B argument count  
Prints an interpolated string `print "Hello there, {}" "General Kenobi"`
Pops arguments from stack and tries to replace `{}` in the string with it.

- drop 0x11
Drops first value from the stack.

- dup 0x12
Pops value from the stack and then pushes this value twice on top of the stack

- get_global 0x13 | 4B index to constant pool
Push the value of a global variable which name is stored in constant pool to the top stack
- set_global 0x14 | 4B index to constant pool
Pop value from the top of the stack and assign it into variable which name is equal to the constant pool string indexed.

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
