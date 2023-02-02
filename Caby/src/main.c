#include <string.h>
#include <stdbool.h>

#include "common.h"
#include "memory/block_alloc.h"
#include "serializer.h"
#include "vm.h"
#include "dissasembler.h"
#include "bytecode.h"

#define EQ(right, i) (strcmp(argv[(i)], (right)) == 0)

/// Serializes program into constant pool 'cp'.
/// Returns non-zero if serialization failed.
vm_t read_program(const char* filename, u32* ep) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open file '%s'.", filename);
        exit(-2);
    }

    vm_t vm = serialize(f, ep);
    return vm;
}

void usage() {
    fprintf(stderr, "usage: caby command <input-file>\n");
    fprintf(stderr, " commands:\n");
    fprintf(stderr, "  disassemble <file> - Serializes bytecode from file and disassembles it.\n");
    fprintf(stderr, "  execute <file> - Serializes bytecode from file and executes it.\n");
}

static int disassemble(const char* argv[]) {
    const char* filename = *argv++;

    u32 ep;
    vm_t vm = read_program(filename, &ep);

    disassemble_constant_pool(stdout, &vm.const_pool);
    free_vm_state(&vm);

    return 0;
}

static int execute(const char* argv[]) {
    const char* filename = NULL;
    const char* source = NULL;
    for (;*argv != NULL; ++ argv) {
        if (strcmp(*argv, "--source") == 0) {
            source = *(++argv);
        } else {
            filename = *argv;
        }
    }

    if (filename == NULL) {
        fprintf(stderr, "Expected file after command 'run'\n");
        exit(4);
    }

    u32 ep;
    vm_t vm = read_program(filename, &ep);
    vm.filename = source;

    interpret(&vm, ep);

    free_vm_state(&vm);

    return 0;
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        usage();
        exit(1);
    }

    // 1MB heap
    init_heap(1024 * 1024 * 1024);
    int exit = 1;
    for (int i = 0; i < argc; ++ i) {
        if (EQ("disassemble", i)) {
            exit = disassemble(argv + 2);
        } else if (EQ("execute", i)) {
            exit = execute(argv + 2);
        }
    }
    if (exit == 1) {
        usage();
    }

    done_heap();
    return exit;
}
