#include <assert.h>

#include <clox/native/reflect/reflect.h>

bool hasField(ObjInstance* instance, ObjString* key) {
    return tableGet(&instance->fields, key, NULL);
}

ValueResult getField(ObjInstance* instance, ObjString* key) {
    Value value;
    if(!tableGet(&instance->fields, key, &value)) {
        return (ValueResult) {
            .success = false,
            .exception = NATIVE_ERROR("Instance doesn't have the requested field.")
        };
    }

    return (ValueResult) {
        .success = true,
        .value = value
    };
}

void setField(ObjInstance* instance, ObjString* key, Value value) {
    tableSet(&instance->fields, key, value);
}

void deleteField(ObjInstance* instance, ObjString* key) {
    tableDelete(&instance->fields, key);
}

ObjArray* fieldNames(ObjInstance* instance) {
    ObjArray* arr = newArray();
    PUSH_OBJ(arr);

    for(TableIterator it = newTableIterator(&instance->fields);
        !it.done;
        advanceTableIterator(&it)
    ) {
        ObjString* key = getKeyTableIterator(&it);
        writeValueArray(&arr->array, OBJ_VAL((Obj*) key));
    }

    pop();
    return arr;
}

