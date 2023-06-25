#include <string.h>
#include <stdbool.h>

#include "common.h"
#include "memory/block_alloc.h"
#include "serializer.h"
#include "compiler.h"
#include "parser.h"
#include "vm.h"
#include "dissasembler.h"
#include "bytecode.h"
#include "memory/arena_alloc.h"

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
    fprintf(stderr, "usage: caby <command> <input-file>\n");
    fprintf(stderr, " If no command is specified, then the input file is executed.\n");
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

char* readfile(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Unable to open file at '%s'\n", filename);
        return NULL;
    }
    char* buff;

    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);

    buff = malloc((sizeof(*buff) + 1) * fsize);
    if (buff == NULL) {
        fprintf(stderr, "bad alloc: out of memory\n");
        goto ERR;
    }

    if (fread(buff, 1, fsize, f) != fsize) {
        fprintf(stderr, "Unable to read the source file\n");
        goto ERR;
    }
    buff[fsize] = 0;
    
    fclose(f);
    return buff;

ERR:
    fclose(f);
    free(buff);
    return NULL;
}

int run(const char* sourcefile, const char* argv[]) {
    char* source = readfile(sourcefile);
    if (!source) {
        fprintf(stderr, "Unable to read source code");
        exit(-1);
    }

    struct ArenaAllocator alloc = arena_init();
    struct stmt* program = parse(source, &alloc);

    u32 ep;
    vm_t vm = compile(program, &ep); 
    vm.filename = sourcefile;
    arena_done(&alloc);

    int exit_code = interpret(&vm, ep);
    free_vm_state(&vm);
    free(source);
    return exit_code;
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        usage();
        exit(1);
    }

    // 1MB heap
    init_heap(1024 * 1024 * 1024);
    int exit = 1;
    if (EQ("disassemble", 1)) {
        exit = disassemble(argv + 2);
    } else if (EQ("execute", 1)) {
        exit = execute(argv + 2);
    } else {
        exit = run(argv[1], argv + 2);
    }

    if (exit == 1) {
        usage();
    }

    done_heap();
    return exit;
}
