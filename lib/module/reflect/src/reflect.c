#include <assert.h>

#include <clox/native/reflect/reflect.h>

bool hasField(ObjInstance* instance, ObjString* key) {
    return tableGet(&instance->fields, key, NULL);
}

ValueResult getField(ObjInstance* instance, ObjString* key) {
    ValueResult result;
    result.success = tableGet(&instance->fields, key, &result.value);

    if (!result.success) {
        result.exception = NATIVE_ERROR("Instance doesn't have the requested field.");
    }

    return result;
}

void setField(ObjInstance* instance, ObjString* key, Value value) {
    tableSet(&instance->fields, key, value);
}

void deleteField(ObjInstance* instance, ObjString* key) {
    tableDelete(&instance->fields, key);
}

ObjArray* fieldNames(ObjInstance* instance) {
    int refScope = referenceScope();
    ObjArray* arr = newArray();

    pushReference(OBJ_VAL((Obj*) arr));

    for(TableIterator it = newTableIterator(&instance->fields);
        !it.done;
        advanceTableIterator(&it)
    ) {
        ObjString* key = getKeyTableIterator(&it);
        writeValueArray(&arr->array, OBJ_VAL((Obj*) key));
    }

    resetReferences(refScope);
    return arr;
}

