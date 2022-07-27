#include "dissasembler.h"

void dissasemble_chunk(FILE* f, struct bc_chunk* c) {
    for (size_t i = 0; i < c->len; ++ i) {
        fprintf(f, "%lu ", i);
        dissasemble_instruction(f, &c->data[i]);
        fprintf(f, "\n");
        fputs("", f);
    }
}

size_t dissasemble_instruction(FILE* f, u8* ins) {
    switch (*ins) {
            case OP_RETURN:
                fprintf(f, "OP_RETURN");
                return 1;
            default:
                fprintf(f, "UNKNOWN_INSTRUCTION");
                return 1;
        }
}
