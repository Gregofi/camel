#pragma once

#include "common.h"
#include "bytecode.h"

#include <stdio.h>

void dissasemble_chunk(FILE* f, struct bc_chunk* c, const char* prefix);

size_t dissasemble_instruction(FILE* f, u8* ins);

void dissasemble_object(FILE* f, struct object* obj);

void disassemble_constant_pool(FILE* f, struct constant_pool* cp);
