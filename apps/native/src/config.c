#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <common/util.h>

#include <jansson.h>

#include "config.h"

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

#define LOG_ERROR(FORMAT, ...) fprintf(stderr, "> " FORMAT, __VA_ARGS__)

static void freeNativeFunction(NativeFunction* function) {
    free(function->name);
    free(function->export);
    free(function->argTypes);
}

void freeNativeModule(NativeModule* module) {
    free(module->name);
    free(module->namePrefix);
    for(size_t i = 0 ; i < module->functionCount; i++) {
        freeNativeFunction(&module->functions[i]);
    }
    free(module->functions);
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
    massert(id >= 0 || id < arg_types_size, "Invalid argument type id");
    return supported_arg_types[id];
}

static NativeFunctionArgType decodeArgType(const char* const type) {
    if (type == NULL) {
        return NATIVE_FUNCTION_TYPE_NONE;
    }
    
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

typedef struct {
    ParseResult pr;
    bool failed;
    bool fatal;
} ParseState;

void nonFatalError(ParseState* pe, int code) {
    pe->pr.count++;
    pe->pr.code = code;
    pe->failed = false;
    pe->fatal = false;
}

void fatalError(ParseState* pe, int code) {
    nonFatalError(pe, code);
    pe->fatal = true;
}

static void parseFunction(ParseState* pe, const char* filename, size_t index, json_t* root, NativeFunction* out_function) {
    if (root == NULL) {
        nonFatalError(pe, LOAD_ERROR_NULL_ROOT);
        return;
    }
    
    if(!json_is_object(root)) {
        LOG_ERROR(
            ERROR_FORMAT_FUNCTION_FIELD "Invalid type. " ERROR_FORMAT_EXPECTED,
            filename, index, "object", get_json_typename(root));
        nonFatalError(pe,LOAD_ERROR_INVALID_STRUCTURE);
        return;
    }
    
    json_t* exportField = json_object_get(root, "export");
    const char* exportName = json_string_value(exportField);
    if(exportField == NULL) {
        LOG_ERROR(
            ERROR_FORMAT_FUNCTION_FIELD_MISSING,
            filename, index, "export");
        nonFatalError(pe,LOAD_ERROR_MISSING_FIELD);
    } else if(!json_is_string(exportField)) {
        LOG_ERROR(
            ERROR_FORMAT_FUNCTION_FIELD_TYPE, 
            filename, index, "export", "string", get_json_typename(exportField));
        nonFatalError(pe,LOAD_ERROR_FIELD_TYPE);
    }
    
    json_t *nameField = json_object_get(root, "name");
    const char* functionName = json_string_value(nameField);
    if (functionName == NULL) {
        functionName = exportName;
    }
    if(nameField != NULL && !json_is_string(nameField)) {
        LOG_ERROR(
            ERROR_FORMAT_FUNCTION_FIELD_TYPE,
            filename, index, "name", "string", get_json_typename(nameField));
        nonFatalError(pe,LOAD_ERROR_FIELD_TYPE);
    }
    
    json_t* wrappedField = json_object_get(root, "wrap");
    if(wrappedField != NULL && !json_is_boolean(wrappedField)) {
        LOG_ERROR(
            ERROR_FORMAT_FUNCTION_FIELD_TYPE,
            filename, index, "wrap", "boolean", get_json_typename(wrappedField));
        nonFatalError(pe, LOAD_ERROR_FIELD_TYPE);
    }

    if (wrappedField != NULL && !json_boolean_value(wrappedField)) {
        if (pe->failed) return;
        char* functionNameDup = strdup(functionName);
        char* exportNameDup = strdup(exportName);
        
        if (functionNameDup == NULL || exportNameDup == NULL) {
            free(functionNameDup);
            free(exportNameDup);
            fatalError(pe, LOAD_ERROR_MEMORY);
            return;
        }

        *out_function = (NativeFunction) {
            .name = functionNameDup,
            .export = exportNameDup,
            .returnType = NATIVE_FUNCTION_TYPE_NONE,
            .argTypesCount = 0,
            .argTypes = NULL,
            .wrapped = false,
            .canFail = true
        };

        return; // Success
    }

    json_t *failsField = json_object_get(root, "fails");
    if(failsField != NULL && !json_is_boolean(failsField)) {
        LOG_ERROR(
            ERROR_FORMAT_FUNCTION_FIELD_TYPE,
            filename, index, "fails", "boolean", get_json_typename(failsField));
        nonFatalError(pe, LOAD_ERROR_FIELD_TYPE);
    }
    
    json_t* returnsField = json_object_get(root, "returns");
    const char* typeName = json_string_value(returnsField);
    if(returnsField == NULL) {
        LOG_ERROR(
            ERROR_FORMAT_FUNCTION_FIELD_MISSING,
            filename, index, "returns");
        nonFatalError(pe, LOAD_ERROR_MISSING_FIELD);
    }else if(!json_is_string(returnsField)) {
        LOG_ERROR(
            ERROR_FORMAT_FUNCTION_FIELD_TYPE,
            filename, index, "returns", "string", get_json_typename(returnsField));
        nonFatalError(pe, LOAD_ERROR_FIELD_TYPE);
    }else if (decodeArgType(typeName) == NATIVE_FUNCTION_TYPE_NONE) {
        LOG_ERROR(
            ERROR_FORMAT_FUNCTION_FIELD " Unknown return type \'%s\'\n",
            filename, index, typeName);
        nonFatalError(pe, LOAD_ERROR_FUNCTION_ARG_TYPE);
    }

    json_t *argsField;
    argsField = json_object_get(root, "args");
    if(argsField == NULL) {
        LOG_ERROR(ERROR_FORMAT_FUNCTION_FIELD_MISSING, filename, index, "args");
        nonFatalError(pe, LOAD_ERROR_MISSING_FIELD);
    } else if(!json_is_array(argsField)) {
        LOG_ERROR(ERROR_FORMAT_FUNCTION_FIELD_TYPE, filename, index, "args", "array", get_json_typename(argsField));
        nonFatalError(pe, LOAD_ERROR_FIELD_TYPE);
    }

    if(pe->failed) return;

    size_t argIndex;
    json_t* argValue;
    json_array_foreach(argsField, argIndex, argValue) {
        const char* const typeName = json_string_value(argValue);
        if (!json_is_string(argValue)) {
            LOG_ERROR(
                ERROR_FORMAT_FUNCTION_FIELD ERROR_FORMAT_EXPECTED ", for argument at index %zu.\n",
                filename, index, "string", get_json_typename(argValue), argIndex);
            nonFatalError(pe, LOAD_ERROR_FIELD_TYPE);
        } else if (decodeArgType(typeName) == NATIVE_FUNCTION_TYPE_NONE) {
            LOG_ERROR(
                ERROR_FORMAT_FUNCTION_FIELD " Unknown argument type at index %zu -\'%s\'.\n",
                filename, index, argIndex, typeName);
            nonFatalError(pe, LOAD_ERROR_FUNCTION_ARG_TYPE);
        }
    }

    char* functionNameDup = NULL;
    char* exportNameDup = NULL;
    NativeFunctionArgType* argTypes = NULL;;
    size_t argCount = 0;

    functionNameDup = strdup(functionName);
    exportNameDup = strdup(exportName);
        
    if (functionNameDup == NULL || exportNameDup == NULL) {
        goto out_of_memory;
    }
    
    argCount = json_array_size(argsField);
    argTypes = calloc(argCount, sizeof(NativeFunctionArgType));
    if (argTypes == NULL) {
        goto out_of_memory;
    }

    json_array_foreach(argsField, argIndex, argValue) {
        const char* const typeName = json_string_value(argValue);
        argTypes[argIndex] = decodeArgType(typeName);
    }

    *out_function = (NativeFunction) {
            .name = functionNameDup,
            .export = exportNameDup,
            .returnType = decodeArgType(typeName),
            .argTypesCount = argCount,
            .argTypes = argTypes,
            .wrapped = json_boolean_value(wrappedField) || true,
            .canFail = false || json_boolean_value(failsField)
    };

    return; // Success
    
out_of_memory:
    free(functionNameDup);
    free(exportNameDup);
    free(argTypes);
    fatalError(pe, LOAD_ERROR_MEMORY);
}

static char* generateNamePrefix(const char* libNameCopy) {
    size_t bufferSize = strlen(libNameCopy);
    char* buffer = calloc(bufferSize + 1, sizeof(char));
    
    if (buffer == NULL) {
        fprintf(stderr, "out of memory\n");
        return NULL;
    }
    
    strcpy(buffer, libNameCopy);
    for(size_t i = 0 ; i < bufferSize; i++) {
        buffer[i] = toupper(buffer[i]);
    }
    return buffer;
}

static void loadNativeModuleImpl(ParseState* pe, const char* filename, json_t* root, NativeModule* module) {
    if (root == 0) {
        fatalError(pe, LOAD_ERROR_NULL_ROOT);
        return;
    }

    json_t* interfaceVersionField = json_object_get(root, "interfaceVersion");
    int interfaceVersion = CURRENT_INTERFACE_VERSION;
    if (interfaceVersionField != NULL && !json_is_integer(interfaceVersionField)) {
        LOG_ERROR(
            ERROR_FORMAT_MODULE_FIELD_TYPE,
            filename, "library", "string", get_json_typename(interfaceVersionField));
        nonFatalError(pe, LOAD_ERROR_FIELD_TYPE);
    }else {
        interfaceVersion = json_integer_value(interfaceVersionField);
    }

    json_t* libraryField = json_object_get(root, "library");
    const char* libName = json_string_value(libraryField);
    if (libraryField == NULL) {
        LOG_ERROR(
            ERROR_FORMAT_MODULE_FIELD_MISSING,
            filename, "library");
        nonFatalError(pe, LOAD_ERROR_MISSING_FIELD);
    }else if (!json_is_string(libraryField)) {
        LOG_ERROR(ERROR_FORMAT_MODULE_FIELD_TYPE, filename, "library", "string", get_json_typename(libraryField));
        nonFatalError(pe, LOAD_ERROR_FIELD_TYPE);
    }

    json_t *functionsField = json_object_get(root, "functions");
    if (functionsField == NULL) {
        LOG_ERROR(
            ERROR_FORMAT_MODULE_FIELD_MISSING,
            filename, "functions");
        fatalError(pe, LOAD_ERROR_MISSING_FIELD);
        return;
    } else if(!json_is_array(functionsField)) {
        LOG_ERROR(
            ERROR_FORMAT_MODULE_FIELD_TYPE,
            filename, "functions", "array", get_json_typename(functionsField));
        fatalError(pe, LOAD_ERROR_MISSING_FIELD);
        return;
    }
    
    size_t index;
    json_t* functionObject;

    char* libNameDup = NULL;
    char* libNamePrefix = NULL;
    NativeFunction* functions = NULL;
    size_t functionCount = 0;

    libNameDup = strdup(libName);
    if (libNameDup == NULL) {
        fatalError(pe, LOAD_ERROR_MEMORY);
        goto free_all;
    }
    
    libNamePrefix = generateNamePrefix(libNameDup);
    if (libNamePrefix == NULL) {
        fatalError(pe, LOAD_ERROR_MEMORY);
        goto free_all;
    }

    functionCount = json_array_size(functionsField);
    functions = calloc(functionCount, sizeof(NativeFunction));
    if (functions == NULL) {
        fatalError(pe, LOAD_ERROR_MEMORY);
        return;
    }

    json_array_foreach(functionsField, index, functionObject) {
        parseFunction(pe, filename, index, functionObject, &functions[index]);
        if (pe->fatal) {
            goto free_all;
            return;
        }
    }

    if (pe->failed)  goto free_all;

    *module = (NativeModule) {
        .interfaceVersion = interfaceVersion,
        .name = libNameDup,
        .namePrefix = libNamePrefix,
        .functionCount = functionCount,
        .functions = functions
    };

    return; // Success
   
free_all:
    for(size_t i = 0; i < functionCount; i++) {
        freeNativeFunction(&functions[i]);
    }
    free(functions);
}

static void loadConfigRoot(ParseState* pe, const char* filename, json_t** out_root) {
    json_error_t error;
    json_t* root = json_load_file(filename, JSON_REJECT_DUPLICATES, &error);

    if (root == NULL) {
        if(json_error_code(&error) == json_error_cannot_open_file) {
            LOG_ERROR(ERROR_FORMAT_FAILED_OPEN, error.text);
            fatalError(pe, LOAD_ERROR_FAILED_OPEN);
            return;
        }

        LOG_ERROR(ERROR_FORMAT_LOAD, filename, error.line, error.column, (char*) error.text);
        fatalError(pe, LOAD_ERROR_INVALID_JSON_FORMAT);
    }

    if (!json_is_object(root)) {
        LOG_ERROR(ERROR_FORMAT_MODULE, filename, "Invalid module  format. Expected JSON object.");
        json_decref(root);
        fatalError(pe, LOAD_ERROR_INVALID_STRUCTURE);
    }

    *out_root = root;
}

ParseResult loadNativeModule(const char* filename, NativeModule* module) {
    massert(filename != NULL, "Expected non-NULL filename");
    massert(module != NULL, "Expected non-NULL NativeModule pointer");

    ParseState pe = {{0, 0}, 0, false};

    json_t* root;
    loadConfigRoot(&pe, filename, &root);

    if (pe.failed && pe.fatal) {
        return pe.pr;
    }

    loadNativeModuleImpl(&pe, filename, root, module);
    json_decref(root);

    return pe.pr;
}

int formatFunctionSignature(char* buffer, int cap, NativeFunction* function) {
    int bufferSize = 0;
    bufferSize += snprintf(buffer, cap, "fun %s(", function->export);
    if (bufferSize >= cap) {
        return bufferSize;
    }
    for(size_t i = 0; i < function->argTypesCount; i++) {
        const char* sep = i != function->argTypesCount - 1 ? ", " : "";
        bufferSize += snprintf(
            buffer + bufferSize, cap - bufferSize,
            "%s%s",
            nativeFunctionArgName(function->argTypes[i]), sep
        );
        if (bufferSize >= cap) {
            return bufferSize;
        }
    }
    bufferSize += snprintf(
        buffer + bufferSize, cap - bufferSize, 
        ") -> %s",
        nativeFunctionArgName(function->returnType)
    );

    return bufferSize;
}

int printFunctionSignature(FILE* file, NativeFunction* function) {
    int bufferSize = 0;
    bufferSize += fprintf(file, "fun %s(", function->export);
    for(size_t i = 0; i < function->argTypesCount; i++) {
        const char* sep = i != function->argTypesCount - 1 ? ", " : "";
        bufferSize += fprintf(
            file,
            "%s%s",
            nativeFunctionArgName(function->argTypes[i]), sep
        );
    }
    bufferSize += fprintf(
        file, ") -> %s",
        nativeFunctionArgName(function->returnType)
    );

    return bufferSize;
}
