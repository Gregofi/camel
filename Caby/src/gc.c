#include "gc.h"
#include "common.h"
#include "vm.h"
#include "object.h"
#include "hashtable.h"
#include "memory.h"

#define IS_MARKED(val) ((val & 1) == 1)

void init_gc(struct gc_state* gc) {
    memset(gc, 0, sizeof(*gc));
}

void free_gc(struct gc_state* gc) {
    free(gc->worklist);
    init_gc(gc);
}

static void mark_object(struct gc_state* gc, struct object* obj) {
    if (obj == NULL || IS_MARKED(obj->gc_data)) {
        return;
    }
    obj->gc_data = 1;
    gc->worklist = handle_capacity(gc->worklist, gc->wl_count,
                                   &gc->wl_capacity, sizeof(*gc->worklist));
    if (gc->worklist == NULL) {
        fprintf(stderr, "Allocation failed for internal GC data, aborting\n");
        exit(-11);
    }
    gc->worklist[gc->wl_count++] = obj;
#ifdef __GC_DEBUG__
    GC_LOG("Marking object %p: ", obj);
    dissasemble_object(obj);
    GC_LOG("\n");
#endif
}

static void mark_val(struct gc_state* gc, struct value* v) {
    if (v->type == VAL_OBJECT) {
        mark_object(gc, v->object);
    }
}

static void mark_table(struct gc_state* gc, struct table* table) {
    for (size_t i = 0; i < table->capacity; ++i) {
        struct entry* e = &table->entries[i];
        mark_val(gc, &e->key);
        // TODO
        // if (e->key.type != VAL_NONE) {
            mark_val(gc, &e->val);
        // }
    }
}

static void mark_roots(vm_t* vm) {
    // stack
    for (size_t i = 0; i < vm->stack_len; ++i) {
        mark_val(&vm->gc, &vm->op_stack[i]);
    }
    // globals
    mark_table(&vm->gc, &vm->globals);
    // locals in frames
    for (size_t i = 0; i < vm->frame_len; ++i) {
        for (size_t j = 0; j < vm->frames->function->locals; ++j) {
            mark_val(&vm->gc, &vm->frames[i].slots[j]);
        }
    }
}

static void close_obj(struct object* obj) {
    switch (obj->type) {
        case OBJECT_STRING:
        case OBJECT_NATIVE:
        // We don't have to handle functions because they are not in the
        // GC linked list of objects to be collected.
        case OBJECT_FUNCTION:
      break;
    }
}

static void trace_references(struct gc_state* gc) {
    while (gc->wl_count > 0) {
        struct object* obj = gc->worklist[--gc->wl_count];
        close_obj(obj);
    }
}

static void sweep(vm_t* vm) {
    struct object* prev = NULL;
    struct object* obj  = vm->objects;
    while (obj != NULL) {
        if (IS_MARKED(obj->gc_data)) {
            obj->gc_data = 0;
            prev = obj;
            obj = obj->next;
        } else {
            struct object* unreached = obj;
            obj = obj->next;
            if (prev != NULL) {
                prev->next = obj;
            } else {
                vm->objects = obj;
            }

            free_object(obj);
        }
    }
}

void gc_collect(vm_t* vm) {
    GC_LOG("=== GC BEGIN ===\n");
    
    mark_roots(vm);
    trace_references(&vm->gc);
    sweep(vm);

    GC_LOG("=== GC END ===\n\n");
}