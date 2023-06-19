#include "src/memory.h"
#include "test.h"
#include "src/memory/arena_alloc.h"
#include "src/parser.h"
#include "src/memory/block_alloc.h"
#include "src/compiler.h"
#include <string.h>

TEST(BasicCompilerTest) {
    struct ArenaAllocator alloc = arena_init();
    struct stmt* program = parse("1", &alloc);
    u32 ep;
    vm_t vm = compile(program, &ep); 
    interpret(&vm, ep);
    arena_done(&alloc);
    free_vm_state(&vm);
    return 0;
}

int main() {
    init_heap(1024 * 1024 * 1024);
    BasicCompilerTest();
    done_heap();
}
