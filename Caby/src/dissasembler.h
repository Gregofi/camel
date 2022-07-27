#pragma once

#include "common.h"
#include "bytecode.h"

#include <stdio.h>

void dissasemble_chunk(FILE* f, struct bc_chunk* c);

size_t dissasemble_instruction(FILE* f, u8* ins);
