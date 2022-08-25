#include "gc.h"
#include "vm.h"
#include "object.h"
#include "hashtable.h"

#define IS_MARKED(val) (val & 1)

static void mark_object(struct object* obj) {
    if (obj == NULL) {
        return;
    }
    obj->gc_data = 1;
#ifdef __GC_DEBUG__
    GC_LOG("Marking object %p: ", obj);
    dissasemble_object(obj);
    GC_LOG("\n");
#endif
}

static void mark_val(struct value* v) {
    if (v->type == VAL_OBJECT) {
        mark_object(v->object);
    }
}

static void mark_table(struct table* table) {
    for (size_t i = 0; i < table->capacity; ++i) {
        struct entry* e = &table->entries[i];
        mark_val(&e->key);
        // TODO
        // if (e->key.type != VAL_NONE) {
            mark_val(&e->val);
        // }
    }
}

static void mark_roots(vm_t* vm) {
    // stack
    for (size_t i = 0; i < vm->stack_len; ++i) {
        mark_val(&vm->op_stack[i]);
    }
    // globals
    mark_table(&vm->globals);
    // locals in frames
    for (size_t i = 0; i < vm->frame_len; ++i) {
        for (size_t j = 0; j < vm->frames->function->locals; ++j) {
            mark_val(&vm->frames[i].slots[j]);
        }
    }
}

void gc_collect(vm_t* vm) {
    GC_LOG("=== GC BEGIN ===\n");
    
    mark_roots(vm);

    GC_LOG("=== GC END ===\n\n");
}