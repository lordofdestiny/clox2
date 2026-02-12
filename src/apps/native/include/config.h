#ifndef __CLOX_NATIVE_LIBRARY_CONFIG_H__
#define __CLOX_NATIVE_LIBRARY_CONFIG_H__

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define NATIVE_MODULE_LOAD_SUCCESS 0
#define NATIVE_MODULE_LOAD_ERROR_MEMORY 0x1
#define NATIVE_MODULE_LOAD_ERROR_FAILED_OPEN 0x10
#define NATIVE_MODULE_LOAD_ERROR_INVALID_JSON_FORMAT 0x100
#define NATIVE_MODULE_LOAD_ERROR_INVALID_STRUCTURE 0x1000
#define NATIVE_MODULE_LOAD_ERROR_MISSING_FIELD 0x1001
#define NATIVE_MODULE_LOAD_ERROR_FIELD_TYPE 0x1002
#define NATIVE_MODULE_LOAD_ERROR_FUNCTION_ARG_TYPE 0x10001

typedef enum {
    NATIVE_FUNCTION_TYPE_NONE,
    NATIVE_FUNCTION_TYPE_VALUE,
    NATIVE_FUNCTION_TYPE_NUMBER,
    NATIVE_FUNCTION_TYPE_BOOL,
    NATIVE_FUNCTION_TYPE_NIL,
    NATIVE_FUNCTION_TYPE_OBJ,
    NATIVE_FUNCTION_TYPE_OBJ_ARRAY,
    NATIVE_FUNCTION_TYPE_OBJ_CLASS,
    NATIVE_FUNCTION_TYPE_OBJ_INSTANCE,
    NATIVE_FUNCTION_TYPE_OBJ_STRING,
} NativeFunctionArgType;

typedef struct {
    char* name;
    char* export;
    NativeFunctionArgType returnType;
    size_t argTypesCount;
    NativeFunctionArgType* argTypes;
    bool wrapped;
} NativeFunctionDescriptor;

typedef struct {
    char* name;
    size_t functionCount;
    NativeFunctionDescriptor* functions;
} NativeModuleDescriptor;

int loadNativeModuleDescriptor(const char* filename, NativeModuleDescriptor* moduleDescriptor);
void freeNativeModuleDescriptor(NativeModuleDescriptor* moduleDescriptor);

const char* nativeFunctionArgName(NativeFunctionArgType id);
int formatFunctionSignature(char* buffer, int cap, NativeFunctionDescriptor* function);
int printFunctionSignature(FILE* file, NativeFunctionDescriptor* function);

char* getNativeModuleError();

#endif // __CLOX_NATIVE_LIBRARY_CONFIG_H__
