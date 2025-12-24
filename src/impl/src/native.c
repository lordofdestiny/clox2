#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <ctype.h>
#include <setjmp.h>

#include "native.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

static bool hasFieldNative([[maybe_unused]] int argCount, Value* implicit, Value* args) {
    if (!IS_INSTANCE(args[0])) {
        *implicit = NATIVE_ERROR("Function 'hasField' expects an instance as the first argument.");
        return false;
    }
    if (!IS_STRING(args[1])) {
        *implicit = NATIVE_ERROR("Function 'hasField' expects a string as the second argument.");
        return false;
    }
    ObjInstance* instance = AS_INSTANCE(args[0]);
    ObjString* key = AS_STRING(args[1]);
    Value dummy;

    *implicit = BOOL_VAL(tableGet(&instance->fields, key, &dummy));
    return true;
}

static bool getFieldNative([[maybe_unused]] int argCount, Value* implicit, Value* args) {
    if (!IS_INSTANCE(args[0])) {
        *implicit = NATIVE_ERROR("Function 'getField' expects an instance as the first argument.");
        return false;
    }
    if (!IS_STRING(args[1])) {
        *implicit = NATIVE_ERROR("Function 'getField' expects a string as the second argument.");
        return false;
    }
    ObjInstance* instance = AS_INSTANCE(args[0]);
    ObjString* key = AS_STRING(args[1]);
    Value value;

    if (!tableGet(&instance->fields, key, &value)) {
        *implicit = NATIVE_ERROR("Instance doesn't have the requested field.");
        return false;
    }

    *implicit = value;
    return true;
}

static bool setFieldNative([[maybe_unused]] int argCount, Value* implicit, Value* args) {
    if (!IS_INSTANCE(args[0])) {
        *implicit = NATIVE_ERROR("Function 'setField' expects an instance as the first argument.");
        return false;
    }
    if (!IS_STRING(args[1])) {
        *implicit = NATIVE_ERROR("Function 'setField' expects a string as the second argument.");
        return false;
    }
    ObjInstance* instance = AS_INSTANCE(args[0]);
    tableSet(&instance->fields, AS_STRING(args[1]), args[2]);
    *implicit = args[2];
    return true;
}

static bool deleteFieldNative([[maybe_unused]] int argCount, Value* implicit, Value* args) {
    if (!IS_INSTANCE(args[0])) {
        *implicit = NATIVE_ERROR(
            "Function 'deleteField' expects an instance as the first argument.");
        return false;
    }
    if (!IS_STRING(args[1])) {
        *implicit = NATIVE_ERROR("Function 'deleteField' expects a string as the second argument.");
        return false;
    }
    ObjInstance* instance = AS_INSTANCE(args[0]);
    tableDelete(&instance->fields, AS_STRING(args[1]));
    *implicit = NIL_VAL;
    return true;
}

static bool clockNative([[maybe_unused]] int argCount, Value* implicit,[[maybe_unused]]  Value* args) {
    *implicit = NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
    return true;
}

static bool exitNative(int argCount, Value* implicit, Value* args) {
    if (argCount > 1) {
        *implicit = NATIVE_ERROR("Exit takes zero arguments, or one argument that is a number");
        return false;
    }
    if (argCount == 0) {
        vm.exit_code = 0;
        longjmp(vm.exit_state, 1);
    }
    if (!IS_NUMBER(args[0])) {
        *implicit = NATIVE_ERROR("Exit code must be a number.");
        return false;
    }

    vm.exit_code = (int) AS_NUMBER(args[0]);
    longjmp(vm.exit_state, 1);
}

NativeMethodDef nativeMethods[] = {
    {"hasField", 2, hasFieldNative},
    {"getField", 2, getFieldNative},
    {"setField", 3, setFieldNative},
    {"deleteField", 2, deleteFieldNative},
    {"clock", 0, clockNative},
    {"exit", -1, exitNative},
    {NULL, 0, NULL}
};

bool initExceptionNative(int argCount, Value* implicit, Value* args) {
    if (argCount > 1) {
        *implicit = NATIVE_ERROR("Exit takes either 0 arguments or one a string.");
        return false;
    }
    ObjInstance* exception = AS_INSTANCE((*implicit));
    if (argCount == 1) {
        if (!IS_STRING(args[0])) {
            *implicit = NATIVE_ERROR("Expected a string as an argument");
            return false;
        }
        tableSet(&exception->fields, copyString("message", 7), args[0]);
    } else {
        tableSet(&exception->fields, copyString("message", 7), NIL_VAL);
    }
    *implicit = OBJ_VAL((Obj *) exception);
    return true;
}

void tryUnpack(Value* valuePtr) {
    if (IS_INSTANCE(*valuePtr)) {
        const Value unpacked = AS_INSTANCE((*valuePtr))->this_;
        if (!IS_INSTANCE(unpacked)) {
            *valuePtr = unpacked;
        }
    }
}

bool initNumberNative(int argCount, Value* implicit, Value* args) {
    if (argCount != 1) {
        *implicit = NATIVE_ERROR("Number constructor takes 1 argument.");
        return false;
    }
    Value value = args[0]; // Argument
    tryUnpack(&value);
    ObjInstance* instance = AS_INSTANCE((*implicit)); // This

    if (!IS_NUMBER(value) &&
        !IS_STRING(value) &&
        !(IS_INSTANCE(value)
          && !IS_NUMBER(AS_INSTANCE(value)->this_)
          && !IS_STRING(AS_INSTANCE(value)->this_))) {
        *implicit = NATIVE_ERROR("Value cannot be converted to a number");
        return false;
    }

    if (IS_NUMBER(value) || (IS_INSTANCE(value) && IS_NUMBER(AS_INSTANCE(value)->this_))) {
        instance->this_ = IS_NUMBER(value)
                              ? value
                              : AS_INSTANCE(value)->this_;
        return true;
    }

    if (IS_STRING(value) || (IS_INSTANCE(value) && IS_STRING(AS_INSTANCE(value)->this_))) {
        const char* chars = AS_CSTRING(
            IS_STRING(value)
            ? value
            : AS_INSTANCE(value)->this_);
        char* end;
        const int oldErrno = errno;
        errno = 0;
        const double val = strtod(chars, &end);
        if (errno == 0) {
            errno = oldErrno;
        }

        if (chars == end) {
            *implicit = NATIVE_ERROR("Invalid number literal.");
            return false;
        }

        if ((errno == ERANGE && fabs(val) == HUGE_VAL)
            || (errno == ERANGE && fabs(val) == DBL_MIN)
            || end == chars) {
            *implicit = NATIVE_ERROR("Invalid number literal.");
            return false;
        }
        instance->this_ = NUMBER_VAL(val);
        return true;
    }

    return true;
}

bool toPrecisionNative([[maybe_unused]] int argCount, Value* implicit, Value* args) {
    const ObjInstance* instance = AS_INSTANCE((*implicit)); // This
    if (!IS_NUMBER(args[0])) {
        *implicit = NATIVE_ERROR("Number of digits must be a number!");
        return false;
    }
    const int decimals = (int) AS_NUMBER(args[0]);
    const double value = AS_NUMBER(instance->this_);
    const int len = snprintf(NULL, 0, "%.*lf", decimals, value);
    char* buffer = ALLOCATE(char, len + 1);
    snprintf(buffer, len + 1, "%.*lf", decimals, value);
    *implicit = OBJ_VAL((Obj*) takeString(buffer, len));
    return true;
}

bool initBooleanNative(int argCount, Value* implicit, Value* args) {
    if (argCount != 1) {
        *implicit = NATIVE_ERROR("Number constructor takes 1 argument.");
        return false;
    }
    Value value = args[0]; // argument
    tryUnpack(&value);
    ObjInstance* instance = AS_INSTANCE((*implicit)); // This

    if (!IS_NIL(value) && !IS_NUMBER(value) &&
        !IS_BOOL(value) && !IS_STRING(value) &&
        !(IS_INSTANCE(value)
          && !IS_NUMBER(AS_INSTANCE(value)->this_)
          && !IS_STRING(AS_INSTANCE(value)->this_)
          && !IS_BOOL(AS_INSTANCE(value)->this_))) {
        *implicit = NATIVE_ERROR("Value cannot be converted to a boolean");
        return false;
    }

    if (IS_NIL(value)) {
        instance->this_ = FALSE_VAL;
        return true;
    }

    if (IS_NUMBER(value) || (IS_INSTANCE(value) && IS_NUMBER(AS_INSTANCE(value)->this_))) {
        const int b = (int) (IS_NUMBER(value)
                                 ? AS_NUMBER(value)
                                 : AS_NUMBER(AS_INSTANCE(value)->this_));

        instance->this_ = b ? TRUE_VAL : FALSE_VAL;
        return true;
    }

    if (IS_BOOL(value) || (IS_INSTANCE(value) && IS_BOOL(AS_INSTANCE(value)->this_))) {
        instance->this_ = IS_BOOL(value)
                              ? value
                              : AS_INSTANCE(value)->this_;
        return true;
    }

    if (IS_STRING(value) || (IS_INSTANCE(value) && IS_STRING(AS_INSTANCE(value)->this_))) {
        const char* chars = AS_CSTRING(
            IS_STRING(value)
            ? value
            : AS_INSTANCE(value)->this_);
        const int len = (int) strlen(chars);
        if (len > 5) {
            *implicit = NATIVE_ERROR("Invalid boolean literal.");
            return false;
        }
        char text[6];
        for (int i = 0; i < len; i++) {
            text[i] = toupper(chars[i]);
        }

        text[len] = '\0';
        if (strcmp(text, "FALSE") == 0) {
            instance->this_ = FALSE_VAL;
        } else if (strcmp(text, "TRUE") == 0) {
            instance->this_ = TRUE_VAL;
        } else {
            *implicit = NATIVE_ERROR("Invalid boolean literal.");
            return false;
        }
    }

    return true;
}

bool initStringNative(int argCount, Value* implicit, Value* args) {
    if (argCount != 1) {
        *implicit = NATIVE_ERROR("Number constructor takes 1 argument.");
        return false;
    }
    Value value = args[0]; // argument
    tryUnpack(&value);
    ObjInstance* instance = AS_INSTANCE((*implicit)); // This

    if (!IS_NUMBER(value) &&
        !IS_BOOL(value) && !IS_STRING(value) &&
        !(IS_INSTANCE(value)
          && !IS_NUMBER(AS_INSTANCE(value)->this_)
          && !IS_STRING(AS_INSTANCE(value)->this_)
          && !IS_BOOL(AS_INSTANCE(value)->this_))) {
        *implicit = NATIVE_ERROR("Value cannot be converted to a string");
        return false;
    }

    if (IS_NUMBER(value) || (IS_INSTANCE(value) && IS_NUMBER(AS_INSTANCE(value)->this_))) {
        const double x = IS_NUMBER(value)
                             ? AS_NUMBER(value)
                             : AS_NUMBER(AS_INSTANCE(value)->this_);

        const int len = snprintf(NULL, 0, "%g", x);

        char* chars = ALLOCATE(char, len + 1);
        snprintf(chars, len + 1, "%g", x);

        instance->this_ = OBJ_VAL((Obj*) takeString(chars, len));
        tableSet(&instance->fields, copyString("length", 6), NUMBER_VAL(len));

        return true;
    }

    if (IS_BOOL(value) || (IS_INSTANCE(value) && IS_BOOL(AS_INSTANCE(value)->this_))) {
        const bool b = AS_BOOL(
            IS_BOOL(value)
            ? value
            : AS_INSTANCE(value)->this_);

        const char* str = b ? "true" : "false";
        const int len = b ? 4 : 5;
        instance->this_ = OBJ_VAL((Obj*) copyString(str, len));
        tableSet(&instance->fields, copyString("length", 6), NUMBER_VAL(len));
        return true;
    }

    if (IS_STRING(value) || (IS_INSTANCE(value) && IS_STRING(AS_INSTANCE(value)->this_))) {
        const ObjString* str = AS_STRING(
            IS_STRING(value)
            ? value
            : AS_INSTANCE(value)->this_);

        instance->this_ = OBJ_VAL((Obj*) str);
        tableSet(&instance->fields, copyString("length", 6), NUMBER_VAL(str->length));
        return true;
    }

    return true;
}

bool initArrayNative(int argCount, Value* implicit, Value* args) {
    if (argCount > 1) {
        *implicit = NATIVE_ERROR("Array constructor takes 1 argument.");
        return false;
    }

    if (argCount == 0) {
        *implicit = OBJ_VAL((Obj*) newArray());
        return true;
    }

    Value value = args[0]; // argument
    tryUnpack(&value);
    ObjInstance* instance = AS_INSTANCE((*implicit)); // This

    if (!IS_ARRAY(value) &&
        !IS_NUMBER(value) &&
        !(IS_INSTANCE(value)
          && !IS_ARRAY(AS_INSTANCE(value)->this_)
          && !IS_NUMBER(AS_INSTANCE(value)->this_))) {
        *implicit = NATIVE_ERROR("Value cannot be converted to an array");
        return false;
    }

    if (IS_NUMBER(value) || (IS_INSTANCE(value) && IS_NUMBER(AS_INSTANCE(value)->this_))) {
        const int len = (int) AS_NUMBER(
            IS_NUMBER(value)
            ? value
            : AS_INSTANCE(value)->this_);
        ObjArray* array_ = newArray();
        valueInitValueArray(&array_->array, NIL_VAL, len);
        instance->this_ = OBJ_VAL((Obj*) array_);
        tableSet(&instance->fields, copyString("length", 6), NUMBER_VAL(len));
        return true;
    }

    if (IS_ARRAY(value)) {
        ObjArray* array_ = AS_ARRAY(value);
        instance->this_ = OBJ_VAL((Obj*) array_);
        tableSet(&instance->fields, copyString("length", 6), NUMBER_VAL(array_->array.count));
        return true;
    }

    return false;
}

bool appendArrayNative([[maybe_unused]] int argCount, Value* implicit, Value* args) {
    const Value value = args[0]; // Argument
    const ObjInstance* instance = AS_INSTANCE((*implicit)); // This
    ObjArray* array = AS_ARRAY(instance->this_);

    writeValueArray(&array->array, value);
    *implicit = NIL_VAL;

    return true;
}

bool popArrayNative([[maybe_unused]] int argCount, Value* implicit, [[maybe_unused]] Value* args) {
    const ObjInstance* instance = AS_INSTANCE((*implicit)); // This
    ObjArray* array = AS_ARRAY(instance->this_);
    ValueArray* va = &array->array;

    *implicit = va->values[--va->count];

    return true;
}
