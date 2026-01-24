#include "generate.h"

static const char* returnTypeWrapperNames[] = {
    [NATIVE_FUNCTION_TYPE_NONE] = NULL,
    [NATIVE_FUNCTION_TYPE_VALUE] = "ValueResult",
    [NATIVE_FUNCTION_TYPE_NUMBER] = "NumberResult",
    [NATIVE_FUNCTION_TYPE_BOOL] = "BoolResult",
    [NATIVE_FUNCTION_TYPE_NIL] = "NilResult",
    [NATIVE_FUNCTION_TYPE_OBJ] = "ObjResult",
    [NATIVE_FUNCTION_TYPE_OBJ_ARRAY] = "ArrayResult",
    [NATIVE_FUNCTION_TYPE_OBJ_CLASS] = "ClassResult",
    [NATIVE_FUNCTION_TYPE_OBJ_INSTANCE] = "InstanceResult",
    [NATIVE_FUNCTION_TYPE_OBJ_STRING] = "StringResult",
};

static const char* isArgTypeNames[] = {
    [NATIVE_FUNCTION_TYPE_NONE] = NULL,
    [NATIVE_FUNCTION_TYPE_VALUE] = NULL,
    [NATIVE_FUNCTION_TYPE_NUMBER] = "IS_NUMBER",
    [NATIVE_FUNCTION_TYPE_BOOL] = "IS_BOOL",
    [NATIVE_FUNCTION_TYPE_NIL] = "IS_NIL",
    [NATIVE_FUNCTION_TYPE_OBJ] = "IS_OBJ",
    [NATIVE_FUNCTION_TYPE_OBJ_ARRAY] = "IS_ARRAY",
    [NATIVE_FUNCTION_TYPE_OBJ_CLASS] = "IS_CLASS",
    [NATIVE_FUNCTION_TYPE_OBJ_INSTANCE] = "IS_INSTANCE",
    [NATIVE_FUNCTION_TYPE_OBJ_STRING] = "IS_STRING",
};

static struct {
    const char* typeName;
    const char* conversion;
} argTypeCastNames[] = {
    [NATIVE_FUNCTION_TYPE_NONE] = { NULL, NULL },
    [NATIVE_FUNCTION_TYPE_VALUE] = { NULL, NULL },
    [NATIVE_FUNCTION_TYPE_NUMBER] = { "double", "AS_NUMBER" },
    [NATIVE_FUNCTION_TYPE_BOOL] = { "bool", "AS_BOOL" },
    [NATIVE_FUNCTION_TYPE_NIL] = { "void*", "AS_NIL" },
    [NATIVE_FUNCTION_TYPE_OBJ] = { "Obj*", "AS_OBJ" },
    [NATIVE_FUNCTION_TYPE_OBJ_ARRAY] = { "ObjArray*", "AS_ARRAY" },
    [NATIVE_FUNCTION_TYPE_OBJ_CLASS] = { "ObjClass*", "AS_CLASS" },
    [NATIVE_FUNCTION_TYPE_OBJ_INSTANCE] = { "ObjInstance*", "AS_INSTANCE" },
    [NATIVE_FUNCTION_TYPE_OBJ_STRING] = { "ObjString*", "AS_STRING" },
};

static void generateFunctionSignatures(FILE* file, NativeModuleDescriptor* moduleDescriptor) {
    for (size_t i = 0; i < moduleDescriptor->functionCount; i++) {
        NativeFunctionDescriptor* function = &moduleDescriptor->functions[i];

        fprintf(file, "%s %s(", returnTypeWrapperNames[function->returnType], function->name);
        for (size_t j = 0; j < function->argTypesCount; j++) {
            fprintf(file, "%s%s", returnTypeWrapperNames[function->argTypes[j]], 
                    j < function->argTypesCount - 1 ? ", " : "");
        }
        fprintf(file, ");\n");
    }
    fprintf(file, "\n");
}

void generateModuleWrapperHeader(FILE* file, NativeModuleDescriptor* moduleDescriptor) {
    fprintf(file, "// Auto-generated header for native module: %s\n", moduleDescriptor->name);
    fprintf(file, "#ifndef __CLOX_NATIVE_MODULE_%s_H__\n", moduleDescriptor->name);
    fprintf(file, "#define __CLOX_NATIVE_MODULE_%s_H__\n\n", moduleDescriptor->name);

    fprintf(file, "#include <stdbool.h>\n");
    fprintf(file, "\n");
    
    // TODO Emit native includes
    // Placeholder:
    fprintf(file, "#define NATIVE_ERROR(str) ((Value)str)\n\n");
    
    const int declCount = sizeof(returnTypeWrapperNames) / sizeof(returnTypeWrapperNames[0]);
    for (int i = 1; i < declCount; i++) {
        fprintf(file, "typedef struct %s %s;\n", returnTypeWrapperNames[i], returnTypeWrapperNames[i]);
    }
    fprintf(file, "\n");

    printf("#define IS_VALUE(arg) true\n");
    for (int i = 1; i < declCount; i++) {
        if (isArgTypeNames[i] != NULL) {
            fprintf(file, "#define %s(arg) true\n", isArgTypeNames[i]);
        }
    }
    fprintf(file, "\n");

    fprintf(file, "typedef struct Value Value;\n");
    fprintf(file, "typedef struct ObjInstance ObjInstance;\n");
    fprintf(file, "typedef struct ObjClass ObjClass;\n");
    fprintf(file, "typedef struct ObjString ObjString;\n");
    fprintf(file, "typedef struct ObjArray ObjArray;\n");
    fprintf(file, "\n");

    fprintf(file, "typedef bool (*NativeFn)(int argCount, struct Value* implicit, struct Value* args);\n\n");
    
    for (int i = 1; i < declCount; i++) {
        if (argTypeCastNames[i].typeName != NULL) {
            fprintf(file, "#define %s(arg) ((%s) (arg))\n", argTypeCastNames[i].conversion, argTypeCastNames[i].typeName);
        }
    }
    fprintf(file, "\n");

    generateFunctionSignatures(file, moduleDescriptor); 

    fprintf(file, "#endif // __CLOX_NATIVE_MODULE_%s_H__\n", moduleDescriptor->name);
}

static void generateFunctionArgCheck(FILE* file, NativeFunctionDescriptor* function, size_t argIndex) {
    NativeFunctionArgType argType = function->argTypes[argIndex];
    const char* isTypeName = isArgTypeNames[argType];
    
    if (!isTypeName) {
        fprintf(file, "    // Argument %zu is of type %s, no check needed.\n", argIndex, nativeFunctionArgName(argType));
        return;
    }

    fprintf(file, "    if (!%s(args[%zu])) {\n", isTypeName, argIndex);
    fprintf(file, "        *implicit = NATIVE_ERROR(\"Function '%s' expects first argument to be of type %s\");\n",
                 function->name, nativeFunctionArgName(argType));
    fprintf(file, "        return false; // Invalid argument type\n");
    fprintf(file, "    }\n");
}

static void generateFunctionWrapper(FILE* file, NativeFunctionDescriptor* function) {
    fprintf(file, "bool %sNativeWrapper(int argCount, Value* implicit, Value* args) {\n", function->name);
    for (size_t i = 0; i < function->argTypesCount; i++) {
        generateFunctionArgCheck(file, function, i);
    }
    fprintf(file, "    // Call the actual function here\n");
    // Cast arguments here
    for(size_t i = 0; i < function->argTypesCount; i++) {
        int typeId = function->argTypes[i];
        auto desc = argTypeCastNames[typeId];
        if(typeId == NATIVE_FUNCTION_TYPE_VALUE) {
            fprintf(file, "    Value arg%zu = args[%zu];\n", i, i);
        }
        else {
            fprintf(file, "    %s arg%zu = %s(args[%zu]);\n", desc.typeName, i, desc.conversion, i);
        }
    }
    fprintf(file, "\n");

    fprintf(file, "\t%s result = %s(", returnTypeWrapperNames[function->returnType], function->name);
    for (size_t i = 0; i < function->argTypesCount; i++) {
        fprintf(file, "arg%zu%s", i, i < function->argTypesCount - 1 ? ", " : "");
    }
    fprintf(file, ");\n");
    fprintf(file, 
        "    if (result.exception != NULL) {\n"
        "        *implicit = NATIVE_ERROR(result.exception);\n"
        "        return false;\n"
        "    }\n\n"
        "    *implicit = result.value;\n"
        "    return true;\n"
    );

    fprintf(file, "}\n\n");
}

static void generateRegistrationFunctions(FILE* file, NativeModuleDescriptor* moduleDescriptor) {
    fprintf(file,
    "size_t moduleFunctionCount() {\n"
    "    return %zu;\n"
    "}\n\n",
    moduleDescriptor->functionCount);

    fprintf(file,
    "size_t moduleClassCount() {\n"
    "    return 0;\n"
    "}\n\n");

    fprintf(file,
    "void registerFunction(size_t i, size_t* arity, char** name, NativeFn* fn) {\n"
    "    auto fnd = &functionMap[i];\n"
    "    *arity = fnd->arity;\n"
    "    *name = fnd->name;\n"
    "    *fn = fnd->fn;\n"
    "}\n\n");
}

static void generateFunctionMap(FILE* file, NativeModuleDescriptor* moduleDescriptor) {
    fprintf(file, "static struct {const char* name; int arity; NativeFn fn; } functionMap[] = {\n");
    for(size_t i = 0; i < moduleDescriptor->functionCount; i++) {
        NativeFunctionDescriptor* fn = &moduleDescriptor->functions[i];
        fprintf(file, "    {\"%s\", %zu, %sNativeWrapper},\n",  fn->name, fn->argTypesCount, fn->name);
    }
    fprintf(file,"};\n\n");
}

void generateModuleWrapperSource(FILE* file, const char* header, NativeModuleDescriptor* moduleDescriptor) {
    fprintf(file, "// Auto-generated source for native module: %s\n", moduleDescriptor->name);
    fprintf(file, "#include <stddef.h>\n\n");
    fprintf(file, "#include \"%s\"\n\n", header);

    for(size_t i = 0; i < moduleDescriptor->functionCount; i++) {
        generateFunctionWrapper(file, &moduleDescriptor->functions[i]);
    }

    fprintf(file, "void %sDefaultModuleOnLoad(void) {}\n", moduleDescriptor->name);
    fprintf(file, "void onLoad(void) __attribute__((weak, alias(\"%sDefaultModuleOnLoad\")));\n", moduleDescriptor->name);

    fprintf(file, "void %sDefaultModuleOnUnload(void) {}\n", moduleDescriptor->name);
    fprintf(file, "void onUnload(void) __attribute__((weak, alias(\"%sDefaultModuleOnUnoad\")));\n", moduleDescriptor->name);
    fprintf(file, "\n");

    generateFunctionMap(file, moduleDescriptor);

    generateRegistrationFunctions(file, moduleDescriptor);
}
