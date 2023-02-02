#include "gc.h"
#include "class.h"
#include "bytecode.h"
#include "common.h"
#include "vm.h"
#include "object.h"
#include "hashtable.h"
#include "memory.h"
#include "dissasembler.h"

#define IS_MARKED(val) (((val) & 1) == 1)

void init_gc(struct gc_state* gc) {
    gc->wl_capacity = 0;
    gc->wl_count = 0;
    gc->worklist = NULL;
    gc->next_gc = GC_THRESHOLD_START;
    gc->gc_off = false;
}

void free_gc(struct gc_state* gc) {
    free(gc->worklist);
    init_gc(gc);
}

static void mark_val(struct gc_state*, struct value*);

static void mark_table(struct gc_state* gc, struct table* table) {
    for (size_t i = 0; i < table->capacity; ++i) {
        struct entry* e = &table->entries[i];
        mark_val(gc, &e->key);
        // TODO: Maybe not necessary
        if (e->key.type != VAL_NONE) {
            mark_val(gc, &e->val);
        }
    }
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

    if (obj->type == OBJECT_INSTANCE) {
        struct object_instance* instance = as_instance(obj);
        mark_table(gc, &instance->members);
    }
#ifdef __GC_DEBUG__
    GC_LOG("Marking object %p: ", obj);
    dissasemble_object(stderr, obj);
    GC_LOG("\n");
#endif
}

static void mark_val(struct gc_state* gc, struct value* v) {
    if (v->type == VAL_OBJECT) {
        mark_object(gc, v->object);
    }
}

static void mark_roots(vm_t* vm) {
    // constant pool
    for (size_t i = 0; i < vm->const_pool.len; ++i) {
        mark_object(&vm->gc, vm->const_pool.data[i]);
    }
    
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

static void close_obj(vm_t* vm, struct object* obj) {
    switch (obj->type) {
        case OBJECT_FUNCTION:
        case OBJECT_STRING:
        case OBJECT_NATIVE:
            break;
        case OBJECT_CLASS: {
            struct object_class* c = as_class(obj);
            mark_table(&vm->gc, &c->methods);
        }
        case OBJECT_INSTANCE: {
            struct object_instance* c = as_instance(obj);
            mark_table(&vm->gc, &c->members);
        }
    }
}

static void trace_references(vm_t* vm) {
    struct gc_state* gc = &vm->gc;
    while (gc->wl_count > 0) {
        struct object* obj = vm->gc.worklist[--gc->wl_count];
        close_obj(vm, obj);
    }
}

static void sweep(vm_t* vm) {
    struct object** obj = &vm->objects;
    while (*obj) {
        if (IS_MARKED((*obj)->gc_data)) {
            (*obj)->gc_data = 0;
            obj = &(*obj)->next;
        } else {
            struct object* unreached = *obj;
            *obj = (*obj)->next;
            free_object(unreached);
        }
    }
}

void gc_collect(vm_t* vm) {
    if (vm->gc.gc_off) {
        GC_LOG("Collection was called but GC is turned off\n");
        return;
    }
    GC_LOG("=== GC BEGIN ===\n");
    size_t before = mem_taken();
    GC_LOG("Taken memory: %lu/%luB\n", before, mem_total());

    mark_roots(vm);
    trace_references(vm);
    GC_LOG("Begin sweeping\n");
    sweep(vm);

    size_t after = mem_taken();
    GC_LOG("Taken memory: %lu/%luB\n", after, mem_total());
    GC_LOG("Difference: %luB\n", before - after);
    GC_LOG("=== GC END ===\n\n");
}
