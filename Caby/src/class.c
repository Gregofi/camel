#include "class.h"
#include "object.h"


struct object_class* new_class(vm_t* vm, u32 name, struct table methods) {
    struct object_class* klass = vmalloc(vm, sizeof(*klass));
    klass->name = name;
    klass->methods = methods;
    init_object(vm, &klass->object, OBJECT_CLASS);
    return klass;
}

struct object_class* as_class(struct object* object) {
    return (struct object_class*)object;
}

struct object_class* as_class_s(struct object* object) {
    if (object->type == OBJECT_CLASS) {
        return (struct object_class*)object;
    }
    return NULL;
}

struct object_instance* new_instance(vm_t* vm, struct object_class* klass, struct table members) {
    struct object_instance* instance = vmalloc(vm, sizeof(*instance));
    instance->klass = klass;
    instance->members = members;
    init_object(vm, &instance->object, OBJECT_INSTANCE);
    return instance;
}

struct object_instance* as_instance(struct object* object) {
    return (struct object_instance*)object;
}

struct object_instance* as_instance_s(struct object* object) {
    if (object->type == OBJECT_INSTANCE) {
        return (struct object_instance*)object;
    }
    return NULL;
}
