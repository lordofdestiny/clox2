//
// Created by djumi on 11/10/2023.
//

#include "../h/native.h"


#include <stdlib.h>
#include <time.h>

#define NATIVE_ERROR(msg) (OBJ_VAL((Obj*) copyString(msg, strlen(msg))))

static bool hasFieldNative(int argCount, Value *args) {
    if (!IS_INSTANCE(args[0])) {
        args[-1] = NATIVE_ERROR("Function 'hasField' expects an instance as the first argument.");
        return false;
    }
    if (!IS_INSTANCE(args[1])) {
        args[-1] = NATIVE_ERROR("Function 'hasField' expects a string as the second argument.");
        return false;
    }
    ObjInstance *instance = AS_INSTANCE(args[0]);
    ObjString *key = AS_STRING(args[1]);
    Value dummy;

    args[-1] = BOOL_VAL(tableGet(&instance->fields, key, &dummy));
    return true;
}

static bool getFieldNative(int argCount, Value *args) {
    if (!IS_INSTANCE(args[0])) {
        args[-1] = NATIVE_ERROR("Function 'getField' expects an instance as the first argument.");
        return false;
    }
    if (!IS_INSTANCE(args[1])) {
        args[-1] = NATIVE_ERROR("Function 'getField' expects a string as the second argument.");
        return false;
    }
    ObjInstance *instance = AS_INSTANCE(args[0]);
    ObjString *key = AS_STRING(args[1]);
    Value value;

    if (!tableGet(&instance->fields, key, &value)) {
        args[-1] = NATIVE_ERROR("Instance doesn't have the requested field.");
        return false;
    }

    args[-1] = value;
    return true;
}

static bool setFieldNative(int argCount, Value *args) {
    if (!IS_INSTANCE(args[0])) {
        args[-1] = NATIVE_ERROR("Function 'setField' expects an instance as the first argument.");
        return false;
    }
    if (!IS_INSTANCE(args[1])) {
        args[-1] = NATIVE_ERROR("Function 'setField' expects a string as the second argument.");
        return false;
    }
    ObjInstance *instance = AS_INSTANCE(args[0]);
    tableSet(&instance->fields, AS_STRING(args[1]), args[2]);
    args[-1] = args[2];
    return true;
}

static bool deleteFieldNative(int argCount, Value *args) {
    if (!IS_INSTANCE(args[0])) {
        args[-1] = NATIVE_ERROR("Function 'deleteField' expects an instance as the first argument.");
        return false;
    }
    if (!IS_INSTANCE(args[1])) {
        args[-1] = NATIVE_ERROR("Function 'deleteField' expects a string as the second argument.");
        return false;
    }
    ObjInstance *instance = AS_INSTANCE(args[0]);
    tableDelete(&instance->fields, AS_STRING(args[1]));
    args[-1] = NIL_VAL;
    return true;
}

static bool clockNative(int argCount, Value *args) {
    args[-1] = NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
    return true;
}

static bool exitNative(int argCount, Value *args) {
    if (argCount > 1) {
        args[-1] = NATIVE_ERROR("Exit takes zero arguments, or one argument that is a number");
        return false;
    }
    if (argCount == 0) {
        exit(0);
    }
    if (!IS_NUMBER(args[0])) {
        args[-1] = NATIVE_ERROR("Exit code must be a number.");
        return false;
    }
    int exitCode = (int) AS_NUMBER(args[0]);
    exit(exitCode);
    args[-1] = NIL_VAL;
    return true;
}

NativeMethodDef nativeMethods[] = {
        {"hasField",    2,  hasFieldNative},
        {"getField",    2,  getFieldNative},
        {"setField",    3,  setFieldNative},
        {"deleteField", 2,  deleteFieldNative},
        {"clock",       0,  clockNative},
        {"exit",        -1, exitNative},
        {NULL,          0, NULL}
};
