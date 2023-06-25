#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "compiler.h"
#include "vm.h"
#include "hashtable.h"

typedef struct environment {
    struct environment* prev;
    struct table env;
} environment_t;

struct compiler {
    bc_chunk_t* current_chunk;
    environment_t* env;
    vm_t* vm;
    /// Current count of active variables.
    u16 local_count;
    /// Maximum number of local slots needed by a function.
    u16 local_max;
} compiler;

static void compiler_error(const char* str, ...) {
    va_list args;
    va_start(args, str);
    fprintf(stderr, "Compiler error: ");
    vfprintf(stderr, str, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(-1);
}

u16 max_u16(u16 a, u16 b) {
    return a > b ? a : b;
}

void add_locals(u16 count) {
    compiler.local_count += count;
    compiler.local_max = max_u16(compiler.local_count, compiler.local_max);
}

void reset_locals() {
    NOT_IMPLEMENTED();
}

// Returns true if the compiler is currently in the global environment.
static bool is_global() {
    return compiler.env == NULL;
}

void push_env() {
    struct environment* e = malloc(sizeof(*e));
    init_table(&e->env);
    e->prev = compiler.env;
    compiler.env = e;
}

void pop_env() {
    assert(compiler.env != NULL);
    struct environment* poped = compiler.env;
    compiler.env = compiler.env->prev;
    // Check that we don't underflow
    assert(compiler.local_count >= poped->env.count);
    compiler.local_count -= poped->env.count;
    free_table(&poped->env);
    free(poped);
}

/// Tries to find an index of local variable.
/// Returns false if no local variable of given
/// name is active.
bool get_local_variable(const char* s, u16* idx) {
    // There is no environment
    if (is_global()) {
        return false;
    }

    struct value key = NEW_OBJECT(new_string(compiler.vm, s));
    struct value res;
    for (struct environment* env = compiler.env; env != NULL; env = env->prev) {
        bool exists = table_get(&env->env, key, &res);
        if (exists) {
            if (res.type != VAL_INT) {
                compiler_error("A value saved under the variable key '%s' is not "
                               "local variable index (the types don't match).",
                               s);
            }

            *idx = res.integer; 
            return true;
        }
    }

    return false;
}

u16 introduce_variable(const char* s) {
    fprintf(stderr, "Introducing var: %s\n", s);
    struct value key = NEW_OBJECT(new_string(compiler.vm, s));
    u16 idx = compiler.local_count;
    table_set(&compiler.env->env, key, NEW_INT(idx));
    add_locals(1);
    return idx;
}

static void init_compiler(vm_t* vm) {
    compiler.vm = vm;
    compiler.local_count = compiler.local_max = 0;
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
    // They are compiled in reversed order because
    // we want the left value to be at the top of the stack.
    compile_expr(e->right, false);
    compile_expr(e->left, false);
    u8 opcode;
    switch (e->op) {
    case OP_PLUS: opcode = OP_IADD; break;
    case OP_TIMES: opcode = OP_IMUL; break;
    case OP_MINUS: opcode = OP_ISUB; break;
    case OP_DIV: opcode = OP_IDIV; break;
    case OP_EQUAL: opcode = OP_EQ; break;
    case OP_NOT_EQUAL: opcode = OP_NEQ; break;
    case OP_LESS: opcode = OP_ILESS; break;
    case OP_GREATER: opcode = OP_IGREATER; break;
    case OP_GEQ: opcode = OP_IGREATEREQ; break;
    case OP_LEQ: opcode = OP_ILESSEQ; break;
    default: compiler_error("Unknown binary operator");
    }
    write_opcode(opcode);
}

/// Generates a bytecode to fetch global variable.
void generate_global_access(const char* name) {
    write_opcode(OP_GET_GLOBAL);
    struct object_string* var_name = new_string(compiler.vm, name);
    u32 idx = write_constant_pool(&compiler.vm->const_pool, (struct object*)var_name);
    write_u32(idx);
}

void compile_expr_id(struct expr_id* id) {
    u16 local_idx;
    fprintf(stderr, "Fetching var %s\n", id->id.s);
    bool local = get_local_variable(id->id.s, &local_idx);
    fprintf(stderr, "The bitch is not local!\n");
    if (local) {
        write_opcode(OP_GET_LOCAL);
        write_u16(local_idx);
    } else {
        generate_global_access(id->id.s);
    }
}

void compile_expr_compound(struct expr_compound* e) {
    for (size_t i = 0; i < e->stmts_len; ++i) {
        compile_stmt(e->stmts[i]);
    }

    compile_expr(e->value, false);
}

void compile_expr_call(struct expr_call* call) {
    for (ssize_t i = call->args_len - 1; i >= 0; --i) {
        compile_expr(call->args[i], false);
    }

    compile_expr(call->target, false);
    write_opcode(OP_CALL_FUNC);
    write_u8(call->args_len);
}

void compile_expr_bool(struct expr_bool* b) {
    write_opcode(OP_PUSH_BOOL);
    write_u8(b->val);
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
        compile_expr_bool((struct expr_bool*)e);
        break;
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
        compile_expr_call((struct expr_call*)e);
        break;
    case EXPR_COMPOUND:
        push_env();
        compile_expr_compound((struct expr_compound*)e);
        pop_env();
        break;
    case EXPR_ID:
        compile_expr_id((struct expr_id*)e);
        break;
    }

    if (drop) {
        write_opcode(OP_DROP);
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

    if (is_global()) {
        if (v->mutable) {
            write_opcode(OP_VAR_GLOBAL);
        } else {
            write_opcode(OP_VAL_GLOBAL);
        }

        struct object_string* var_name = new_string(compiler.vm, v->name.s);
        u32 idx = write_constant_pool(&compiler.vm->const_pool, (struct object*)var_name);
        write_u32(idx);
    } else {
        u16 idx = introduce_variable(v->name.s);

        write_opcode(OP_SET_LOCAL);
        write_u16(idx);
    }
}

void compile_stmt_assign_var(struct stmt_assign_var* a) {
    compile_expr(a->value, false);

    u16 local_idx;
    bool local = get_local_variable(a->name.s, &local_idx);

    if (local) {
        write_opcode(OP_SET_LOCAL);
        write_u16(local_idx);
    } else {
        write_opcode(OP_SET_GLOBAL);

        struct object_string* var_name = new_string(compiler.vm, a->name.s);
        u32 idx = write_constant_pool(&compiler.vm->const_pool, (struct object*)var_name);
        write_u32(idx);
    }
}

void compile_stmt_function(struct stmt_function* f) {
    struct bc_chunk code;
    init_bc_chunk(&code);
    // Replace the current destination chunk
    struct bc_chunk* prev = compiler.current_chunk;
    compiler.current_chunk = &code;

    // Backup locals
    u16 locals_backup = compiler.local_count, locals_max_backup = compiler.local_max;
    compiler.local_count = compiler.local_max = 0;

    push_env();

    // Params
    for (size_t i = 0; i < f->param_len; ++i) {
        fprintf(stderr, "param: %s\n", f->parameters[i].s);
        introduce_variable(f->parameters[i].s);
        write_opcode(OP_SET_LOCAL);
        write_u16(i);
    }
    
    compile_expr(f->body, false);
    write_opcode(OP_RETURN);

    // Restore chunk and locals, do this before
    // defining the function.
    compiler.current_chunk = prev;

    struct object_string* name = new_string(compiler.vm, f->name.s);
    u32 name_idx = write_constant_pool(&compiler.vm->const_pool, (struct object*)name);

    struct object_function* fun = new_function(compiler.vm, f->param_len, compiler.local_max, code, name_idx);
    u32 fun_idx = write_constant_pool(&compiler.vm->const_pool, (struct object*)fun);

    // Define the actual function object as global immutable variable
    write_opcode(OP_PUSH_LITERAL);
    write_u32(fun_idx);
    write_opcode(OP_VAL_GLOBAL);
    write_u32(name_idx);

    pop_env();

    compiler.local_count = locals_backup;
    compiler.local_max = locals_max_backup;
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
    case STMT_FUNCTION:
        compile_stmt_function((struct stmt_function*)s);
        break;
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
    fprintf(stderr, "locals: %d\n", compiler.local_max);
    *ep = write_constant_pool(&vm.const_pool, (struct object*)main_fun);
    vm.gc.gc_off = false;
    return vm;
}
