#include <clox/native/system/system.h>

bool exitNative(int argCount, Value* implicit, Value* args) {
    if (argCount > 1) {
        *implicit = NATIVE_ERROR("invalid call to exit([Number exitCode])");
        return false;
    }

    if (argCount == 1 && !IS_NUMBER(args[0])) {
        *implicit = NATIVE_ERROR("Exit code must be a number");
        return false; 
    }

    int exitCode = 0;
    if (argCount == 1) {
        exitCode = AS_NUMBER(args[0]);
    }
    
    terminate(exitCode);
    __builtin_unreachable();
}
