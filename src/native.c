//
// Created by djumi on 11/10/2023.
//

#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <ctype.h>

#include "../h/native.h"
#include "../h/memory.h"
#include "../h/object.h"

static bool hasFieldNative(int argCount, Value *args) {
    if (!IS_INSTANCE(args[0])) {
        args[-1] = NATIVE_ERROR("Function 'hasField' expects an instance as the first argument.");
        return false;
    }
    if (!IS_STRING(args[1])) {
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
    if (!IS_STRING(args[1])) {
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
    if (!IS_STRING(args[1])) {
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
    if (!IS_STRING(args[1])) {
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

bool initExceptionNative(int argCount, Value *args) {
    if (argCount > 1) {
        args[-1] = NATIVE_ERROR("Exit takes either 0 arguments or one a string.");
        return false;
    }
    ObjInstance *exception = AS_INSTANCE(args[-1]);
    if (argCount == 1) {
        if (!IS_STRING(args[0])) {
            args[-1] = NATIVE_ERROR("Expected a string as an argument");
            return false;
        }
        tableSet(&exception->fields, copyString("message", 7), OBJ_VAL(args[0]));
    } else {
        tableSet(&exception->fields, copyString("message", 7), NIL_VAL);
    }
    args[-1] = OBJ_VAL((Obj *) exception);
    return true;
}

bool initNumberNative(int argCount, Value *args) {
    if (argCount != 1) {
        args[-1] = NATIVE_ERROR("Number constructor takes 1 argument.");
        return false;
    }
    Value value = args[0]; // Argument
    ObjInstance *instance = AS_INSTANCE(args[-1]); // This

    if (!IS_NUMBER(value) &&
        !IS_STRING(value) &&
        !(IS_INSTANCE(value)
          && !IS_NUMBER(AS_INSTANCE(value)->this_)
          && !IS_STRING(AS_INSTANCE(value)->this_))) {
        args[-1] = NATIVE_ERROR("Value cannot be converted to a number");
        return false;
    }

    if (IS_NUMBER(value) || IS_INSTANCE(value) && IS_NUMBER(AS_INSTANCE(value)->this_)) {
        instance->this_ = IS_NUMBER(value)
                          ? value
                          : AS_INSTANCE(value)->this_;
        return true;
    }

    if (IS_STRING(value) || IS_INSTANCE(value) && IS_STRING(AS_INSTANCE(value)->this_)) {
        const char *chars = AS_CSTRING(IS_STRING(value)
                                       ? value
                                       : AS_INSTANCE(value)->this_);
        char *end;
        int oldErrno = 0;
        errno = 0;
        double val = strtod(chars, &end);
        if (errno == 0) {
            errno = oldErrno;
        }

        if (chars == end) {
            args[-1] = NATIVE_ERROR("Invalid number literal.");
            return false;
        }

        if (errno == ERANGE && fabs(val) == HUGE_VAL
            || errno == ERANGE && fabs(val) == DBL_MIN
            || end == chars) {
            args[-1] = NATIVE_ERROR("Invalid number literal.");
            return false;
        }
        instance->this_ = NUMBER_VAL(val);
        return true;
    }

    return true;
}

bool toPrecisionNative(int argCount, Value *args) {
    ObjInstance *instance = AS_INSTANCE(args[-1]); // This
    if (!IS_NUMBER(args[0])) {
        args[-1] = NATIVE_ERROR("Number of digits must be a number!");
        return false;
    }
    int decimals = (int) AS_NUMBER(args[0]);
    double value = AS_NUMBER(instance->this_);
    int len = snprintf(NULL, 0, "%.*lf", decimals, value);
    char *buffer = ALLOCATE(char, len + 1);
    snprintf(buffer, len + 1, "%.*lf", decimals, value);
    args[-1] = OBJ_VAL(takeString(buffer, len));
    return true;
}

bool initBooleanNative(int argCount, Value *args) {
    if (argCount != 1) {
        args[-1] = NATIVE_ERROR("Number constructor takes 1 argument.");
        return false;
    }
    Value value = args[0]; // argument
    ObjInstance *instance = AS_INSTANCE(args[-1]); // This

    if (!IS_NIL(value) && !IS_NUMBER(value) &&
        !IS_BOOL(value) && !IS_STRING(value) &&
        !(IS_INSTANCE(value)
          && !IS_NUMBER(AS_INSTANCE(value)->this_)
          && !IS_STRING(AS_INSTANCE(value)->this_)
          && !IS_BOOL(AS_INSTANCE(value)->this_))) {
        args[-1] = NATIVE_ERROR("Value cannot be converted to a boolean");
        return false;
    }

    if (IS_NIL(value)) {
        instance->this_ = FALSE_VAL;
        return true;
    }

    if (IS_NUMBER(value) || IS_INSTANCE(value) && IS_NUMBER(AS_INSTANCE(value)->this_)) {
        int b = (int) (IS_NUMBER(value)
                       ? AS_NUMBER(value)
                       : AS_INSTANCE(value)->this_);

        instance->this_ = b ? TRUE_VAL : FALSE_VAL;
        return true;
    }

    if (IS_BOOL(value) || IS_INSTANCE(value) && IS_BOOL(AS_INSTANCE(value)->this_)) {
        instance->this_ = IS_BOOL(value)
                          ? value
                          : AS_INSTANCE(value)->this_;
        return true;
    }

    if (IS_STRING(value) || IS_INSTANCE(value) && IS_STRING(AS_INSTANCE(value)->this_)) {
        const char *chars = AS_CSTRING(IS_STRING(value)
                                       ? value
                                       : AS_INSTANCE(value)->this_);
        int len = (int) strlen(chars);
        if (len > 5) {
            args[-1] = NATIVE_ERROR("Invalid boolean literal.");
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
            args[-1] = NATIVE_ERROR("Invalid boolean literal.");
            return false;
        }
    }

    return true;
}

bool initStringNative(int argCount, Value *args) {
    if (argCount != 1) {
        args[-1] = NATIVE_ERROR("Number constructor takes 1 argument.");
        return false;
    }
    Value value = args[0]; // argument
    ObjInstance *instance = AS_INSTANCE(args[-1]); // This

    if (!IS_NUMBER(value) &&
        !IS_BOOL(value) && !IS_STRING(value) &&
        !(IS_INSTANCE(value)
          && !IS_NUMBER(AS_INSTANCE(value)->this_)
          && !IS_STRING(AS_INSTANCE(value)->this_)
          && !IS_BOOL(AS_INSTANCE(value)->this_))) {
        args[-1] = NATIVE_ERROR("Value cannot be converted to a string");
        return false;
    }

    if (IS_NUMBER(value) || IS_INSTANCE(value) && IS_NUMBER(AS_INSTANCE(value)->this_)) {
        double x = (IS_NUMBER(value)
                    ? AS_NUMBER(value)
                    : AS_INSTANCE(value)->this_);

        int len = snprintf(NULL, 0, "%g", x);

        char *chars = ALLOCATE(char, len + 1);
        snprintf(chars, len + 1, "%g", x);

        instance->this_ = OBJ_VAL(takeString(chars, len));
        tableSet(&instance->fields, copyString("length", 6), NUMBER_VAL(len));

        return true;
    }

    if (IS_BOOL(value) || IS_INSTANCE(value) && IS_BOOL(AS_INSTANCE(value)->this_)) {
        bool b = AS_BOOL(IS_BOOL(value)
                          ? value
                          : AS_INSTANCE(value)->this_);

        if (b) {
            instance->this_ = OBJ_VAL(copyString("true", 4));
            tableSet(&instance->fields, copyString("length", 6), NUMBER_VAL(4));
        } else {
            instance->this_ = OBJ_VAL(copyString("false", 5));
            tableSet(&instance->fields, copyString("length", 6), NUMBER_VAL(5));
        }
        return true;
    }

    if (IS_STRING(value) || IS_INSTANCE(value) && IS_STRING(AS_INSTANCE(value)->this_)) {
        ObjString *str = AS_STRING(IS_STRING(value)
                                   ? value
                                   : AS_INSTANCE(value)->this_);

        // Possible place of leakage during long runtime
        // due to interning avoidance
        instance->this_ = OBJ_VAL(copyString(
                str->chars, str->length
        ));
        tableSet(&instance->fields, copyString("length", 6), NUMBER_VAL(str->length));
        return true;
    }

    return true;
}

bool initArrayNative(int argCount, Value *args) {
    if (argCount > 1) {
        args[-1] = NATIVE_ERROR("Array constructor takes 1 argument.");
        return false;
    }

    if (argCount == 0) {
        args[-1] = OBJ_VAL(newArray());
        return true;
    }

    Value value = args[0]; // argument
    ObjInstance *instance = AS_INSTANCE(args[-1]); // This

    if (!IS_ARRAY(value) &&
        !IS_NUMBER(value) &&
        !(IS_INSTANCE(value)
          && !IS_ARRAY(AS_INSTANCE(value)->this_)
          && !IS_NUMBER(AS_INSTANCE(value)->this_))) {
        args[-1] = NATIVE_ERROR("Value cannot be converted to an array");
        return false;
    }

    if (IS_NUMBER(value) || IS_INSTANCE(value) && IS_NUMBER(AS_INSTANCE(value)->this_)) {
        int len = (int) AS_NUMBER(IS_NUMBER(value)
                                   ? value
                                   : AS_INSTANCE(value)->this_);
        ObjArray *array_ = newArray();
        valueInitValueArray(&array_->array, NIL_VAL, len);
        instance->this_ = OBJ_VAL(array_);
        tableSet(&instance->fields, copyString("length", 6), NUMBER_VAL(len));
        return true;
    }

    if (IS_ARRAY(value)) {
        ObjArray *original = AS_ARRAY(value);
        ObjArray *array_ = newArray();
        copyValueArray(&original->array, &array_->array);
        instance->this_ = OBJ_VAL(array_);
        tableSet(&instance->fields, copyString("length", 6), NUMBER_VAL(array_->array.count));
        return true;
    }

    return false;
}
