#include "bytecode.h"
#include "common.h"
#include "memory.h"
#include "object.h"

#include <string.h>

void init_bc_chunk(struct bc_chunk* c) {
    c->data = NULL;
    c->len = 0;
    c->cap = 0;

    c->location = 0;
    c->location_cap = 0;
    c->location_len = 0;
}

void free_bc_chunk(struct bc_chunk* c) {
    free(c->data);
    free(c->location);
    init_bc_chunk(c);
}

size_t ins_size(enum opcode op) {
    switch (op) {
        case OP_RETURN:
        case OP_LABEL:
        case OP_DROP:
        case OP_DUP:
        case OP_IADD:
        case OP_ISUB:
        case OP_IMUL:
        case OP_IDIV:
        case OP_IMOD:
        case OP_IAND:
        case OP_IOR:
        case OP_EQ:
        case OP_NEQ:
        case OP_ILESS:
        case OP_ILESSEQ:
        case OP_IGREATER:
        case OP_IGREATEREQ:
        case OP_INEG:
        case OP_PUSH_NONE:
            return 1;
        case OP_DROPN:
        case OP_PUSH_BOOL:
        case OP_PRINT:
        case OP_CALL_FUNC:
            return 2;
        case OP_PUSH_SHORT:
        case OP_JMP_SHORT:
        case OP_BRANCH_SHORT:
        case OP_SET_LOCAL:
        case OP_GET_LOCAL:
            return 3;
        case OP_PUSH_INT:
        case OP_JMP:
        case OP_BRANCH:
        case OP_BRANCH_FALSE:
        case OP_BRANCH_FALSE_SHORT:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_VAL_GLOBAL:
        case OP_VAR_GLOBAL:
        case OP_PUSH_LITERAL:
            return 5;
        default:
            UNREACHABLE();
    }
}

size_t range_between(u8* begin, u8* end) {
    size_t cnt = 0;
    while (begin < end) {
        cnt += 1;
        begin += ins_size(*begin);
    }
    return cnt;
}

void write_byte(struct bc_chunk* c, u8 byte) {
    c->data = handle_capacity(c->data, c->len, &c->cap, sizeof(*c->data));
    c->data[c->len++] = byte;
}

void write_word(struct bc_chunk* c, u16 word) {
    write_byte(c, word >> 8);
    write_byte(c, word);
}

void write_dword(struct bc_chunk* c, u32 dword) {
    write_word(c, dword >> 16);
    write_word(c, dword);
}

void write_loc(struct bc_chunk* c, u64 begin, u64 end) {
    c->location = handle_capacity(c->location, c->location_len, &c->location_cap, sizeof(*c->location));
    c->location[c->location_len++].begin = begin;
    c->location[c->location_len++].end = end;
}

void init_constant_pool(struct constant_pool* cp) {
    memset(cp, 0, sizeof(*cp));
}

void free_constant_pool(struct constant_pool* cp) {
    free(cp->data);
    init_constant_pool(cp);
}

void write_constant_pool(struct constant_pool* cp, struct object* object) {
    cp->data = handle_capacity(cp->data, cp->len, &cp->cap, sizeof(*cp->data));
    cp->data[cp->len++] = object;
}

struct object* read_constant_pool(struct constant_pool* cp, u32 idx) {
    if (idx >= cp->len) {
        fprintf(stderr, "Error reading constant pool: len - %lu, idx - %u\n", cp->len, idx);
        exit(5);
    }
    return cp->data[idx];
}

struct object_string* read_string_cp(struct constant_pool* cp, u32 idx) {
    struct object_string* s = as_string_s(read_constant_pool(cp, idx));
    if (s == NULL) {
        fprintf(stderr, "Error: Object is not a string");
        exit(5);
    }
    return s;
}

struct object_function* read_function_cp(struct constant_pool* cp, u32 idx) {
    struct object_function* f = as_function_s(read_constant_pool(cp, idx));
    if (f == NULL) {
        fprintf(stderr, "Error: Object is not a string");
        exit(5);
    }
    return f;
}
