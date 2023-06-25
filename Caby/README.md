# Caby - The bytecode interpreter

## Bytecode file format

Bytecode is split into three parts

They are described below in detail.

The main structure is:
```
constant pool size - u32 | constant pool objects ... | entry point - u32
```

### Constant pool

This is a table of constant values that will not change in the program. Following items are located here:
#### Constant pool objects
- String
`0x01 | length - 4 bytes | the string`
The string is NOT zero terminated.
- Function
```
0x00 | name - 4 bytes index to constant pool | parameters count - 1 byte | number of locals slots - 2b
     | code length (in instructions) - 4b | (code - ???b | location begin - 8b | location end - 8b) ... |
```
The code is an array of instruction opcodes (one byte) and its location in the source file (4 bytes). Functions themselves are invisible to the VM even when in constant pool. To call them you need to define them in main. Push them onto stack and then use SetVal instruction.
- Class
```
0x02 | name - 4 byte index to constant pool | methods count - 2 bytes | 4 bytes index to constant pool *
```
Methods are stored as normal functions, the class keeps only indexes to them.

Size of constant pool is 2^32 (so it can be indexed by 32bit int).
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

- push_none = 0x20
Pushes None (Unit) value onto the stack.

- get_local = 0x06 | 2B Index to local frame  
Push the value of a local variable onto the stack

- set_local = 0x07 | 2B Index to local frame  
Pop a value from the operand stack and write it into the given local frame

- call_func = 0x08 | 1B Arguments count  
Calls a function (not an object method) at the top of the stack.
Pops arguments of an operand stack. First popped should be the first argument etc.

- return = 0x09  
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

- branch_false_short - 0x2D | 2B address
- branch_false - 0x2E | 4B address
- branch false_long - 0x0F | 8B address

- print 0x10 | 1B argument count  
Prints an interpolated string.
Pops arguments from stack and tries to replace `{}` in the string with it.

- drop 0x11
Drops first value from the stack.

- dropn - 0x25 | 1B count
Drops first 'count' values from the stack.

- dup 0x12
Pops value from the stack and then pushes this value twice on top of the stack

- get_global 0x13 | 4B index to constant pool
Push the value of a global variable which name is stored in constant pool to the top stack
- set_global 0x14 | 4B index to constant pool
Pop value from the top of the stack and assign it into variable which name is equal to the constant pool string indexed.
- def_val_global 0x15 | 4B index to constant pool
Defines a new immutable global variable. The cp index points to a string which is the variable name.
Pops value of the stack and assigns it to the variable.
- def_var_global 0x16 | 4B index to constant pool
Defines a new mutable global variable. The cp index points to a string which is the variable name.
Pops value of the stack and assigns it to the variable.

- new_object 0x60 | 4B index to constant pool
Creates a new instance of class in the constant pool (Not name, but the class itself!).
- get_member 0x61 | 4B index to constant pool
Pops class instance from the stack and pushes value of its member (string in cp). If such member doesn't exists then
runtime error occurs. 
- set_member 0x62 | 4B index to constant pool
Pops class instance and value from the stack and sets its member (string in cp) to that value.
- dispatch_method 0x63 | 4B index to constant pool | 1B number of arguments
Pops object off the stack and corresponding number of arguments. Calls method on popped object with name at cp index with given arguments. 
#### Arithmetic operations
- iadd 0x30
- isub 0x31
- imul 0x32
- idiv 0x33
- imod 0x34
- ineg 0x3C

- iand 0x35
- ior  0x36

- iless 0x37
- ilesseq 0x38
- igreater 0x39
- igreatereq 0x3A,
- ieq  0x3B
- ineq 0x3C
Arithmetic operations on integers, pop two from the stack, perform operation, push the result.

NOTE: Bitwise operations will be added later.

# Implementation details
## Local variables
Local variables have their own array. It has 65536(2^16) slots. So there can be at most 65536 local variables
in the whole program accross all function calls. This should be fine, stack overflow would probably happen
sooner than you running out of local variables.

Best solution was to keep them on the stack, but this is tricky with the block expression, since they do not
immediately not follow each other. For example
```
var x = 1;
if (true && {var y = x == 1; y}) {
    ...
}
```
There is the `true` expression between x and y. This can be solved by simulating the stack offset in compilation.
Also, the last value on the stack is not the return value of the block, because there is the local variables sitting there.
