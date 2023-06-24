#include <assert.h>
#include "compiler.h"
#include "vm.h"
#include "hashtable.h"

typedef struct environment {
    struct environment* prev;
    struct table* env;
} environment_t;

struct compiler {
    bc_chunk_t* current_chunk;
    environment_t* env;
    vm_t* vm;
    /// Current count of active variables.
    u16 local_count;
    /// Maximum number of local slots needed by a function.
    u16 local_max;
    /// Indicates whether the compiler is in the global environment.
    bool global;
} compiler;

static void error(const char* s) {
    fprintf(stderr, "Compiler error: %s", s);
    exit(-1);
}

void push_env() {
    struct environment* e = malloc(sizeof(*compiler.env->prev));
    e->prev = compiler.env;
    compiler.env = e;
}

void pop_env() {
    assert(compiler.env != NULL);
    struct environment* poped = compiler.env;
    compiler.env = compiler.env->prev;
    free(poped);
}

void introduce_variable(const char* s) {
    struct value key = NEW_OBJECT(new_string(compiler.vm, s));
    table_set(compiler.env->env, key, NEW_NONE());
}

static void init_compiler(vm_t* vm) {
    compiler.vm = vm;
    compiler.global = true;
}

static void write_u8(u8 data) {
    write_byte(compiler.current_chunk, data);
}

static void write_opcode(u8 opcode) {
    write_u8(opcode);
}

static void write_u16(u16 data) {
    write_word(compiler.current_chunk, data);
}

static void write_u32(u32 data) {
    write_dword(compiler.current_chunk, data);
}

static void write_u64(u64 data) {
    write_qword(compiler.current_chunk, data);
}

void compile_expr(struct expr* e, bool drop);

void compile_stmt(struct stmt* s);

void compile_expr_none(struct expr_none* e) {
    write_opcode(OP_PUSH_NONE);
}

void compile_expr_int(struct expr_int* e) {
    write_opcode(OP_PUSH_INT);
    write_u32(e->val);
}

void compile_expr_binary(struct expr_binary* e) {
    compile_expr(e->left, false);
    compile_expr(e->right, false);
    u8 opcode;
    switch (e->op) {
    case OP_PLUS: opcode = OP_IADD; break;
    case OP_TIMES: opcode = OP_IMUL; break;
    case OP_MINUS: opcode = OP_ISUB; break;
    default: error("Unknown binary operator");
    }
    write_opcode(opcode);
}

void compile_expr_id(struct expr_id* id) {
    write_opcode(OP_GET_GLOBAL);
    struct object_string* var_name = new_string(compiler.vm, id->id.s);
    u32 idx = write_constant_pool(&compiler.vm->const_pool, (struct object*)var_name);
    write_dword(compiler.current_chunk, idx);
}

void compile_expr(struct expr* e, bool drop) {
    switch (e->k) {
    case EXPR_INTEGER:
        compile_expr_int((struct expr_int*)e);
        break;
    case EXPR_UNARY:
        NOT_IMPLEMENTED();
    case EXPR_BINARY:
        compile_expr_binary((struct expr_binary*)e);
        break;
    case EXPR_FLOAT:
        NOT_IMPLEMENTED();
    case EXPR_BOOL:
        NOT_IMPLEMENTED();
    case EXPR_NONE:
        compile_expr_none((struct expr_none*)e);
        break;
    case EXPR_STRING:
        NOT_IMPLEMENTED();
    case EXPR_LIST:
        NOT_IMPLEMENTED();
    case EXPR_ACCESS_LIST:
        NOT_IMPLEMENTED();
    case EXPR_ACCESS_MEMBER:
        NOT_IMPLEMENTED();
    case EXPR_IF:
        NOT_IMPLEMENTED();
    case EXPR_OP:
        NOT_IMPLEMENTED();
    case EXPR_CALL:
        NOT_IMPLEMENTED();
    case EXPR_COMPOUND:
        NOT_IMPLEMENTED();
    case EXPR_ID:
        compile_expr_id((struct expr_id*)e);
        break;
    }
}

void compile_expr_float(struct expr* e) {
    NOT_IMPLEMENTED();
}

void compile_stmt_top(struct stmt_top* top) {
    for (size_t i = 0; i < top->len; ++i) {
        compile_stmt(top->statements[i]);
    }
}

void compile_stmt_expr(struct stmt_expr* e) {
    compile_expr(e->e, true);
}

void compile_stmt_var(struct stmt_variable* v) {
    compile_expr(v->value, false); 

    if (compiler.global) {
        if (v->mutable) {
            write_opcode(OP_VAR_GLOBAL);
        } else {
            write_opcode(OP_VAL_GLOBAL);
        }

        struct object_string* var_name = new_string(compiler.vm, v->name.s);
        u32 idx = write_constant_pool(&compiler.vm->const_pool, (struct object*)var_name);
        write_dword(compiler.current_chunk, idx);
    } else {
        NOT_IMPLEMENTED();
    }
}

void compile_stmt_assign_var(struct stmt_assign_var* a) {
    compile_expr(a->value, false);

    if (compiler.global) {
        write_opcode(OP_SET_GLOBAL);

        struct object_string* var_name = new_string(compiler.vm, a->name.s);
        u32 idx = write_constant_pool(&compiler.vm->const_pool, (struct object*)var_name);
        write_dword(compiler.current_chunk, idx);
    } else {
        NOT_IMPLEMENTED();
    }
}

void compile_stmt(struct stmt* s) {
    switch (s->k) {
    case STMT_VAR:
        compile_stmt_var((struct stmt_variable*)s);
        break;
    case STMT_ASSIGN_VAR:
        compile_stmt_assign_var((struct stmt_assign_var*)s);
        break;
    case STMT_ASSIGN_LIST: NOT_IMPLEMENTED();
    case STMT_FUNCTION: NOT_IMPLEMENTED();
    case STMT_CLASS: NOT_IMPLEMENTED();
    case STMT_TOP: 
        compile_stmt_top((struct stmt_top*)s);
        break;
    case STMT_WHILE: NOT_IMPLEMENTED();
    case STMT_RETURN: 
        compile_stmt_expr((struct stmt_expr*)s);
        break;
    case STMT_EXPR:
        compile_stmt_expr((struct stmt_expr*)s);
        break;
    case STMT_ASSIGN_MEMBER: NOT_IMPLEMENTED();
    }
}

vm_t compile(struct stmt* top, u32* ep) {
    vm_t vm;
    init_vm_state(&vm);
    vm.gc.gc_off = true;

    bc_chunk_t main_chunk;
    init_bc_chunk(&main_chunk);
    init_compiler(&vm);
    compiler.current_chunk = &main_chunk;

    compile_stmt(top);

    write_byte(&main_chunk, OP_RETURN);

    struct object_string* main_fun_name = new_string(&vm, "#entry_point");
    u32 idx = write_constant_pool(&vm.const_pool, (struct object*)main_fun_name);
    
    struct object_function* main_fun = new_function(&vm, 0, compiler.local_max, main_chunk, idx);
    *ep = write_constant_pool(&vm.const_pool, (struct object*)main_fun);
    vm.gc.gc_off = false;
    return vm;
}
