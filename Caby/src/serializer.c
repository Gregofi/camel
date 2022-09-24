#include "serializer.h"
#include "bytecode.h"
#include "common.h"
#include "vm.h"

#define GEN_READ_NBYTES_LE(type, n) \
type read_##n##bytes_le(FILE* f) {\
    type data;\
    fread(&data, (n), 1, f);\
    return data;\
}

GEN_READ_NBYTES_LE(u8, 1);
GEN_READ_NBYTES_LE(u16, 2);
GEN_READ_NBYTES_LE(u32, 4);
GEN_READ_NBYTES_LE(u64, 8);

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
        case OP_IMOD:
        case OP_LABEL:
        case OP_ILESS:
        case OP_ILESSEQ:
        case OP_IGREATER:
        case OP_IGREATEREQ:
        case OP_EQ:
        case OP_NEQ:
        case OP_INEG:
        case OP_PUSH_NONE:
            break;
        // Two byte size instructions
        case OP_PRINT:
        case OP_PUSH_BOOL:
        case OP_DROPN:
        case OP_CALL_FUNC:
            write_byte(c, fgetc(f));
            break;
        // Three byte size instructions
        case OP_JMP_SHORT:
        case OP_BRANCH_SHORT:
        case OP_BRANCH_FALSE_SHORT:
        case OP_PUSH_SHORT:
        case OP_SET_LOCAL:
        case OP_GET_LOCAL:
            write_word(c, read_2bytes_le(f));
            break;
        // Five byte size instructions
        case OP_PUSH_INT:
        case OP_PUSH_LITERAL:
        case OP_JMP:
        case OP_BRANCH:
        case OP_BRANCH_FALSE:
        case OP_SET_GLOBAL:
        case OP_GET_GLOBAL:
        case OP_VAL_GLOBAL:
        case OP_VAR_GLOBAL:
            write_dword(c, read_4bytes_le(f));
            break;
        default:
            fprintf(stderr, "Unknown instruction opcode in deserialize: 0x%x", ins);
            exit(-3);
    }
    // write_loc(c, )
    // There are locations remaining, skip them for now
    fseek(f, 16, SEEK_CUR);
}

struct object* serialize_object(FILE* f, vm_t* vm) {
    u8 tag;
    fread(&tag, 1, 1, f);

    switch (tag) {
        case TAG_FUNCTION: {
            u32 name = read_4bytes_le(f);
            u8 parameters = fgetc(f);
            u16 locals_cnt = read_2bytes_le(f);
            u32 body_len = read_4bytes_le(f);
            struct bc_chunk bc;
            init_bc_chunk(&bc);
            for (u32 i = 0; i < body_len; ++i) {
                serialize_instruction(f, &bc);
            }
            return (struct object*)new_function(vm, parameters, locals_cnt, bc, name);
        }
        case TAG_STRING: {
            u32 len = read_4bytes_le(f);
            char *str = vmalloc(vm, len + 1);
            fread(str, 1, len, f);
            str[len] = '\0';
            // These guys don't have to be in object linked list, since they
            // will exist for the whole duration of the program.
            struct object_string* obj_str = new_string_move(vm, str, len);
            return (struct object*)obj_str;
        }
        default:
            fprintf(f, "Unknown tag in serialize: 0x%x", tag);
            return NULL;
    }
}

void serialize_constant_pool(FILE* f, vm_t* vm) {
    u32 len = read_4bytes_le(f);
    for (u32 i = 0; i < len; ++i) {
        struct object* obj = serialize_object(f, vm);
        if (obj == NULL) {
            fprintf(stderr, "Unable to serialize object at %u.\n", i);
            exit(-3);
        }

        write_constant_pool(&vm->const_pool, obj);
    }
}

vm_t serialize(FILE* f, u32* ep) {
    vm_t vm;
    init_vm_state(&vm);
    vm.gc.gc_off = true;
    serialize_constant_pool(f, &vm);
    *ep = read_4bytes_le(f);
    vm.gc.gc_off = false;
    return vm;
}
