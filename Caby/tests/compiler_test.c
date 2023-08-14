#include "src/memory.h"
#include "test.h"
#include "src/memory/arena_alloc.h"
#include "src/parser.h"
#include "src/memory/block_alloc.h"
#include "src/compiler.h"
#include <string.h>

vm_t run_program(const char* s_p) {
    struct ArenaAllocator alloc = arena_init();
    struct stmt* program = parse(s_p, &alloc, NULL);

    u32 ep;
    vm_t vm = compile(program, &ep); 
    arena_done(&alloc);

    interpret(&vm, ep);
    return vm;
}

TEST(BasicCompilerTest1) {
    vm_t vm = run_program("1");
    ASSERT_EQ(vm.op_stack->type, VAL_INT);
    ASSERT_EQ(vm.op_stack->integer, 1);

    free_vm_state(&vm);
    return 0;
}

TEST(BasicCompilerTest2) {
    vm_t vm = run_program("1 + 2");
    ASSERT_EQ(vm.op_stack->type, VAL_INT);
    ASSERT_EQ(vm.op_stack->integer, 3);

    free_vm_state(&vm);
    return 0;
}

TEST(BasicCompilerTest3) {
    vm_t vm = run_program("1 2 3 4 + 1 5");
    ASSERT_EQ(vm.op_stack->type, VAL_INT);
    ASSERT_EQ(vm.op_stack->integer, 5);

    free_vm_state(&vm);
    return 0;
}

TEST(CompoundStmt1) {
    vm_t vm = run_program("{1}");
    ASSERT_EQ(vm.op_stack->type, VAL_INT);
    ASSERT_EQ(vm.op_stack->integer, 1);

    free_vm_state(&vm);
    return 0;
}

TEST(CompoundStmt2) {
    vm_t vm = run_program("{1; 3}");
    ASSERT_EQ(vm.op_stack->type, VAL_INT);
    ASSERT_EQ(vm.op_stack->integer, 3);

    free_vm_state(&vm);
    return 0;
}

TEST(GlobalVars1) {
    vm_t vm = run_program("val x = 3 x");
    ASSERT_EQ(vm.op_stack->type, VAL_INT);
    ASSERT_EQ(vm.op_stack->integer, 3);

    free_vm_state(&vm);
    return 0;
}

TEST(GlobalVars2) {
    vm_t vm = run_program("var x = 5 x = 3 x");
    ASSERT_EQ(vm.op_stack->type, VAL_INT);
    ASSERT_EQ(vm.op_stack->integer, 3);

    free_vm_state(&vm);
    return 0;
}

TEST(LocalVars1) {
    vm_t vm = run_program(
"var x = 5\n"
"{var x = 4;x = 3;}\n"
"x\n");
    ASSERT_EQ(vm.op_stack->type, VAL_INT);
    ASSERT_EQ(vm.op_stack->integer, 5);

    free_vm_state(&vm);
    return 0;
}

TEST(LocalVars2) {
    vm_t vm = run_program(
"var x = 5\n"
"{var y = 4; x = y + 1;}\n"
"x\n");
    ASSERT_EQ(vm.op_stack->type, VAL_INT);
    ASSERT_EQ(vm.op_stack->integer, 5);

    free_vm_state(&vm);
    return 0;
}

TEST(FunctionCall1) {
    vm_t vm = run_program("def foo() = 1 foo() + 2");
    ASSERT_EQ(vm.op_stack->type, VAL_INT);
    ASSERT_EQ(vm.op_stack->integer, 3);

    free_vm_state(&vm);
    return 0;
}

TEST(FunctionCall2) {
    vm_t vm = run_program("def bar(a) = a + 1 bar(2)");
    ASSERT_EQ(vm.op_stack->type, VAL_INT);
    ASSERT_EQ(vm.op_stack->integer, 3);

    free_vm_state(&vm);
    return 0;
}

int main() {
    init_heap(1024 * 1024 * 1024);
    BasicCompilerTest1();
    BasicCompilerTest2();
    BasicCompilerTest3();
    GlobalVars1();
    GlobalVars2();
    CompoundStmt1();
    CompoundStmt2();
    fprintf(stderr, "-----------------------------\n");
    LocalVars1();
    LocalVars2();
    FunctionCall1();
    FunctionCall2();
    done_heap();
}
