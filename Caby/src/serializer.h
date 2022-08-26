#pragma once

#include "object.h"
#include "vm.h"
#include "bytecode.h"
#include "common.h"

#include <stdio.h>

enum object_tag {
    TAG_FUNCTION = 0x00,
    TAG_STRING = 0x01,
};

u32 read_4bytes_be(FILE* f);

u16 read_2bytes_be(FILE* f);

void serialize_instruction(FILE* f, struct bc_chunk* c);

struct object* serialize_object(FILE* f, vm_t* vm);

void serialize_constant_pool(FILE* f, vm_t* vm);

vm_t serialize(FILE* f, u32* ep);
