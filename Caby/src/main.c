#include <string.h>
#include <stdbool.h>

#include "vm.h"
#include "dissasembler.h"
#include "bytecode.h"

#define EQ(right, i) (strcmp(argv[(i)], (right)) == 0)

void usage() {
    fprintf(stderr, "usage: caby command <input-file>\n");
}

int disassemble(const char* argv[]) {
    struct bc_chunk c;
    init_bc_chunk(&c);
    write_byte(&c, OP_RETURN);

    dissasemble_chunk(stdout, &c);

    free_bc_chunk(&c);
    return 0;
}

int execute(const char* argv[]) {
    vm_state vm;
    init_vm_state(&vm);

    struct bc_chunk c;
    init_bc_chunk(&c);
    write_byte(&c, OP_RETURN);

    execute(&vm, &c);

    free_bc_chunk(&c);
    free_vm_state(&vm);

    return 0;
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        usage();
        exit(1);
    }

    for (int i = 0; i < argc; ++ i) {
        if (EQ("disassemble", i)) {
            return disassemble(argv + 2);
        } else if (EQ("execute", i)) {
            return execute(argv + 2);
        }
    }
}