#include "dissasembler.h"
#include "bytecode.h"
#include "common.h"
#include "object.h"

void dissasemble_chunk(FILE* f, struct bc_chunk* c, const char *prefix) {
    for (size_t i = 0; i < c->len;) {
        fprintf(f, "%s%lu ", prefix, i);
        i += dissasemble_instruction(f, &c->data[i]);
        fprintf(f, "\n");
    }
}

size_t dissasemble_instruction(FILE* f, u8* ins) {
    // TODO: Fix repeated code
    switch (*ins) {
        case OP_RETURN:
            fprintf(f, "RETURN");
            return 1;
        case OP_LABEL:
            fprintf(f, "LABEL");
            return 1;
        case OP_DROP:
            fprintf(f, "DROP");
            return 1;
        case OP_DUP:
            fprintf(f, "DUP");
            return 1;
        case OP_IADD:
            fprintf(f, "IADD");
            return 1;
        case OP_ISUB:
            fprintf(f, "ISUB");
            return 1;
        case OP_IMUL:
            fprintf(f, "IMUL");
            return 1;
        case OP_IDIV:
            fprintf(f, "IDIV");
            return 1;
        case OP_IREM:
            fprintf(f, "IREM");
            return 1;
        case OP_IAND:
            fprintf(f, "IAND");
            return 1;
        case OP_IOR:
            fprintf(f, "IOR");
            return 1;
        case OP_EQ:
            fprintf(f, "EQ");
            return 1;
        case OP_PUSH_BOOL:
            fprintf(f, "PUSH_BOOL %s", ins[1] == 1 ? "true" : "false");
            return 2;
        case OP_PRINT:
            fprintf(f, "PRINT args: %d", ins[1]);
            return 2;
        case OP_PUSH_SHORT:
            fprintf(f, "PUSH_SHORT %d", READ_2BYTES_BE(ins + 1));
            return 3;
        case OP_JMP_SHORT:
            fprintf(f, "JMP_SHORT %d", READ_2BYTES_BE(ins + 1));
            return 3;
        case OP_BRANCH_SHORT:
            fprintf(f, "BRANCH_SHORT %d", READ_2BYTES_BE(ins + 1));
            return 3;
        case OP_SET_LOCAL:
            fprintf(f, "GET_LOCAL %d", READ_2BYTES_BE(ins + 1));
            return 3;
        case OP_GET_LOCAL:
            fprintf(f, "SET_LOCAL %d", READ_2BYTES_BE(ins + 1));
            return 3;
        case OP_PUSH_INT:
            fprintf(f, "PUSH_INT %d", READ_4BYTES_BE(ins + 1));
            return 5;
        case OP_JMP:
            fprintf(f, "JMP %d", READ_4BYTES_BE(ins + 1));
            return 5;
        case OP_BRANCH:
            fprintf(f, "BRANCH %d", READ_4BYTES_BE(ins + 1));
            return 5;
        case OP_SET_GLOBAL:
            fprintf(f, "SET_GLOBAL %d", READ_4BYTES_BE(ins + 1));
            return 5;
        case OP_GET_GLOBAL:
            fprintf(f, "GET_GLOBAL %d", READ_4BYTES_BE(ins + 1));
            return 5;
        case OP_PUSH_LITERAL:
            fprintf(f, "PUSH_LITERAL %d", READ_4BYTES_BE(ins + 1));
            return 5;
        case OP_CALL_FUNC:
            fprintf(f, "CALL_FUNC func: %d, args: %d", READ_4BYTES_BE(ins + 1), ins[5]);
            return 6;
        default:
            fprintf(f, "UNKNOWN_INSTRUCTION 0x%x", *ins);
            return 1;
    }
}

void dissasemble_object(FILE* f, struct object* obj) {
    switch (obj->type) {
        case OBJECT_STRING:
            fprintf(f, "STRING \"%s\"\n", as_string(obj)->data);
            break;
        case OBJECT_FUNCTION: {
            struct object_function* fun = as_function(obj);
            fprintf(f, "FUNCTION arity: %d name: %u\n", fun->arity, fun->name);
            dissasemble_chunk(f, &fun->bc, " ");
            break;
        }
      break;
    }
}

void disassemble_constant_pool(FILE* f, struct constant_pool* cp) {
    for (size_t i = 0; i < cp->len; ++i) {
        fprintf(f, "%lu ", i);
        dissasemble_object(f, cp->data[i]);
    }
}
