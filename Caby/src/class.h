#include "object.h"
#include "hashtable.h"

struct object_class {
    struct object object;
    struct object_string* name;
    struct table methods;
};

struct object_instance {
    struct object object;
    struct object_class* klass;
    struct object_string* name;
    struct table members;
};

/// Returns new class, takes ownership of 'name'.
struct object_class* new_class(vm_t* vm, struct object_string* name, struct table methods);

struct object_class* as_class(struct object* object);

struct object_class* as_class_s(struct object* object);

/// Returns new class instance, takes ownership of 'name', but not 'klass'.
struct object_instance* new_instance(vm_t* vm, struct object_instance* klass, struct object_string* name, struct table members);

struct object_instance* as_instance(struct object* object);

struct object_instance* as_instance_s(struct object* object);
