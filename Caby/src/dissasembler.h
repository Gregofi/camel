#pragma once

#include "common.h"
#include "bytecode.h"
#include "object.h"

#include <stdio.h>

void dissasemble_chunk(FILE* f, struct bc_chunk* c, const char* prefix);

size_t dissasemble_instruction(FILE* f, u8* ins);

void dissasemble_object(FILE* f, struct object* obj, bool shrt);

void disassemble_value(FILE* f, struct value v, bool shrt);

void disassemble_constant_pool(FILE* f, struct constant_pool* cp);
