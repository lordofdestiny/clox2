#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define assertm(expression, message) assert((expression) && (message))

#include <jansson.h>

#include "config.h"

static void freeNativeFunctionDescriptor(NativeFunctionDescriptor* functionDescriptor) {
    free(functionDescriptor->name);
    free(functionDescriptor->export);
    free(functionDescriptor->argTypes);
}

void freeNativeModuleDescriptor(NativeModuleDescriptor* moduleDescriptor) {
    free(moduleDescriptor->name);
    for(size_t i = 0 ; i < moduleDescriptor->functionCount; i++) {
        freeNativeFunctionDescriptor(&moduleDescriptor->functions[i]);
    }
    free(moduleDescriptor->functions);
}

const char* json_typenames[] = {
    [JSON_OBJECT]="object",
    [JSON_ARRAY]="array",
    [JSON_STRING]="string",
    [JSON_INTEGER]="integer",
    [JSON_REAL]="double",
    [JSON_TRUE]="true",
    [JSON_FALSE]="false",
    [JSON_NULL]="null",
};

const char* supported_arg_types[] = {
    [NATIVE_FUNCTION_TYPE_VALUE] = "Value",
    [NATIVE_FUNCTION_TYPE_NUMBER] = "Number",
    [NATIVE_FUNCTION_TYPE_BOOL] = "Bool",
    [NATIVE_FUNCTION_TYPE_NIL] = "nil",
    [NATIVE_FUNCTION_TYPE_OBJ] = "Object",
    [NATIVE_FUNCTION_TYPE_OBJ_ARRAY] = "Array",
    [NATIVE_FUNCTION_TYPE_OBJ_CLASS] = "Class",
    [NATIVE_FUNCTION_TYPE_OBJ_INSTANCE] = "Instance",
    [NATIVE_FUNCTION_TYPE_OBJ_STRING] = "String",
};

static size_t arg_types_size = sizeof(supported_arg_types) / sizeof(supported_arg_types[0]);

const char* nativeFunctionArgName(NativeFunctionArgType id) {
    assertm(id >= 0 || id < arg_types_size, "Invalid argument type id");
    return supported_arg_types[id];
}

static NativeFunctionArgType decodeArgType(const char* const type) {
    for(size_t i = 0; i < arg_types_size; i++) {
        if (supported_arg_types[i] == NULL) continue;
        if (strcmp(type, supported_arg_types[i]) == 0) {
            return i;
        }
    }
    return NATIVE_FUNCTION_TYPE_NONE;
}

static const char* get_json_typename(json_t * node) {
    static int typenames_size = sizeof(json_typenames) / sizeof(json_typenames[0]);
    int typeid = json_typeof(node);
    if (typeid < 0 || typeid >= typenames_size) {
        return "unknown";
    }
    return json_typenames[typeid];
}

#define ERROR_FORMAT_FAILED_OPEN \
    "NativeModuleError: %s\n"

#define ERROR_FORMAT_LOAD \
    "NativeModuleError [%s:%d:%d]: %s\n"

#define ERROR_FORMAT_LOCATION \
    "NativeModuleError in file \'%s\'. "

#define ERROR_FORMAT_EXPECTED \
    "Expected JSON %s, but found %s"

#define ERROR_FORMAT_MODULE \
    ERROR_FORMAT_LOCATION "%s.\n"

#define ERROR_FORMAT_MODULE_FIELD_MISSING \
    ERROR_FORMAT_LOCATION "Missing field \"%s\".\n"

#define ERROR_FORMAT_MODULE_FIELD_TYPE \
    ERROR_FORMAT_LOCATION \
    " Invalid type for field \"%s\"." \
    " " ERROR_FORMAT_EXPECTED ".\n"

#define ERROR_FORMAT_FUNCTION_FIELD \
    ERROR_FORMAT_LOCATION \
    "Function at index %zu: "

#define ERROR_FORMAT_FUNCTION_FIELD_MISSING \
    ERROR_FORMAT_FUNCTION_FIELD \
    "missing field \"%s\".\n"

#define ERROR_FORMAT_FUNCTION_FIELD_TYPE \
    ERROR_FORMAT_FUNCTION_FIELD \
     "invalid type for field \"%s\"."\
    " " ERROR_FORMAT_EXPECTED ".\n"

#define ERROR_BUFFER_SIZE 1024
static char* errorBuffer[ERROR_BUFFER_SIZE];

#define STORE_ERROR(FORMAT, ...) snprintf((char*) errorBuffer, sizeof(errorBuffer), FORMAT, __VA_ARGS__)

static int verifyFunctionsDescriptor(const char* filename, size_t index, json_t* root) {
    if(!json_is_object(root)) {
        return NATIVE_MODULE_LOAD_ERROR_FIELD_TYPE;
    }

    json_t *nameField, *exportField, *argsField, *returnsField;
    exportField = json_object_get(root, "export");
    if(exportField == NULL) {
        STORE_ERROR(ERROR_FORMAT_FUNCTION_FIELD_MISSING, filename, index, "export");
        return NATIVE_MODULE_LOAD_ERROR_MISSING_FIELD;
    }
    if(!json_is_string(exportField)) {
        STORE_ERROR(ERROR_FORMAT_FUNCTION_FIELD_TYPE, filename, index, "export", "string", get_json_typename(exportField));
        return NATIVE_MODULE_LOAD_ERROR_FIELD_TYPE;
    }

    nameField = json_object_get(root, "name");
    if(nameField != NULL && !json_is_string(nameField)) {
        STORE_ERROR(ERROR_FORMAT_FUNCTION_FIELD_TYPE, filename, index, "name", "string", get_json_typename(nameField));
        return NATIVE_MODULE_LOAD_ERROR_FIELD_TYPE;
    }

    returnsField = json_object_get(root, "returns");
    if(returnsField == NULL) {
        STORE_ERROR(ERROR_FORMAT_FUNCTION_FIELD_MISSING, filename, index, "returns");
        return NATIVE_MODULE_LOAD_ERROR_MISSING_FIELD;
    }
    if(!json_is_string(returnsField)) {
        STORE_ERROR(ERROR_FORMAT_FUNCTION_FIELD_TYPE, filename, index, "returns", "string", get_json_typename(returnsField));
        return NATIVE_MODULE_LOAD_ERROR_FIELD_TYPE;
    }
    const char* const typeName = json_string_value(returnsField);
    if (decodeArgType(typeName) == NATIVE_FUNCTION_TYPE_NONE) {
        STORE_ERROR(ERROR_FORMAT_FUNCTION_FIELD " Unknown return type \'%s\'", filename, index, typeName);
        return NATIVE_MODULE_LOAD_ERROR_FUNCTION_ARG_TYPE;
    }

    argsField = json_object_get(root, "args");
    if(argsField == NULL) {
        STORE_ERROR(ERROR_FORMAT_FUNCTION_FIELD_MISSING, filename, index, "args");
        return NATIVE_MODULE_LOAD_ERROR_MISSING_FIELD;
    }
    if(!json_is_array(argsField)) {
        STORE_ERROR(ERROR_FORMAT_FUNCTION_FIELD_TYPE, filename, index, "args", "array", get_json_typename(argsField));
        return NATIVE_MODULE_LOAD_ERROR_FIELD_TYPE;
    }

    size_t argIndex;
    json_t* argIndexValue;
    json_array_foreach(argsField, argIndex, argIndexValue) {
        if (!json_is_string(argIndexValue)) {
            STORE_ERROR(
                ERROR_FORMAT_FUNCTION_FIELD ERROR_FORMAT_EXPECTED ", for argument at index %zu.\n",
                filename, index,
                "string",
                get_json_typename(argIndexValue),
                argIndex);
            return NATIVE_MODULE_LOAD_ERROR_FIELD_TYPE;
        }
        const char* const typeName = json_string_value(argIndexValue);
        if (decodeArgType(typeName) == NATIVE_FUNCTION_TYPE_NONE) {
            STORE_ERROR(ERROR_FORMAT_FUNCTION_FIELD " Unknown argument type at index %zu -\'%s\'.\n", filename, index, argIndex, typeName);
            return NATIVE_MODULE_LOAD_ERROR_FUNCTION_ARG_TYPE;
        }
    }

    return NATIVE_MODULE_LOAD_SUCCESS;
}

static int loadNativeModuleDescriptorImpl(const char* filename, json_t* root, NativeModuleDescriptor* moduleDescriptor) {
    json_t* libraryField, *functionsField;

    libraryField = json_object_get(root, "library");
    if (libraryField == NULL) {
        STORE_ERROR(ERROR_FORMAT_MODULE_FIELD_MISSING, filename, "library");
        return NATIVE_MODULE_LOAD_ERROR_MISSING_FIELD;
    }
    if (!json_is_string(libraryField)) {
        STORE_ERROR(ERROR_FORMAT_MODULE_FIELD_TYPE, filename, "library", "string", get_json_typename(libraryField));
        return NATIVE_MODULE_LOAD_ERROR_FIELD_TYPE;
    }
    const char* libName = json_string_value(libraryField);

    functionsField = json_object_get(root, "functions");
    if (functionsField == NULL) {
        STORE_ERROR(ERROR_FORMAT_MODULE_FIELD_MISSING, filename, "functions");
        return NATIVE_MODULE_LOAD_ERROR_MISSING_FIELD;
    }
    if(!json_is_array(functionsField)) {
        STORE_ERROR(ERROR_FORMAT_MODULE_FIELD_TYPE, "filename", "functions", "array", get_json_typename(functionsField));
        return NATIVE_MODULE_LOAD_ERROR_FIELD_TYPE;
    }
    const size_t functionCount = json_array_size(functionsField);

    size_t index;
    json_t* functionObject;

    // Verify all objects have required fields
    json_array_foreach(functionsField, index, functionObject) {
        int status = verifyFunctionsDescriptor(filename, index, functionObject);
        if(status != NATIVE_MODULE_LOAD_SUCCESS) {
            return status;
        }
    }

    NativeFunctionDescriptor* functions = calloc(functionCount, sizeof(NativeFunctionDescriptor));
    if (functions == NULL) {
        return NATIVE_MODULE_LOAD_ERROR_MEMORY;
    }
    
    // Parse here
    bool failedAlloc = false;
    json_array_foreach(functionsField, index, functionObject) {
        json_t *exportField = json_object_get(functionObject, "export");
        json_t *nameField = json_object_get(functionObject, "name");
        json_t *returnsField = json_object_get(functionObject, "returns");
        json_t *argsField = json_object_get(functionObject, "args");

        const char* exportName = json_string_value(exportField);
        const char* functionName;
        if (nameField == NULL) {
            functionName = exportName;
        } else {
            functionName = json_string_value(nameField);
        }
        const char* returnType = json_string_value(returnsField);
        size_t argsSize = json_array_size(argsField);
        NativeFunctionArgType* argTypes = calloc(argsSize, sizeof(NativeFunctionArgType));
        if (argTypes == NULL) {
            failedAlloc = true;
            break;
        }

        size_t argIndex;
        json_t* argValue;
        json_array_foreach(argsField, argIndex, argValue) {
            const char* const typeName = json_string_value(argValue);
            argTypes[argIndex] = decodeArgType(typeName);
        }

        char* exportNameCopy = strdup(exportName);
        if (exportNameCopy == NULL) {
            failedAlloc = true;
            break;
        }

        char* functionNameCopy = strdup(functionName);
        if (functionNameCopy == NULL) {
            failedAlloc = true;
            break;
        }

        functions[index] = (NativeFunctionDescriptor) {
            .name = functionNameCopy,
            .export = exportNameCopy,
            .returnType = decodeArgType(returnType),
            .argTypesCount = argsSize,
            .argTypes = argTypes
        };
    }

    if (failedAlloc) {
        for(size_t i = 0; i < index; i++) {
            freeNativeFunctionDescriptor(&functions[i]);
        }
        return NATIVE_MODULE_LOAD_ERROR_MEMORY;
    }

    char* libNameCopy = strdup(libName);
    if (libNameCopy == NULL) {
        failedAlloc = true;
        return NATIVE_MODULE_LOAD_ERROR_MEMORY;
    }

    *moduleDescriptor = (NativeModuleDescriptor) {
        .name = libNameCopy,
        .functionCount = functionCount,
        .functions = functions
    };
   
    return NATIVE_MODULE_LOAD_SUCCESS;
}

int loadNativeModuleDescriptor(const char* filename, NativeModuleDescriptor* moduleDescriptor) {
    assertm(filename != NULL, "Expected non-NULL filename");
    assertm(moduleDescriptor != NULL, "Expected non-NULL NativeModuleDescriptor pointer");

    json_error_t error;
    json_t* root = json_load_file(filename, JSON_REJECT_DUPLICATES, &error);
    if (root == NULL) {
        if(json_error_code(&error) == json_error_cannot_open_file) {
            STORE_ERROR(ERROR_FORMAT_FAILED_OPEN, error.text);
            return NATIVE_MODULE_LOAD_ERROR_FAILED_OPEN;
        }

        STORE_ERROR(ERROR_FORMAT_LOAD, filename, error.line, error.column, (char*) error.text);
        return NATIVE_MODULE_LOAD_ERROR_INVALID_JSON_FORMAT;
    }

    if (!json_is_object(root)) {
        STORE_ERROR(ERROR_FORMAT_MODULE, filename, "Invalid module descriptor format. Expected JSON object.");
        json_decref(root);
        return NATIVE_MODULE_LOAD_ERROR_INVALID_STRUCTURE;
    }

    int status = loadNativeModuleDescriptorImpl(filename, root, moduleDescriptor);
    json_decref(root);

    return status;
}

int formatFunctionSignature(char* buffer, int cap, NativeFunctionDescriptor* function) {
    int bufferSize = 0;
    bufferSize += snprintf(buffer, cap, "fun %s(", function->export);
    if (bufferSize >= cap) {
        return bufferSize;
    }
    for(size_t i = 0; i < function->argTypesCount; i++) {
        const char* sep = i != function->argTypesCount - 1 ? ", " : "";
        bufferSize += snprintf(
            buffer + bufferSize, cap - bufferSize,
            "%s%s", nativeFunctionArgName(function->argTypes[i]),
            sep
        );
        if (bufferSize >= cap) {
            return bufferSize;;
        }
    }
    bufferSize += snprintf(buffer + bufferSize, cap - bufferSize, 
        ") -> %s",
        nativeFunctionArgName(function->returnType)
    );

    return bufferSize;
}

int printFunctionSignature(FILE* file, NativeFunctionDescriptor* function) {
    int bufferSize = 0;
    bufferSize += fprintf(file, "fun %s(", function->export);
    for(size_t i = 0; i < function->argTypesCount; i++) {
        bufferSize += fprintf(
            file,
            "%s%s", nativeFunctionArgName(function->argTypes[i]),
            i != function->argTypesCount - 1 ? ", " : ""
        );
    }
    bufferSize += fprintf(file, ") -> %s",
        nativeFunctionArgName(function->returnType)
    );

    return bufferSize;
}

char* getNativeModuleError() {
    return (char*) errorBuffer;
}
