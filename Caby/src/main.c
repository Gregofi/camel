#include <string.h>
#include <stdbool.h>

#include "serializer.h"
#include "vm.h"
#include "dissasembler.h"
#include "bytecode.h"

#define EQ(right, i) (strcmp(argv[(i)], (right)) == 0)

/// Serializes program into constant pool 'cp'.
/// Returns non-zero if serialization failed.
int read_program(const char* filename, struct vm_state* vm) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open file '%s'.", filename);
        return 1;
    }

    *vm = serialize(f);
    return 0;
}

void usage() {
    fprintf(stderr, "usage: caby command <input-file>\n");
    fprintf(stderr, " commands:\n");
    fprintf(stderr, "  disassemble <file> - Serializes bytecode from file and disassembles it.\n");
    fprintf(stderr, "  execute <file> - Serializes bytecode from file and executes it.\n");
}

static int disassemble(const char* argv[]) {
    const char* filename = *argv++;

    struct vm_state vm;
    init_vm_state(&vm);
    read_program(filename, &vm);

    disassemble_constant_pool(stdout, &vm.const_pool);
    free_vm_state(&vm);

    return 0;
}

static int execute(const char* argv[]) {
    if (*argv == NULL) {
        fprintf(stderr, "Expected file after command 'run'\n");
        exit(4);
    }
    const char* filename = *argv++;

    struct vm_state vm;
    if (read_program(filename, &vm) != 0) {
        fprintf(stderr, "Fatal\n");
        exit(4);
    }

    interpret(&vm);

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
