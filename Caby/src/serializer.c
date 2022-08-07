#include "serializer.h"

u32 read_4bytes_be(FILE* f) {
    i8 res[4];
    res[0] = fgetc(f);
    res[1] = fgetc(f);
    res[2] = fgetc(f);
    res[3] = fgetc(f);
    if (res[3] == EOF) {
        fprintf(stderr, "Error reading file\n");
        exit(3);
    }
    return *(u32*)res;
}

u16 read_2bytes_be(FILE* f) {
    i8 res[2];
    res[0] = fgetc(f);
    res[1] = fgetc(f);
    if (res[1] == EOF) {
        fprintf(stderr, "Error reading file\n");
        exit(3);
    }
    return *(u16*)res;
}

void serialize_instruction(FILE* f, struct bc_chunk* c) {
    u8 ins = fgetc(f);
    write_byte(c, ins);

    switch (ins) {
        // One byte size instructions
        case OP_RETURN:
        case OP_DROP:
        case OP_DUP:
        case OP_IADD:
        case OP_ISUB:
        case OP_IMUL:
        case OP_IDIV:
        case OP_IAND:
        case OP_IOR:
        case OP_IREM:
        case OP_LABEL:
        case OP_EQ:
        case OP_PUSH_NONE:
            break;
        // Two byte size instructions
        case OP_PRINT:
        case OP_PUSH_BOOL:
            write_byte(c, fgetc(f));
            break;
        // Three byte size instructions
        case OP_JMP_SHORT:
        case OP_BRANCH_SHORT:
        case OP_BRANCH_FALSE_SHORT:
        case OP_PUSH_SHORT:
            write_word(c, read_2bytes_be(f));
            break;
        // Five byte size instructions
        case OP_PUSH_INT:
        case OP_PUSH_LITERAL:
        case OP_JMP:
        case OP_BRANCH:
        case OP_BRANCH_FALSE:
        case OP_SET_GLOBAL:
        case OP_SET_LOCAL:
        case OP_GET_GLOBAL:
        case OP_GET_LOCAL:
            write_dword(c, read_4bytes_be(f));
            break;
        // Six byte size instructions
        case OP_CALL_FUNC:
            write_dword(c, read_4bytes_be(f));
            write_byte(c, fgetc(f));
            break;
        default:
            fprintf(stderr, "Unknown instruction opcode in deserialize: 0x%x", ins);
            exit(-3);
    }
}

struct object* serialize_object(FILE* f) {
    u8 tag;
    fread(&tag, 1, 1, f);

    switch (tag) {
        case TAG_FUNCTION: {
            u32 name = read_4bytes_be(f);
            u8 parameters = fgetc(f);
            u32 body_len = read_4bytes_be(f);
            struct bc_chunk bc;
            init_bc_chunk(&bc);
            for (u32 i = 0; i < body_len; ++i) {
                serialize_instruction(f, &bc);
            }
            return (struct object*)new_function(parameters, bc, name);
        }
        case TAG_STRING: {
            u32 len = read_4bytes_be(f);
            char *str = vmalloc(len + 1);
            fread(str, 1, len, f);
            str[len] = '\0';
            struct object_string* obj_str = new_string_move(str, len);
            return (struct object*)obj_str;
        }
        default:
            fprintf(f, "Unknown tag in serialize: 0x%x", tag);
            return NULL;
    }
}

struct constant_pool serialize_constant_pool(FILE* f) {
    struct constant_pool cp;
    init_constant_pool(&cp);

    u32 len = read_4bytes_be(f);
    for (u32 i = 0; i < len; ++i) {
        struct object* obj = serialize_object(f);
        if (obj == NULL) {
            exit(-3);
        }

        write_constant_pool(&cp, obj);
    }

    return cp;
}

struct vm_state serialize(FILE* f) {
    struct constant_pool cp = serialize_constant_pool(f);
    struct vm_state state;
    init_vm_state(&state);
    state.const_pool = cp;

    u32 entry_point = read_4bytes_be(f);
    state.chunk = &as_function_s(state.const_pool.data[entry_point])->bc;
    return state;
}
