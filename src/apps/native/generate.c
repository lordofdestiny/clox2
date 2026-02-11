// TODO Output #line directives
// TODO Generate code for unpacking primitives
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
    [NATIVE_FUNCTION_TYPE_VALUE] = { "Value", NULL },
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
    fprintf(file, ""
        "#ifndef CLOX_EXPORT\n"
        "\n"
        "#error \"Undefined CLOX_EXPORT macro\"\n"
        "\n"
        "#endif\n\n\n"
    );

    for (size_t i = 0; i < moduleDescriptor->functionCount; i++) {
        NativeFunctionDescriptor* function = &moduleDescriptor->functions[i];

        if (function->wrapped) {
            fprintf(file, "%s %s(", returnTypeWrapperNames[function->returnType], function->export);
            for (size_t j = 0; j < function->argTypesCount; j++) {
                const char* sep = j < function->argTypesCount - 1 ? ", " : "";
                fprintf(file, "%s%s", argTypeCastNames[function->argTypes[j]].typeName, sep);
            }
            fprintf(file, ");\n");
        } else {
            fprintf(file,""
                "bool %s(int argCount, Value* implicit, Value* args);\n",
                function->export
            );
        }
        
    }
    fprintf(file, "\n");
}

void generateModuleWrapperHeader(FILE* file, NativeModuleDescriptor* moduleDescriptor) {
    fprintf(file, "// Auto-generated header for native module: %s\n", moduleDescriptor->name);
    fprintf(file, "#ifndef __CLOX_NATIVE_MODULE_%s_H__\n", moduleDescriptor->name);
    fprintf(file, "#define __CLOX_NATIVE_MODULE_%s_H__\n\n", moduleDescriptor->name);

    fprintf(file, "#include <stdbool.h>\n");
    fprintf(file, "\n");
    
    fprintf(file, "#include <clox/clox.h>\n\n");
    
    fprintf(file, "extern const char CLOX_MODULE_NAME[];\n\n");

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
    if (!function->wrapped) return;
    fprintf(file, "CLOX_NO_EXPORT bool %sNativeWrapper(int argCount, Value* implicit, Value* args) {\n", function->name);
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

    fprintf(file, "\t%s result = %s(", returnTypeWrapperNames[function->returnType], function->export);
    for (size_t i = 0; i < function->argTypesCount; i++) {
        fprintf(file, "arg%zu%s", i, i < function->argTypesCount - 1 ? ", " : "");
    }
    fprintf(file, ");\n");
    fprintf(file, ""
        "    if (!result.success) {\n"
        "        *implicit = result.exception;\n"
        "        return false;\n"
        "    }"
        "    \n"
        "    \n"
        "    *implicit = "
    );

    if (function->returnType == NATIVE_FUNCTION_TYPE_VALUE) {
        fprintf(file, ""
            "result.value;\n"
        );
    } else if (function->returnType == NATIVE_FUNCTION_TYPE_BOOL){
        fprintf(file, ""
            "BOOL_VAL(result.value);\n"
        );
    } else if (function->returnType == NATIVE_FUNCTION_TYPE_NUMBER){
        fprintf(file, ""
            "NUMBER_VAL(result.value);\n"
        );
    } else if (function->returnType == NATIVE_FUNCTION_TYPE_NIL){
        fprintf(file, ""
            "NIL_VAL;\n"
        );
    } else {
        fprintf(file, ""
            "OBJ_VAL(result.value);\n"
        );
    }

    fprintf(file, ""
        "    return true;\n"
    );

    fprintf(file, "}\n\n");
}

static void generateRegistrationFunctions(FILE* file, NativeModuleDescriptor* moduleDescriptor) {
    fprintf(file,""
    "CLOX_EXPORT size_t moduleClassCount() {\n"
    "    return 0;\n"
    "}\n\n");

    fprintf(file,""
    "CLOX_EXPORT size_t registerFunctions(DefineNativeFunctionFn registerFn) {\n"
    "    for (size_t i = 0; i < %zu; i++) {\n"
    "       auto fnd = &functionMap[i];\n"
    "       registerFn(fnd->name, fnd->arity, fnd->fn);\n"
    "    }\n"
    "    return %zu;\n"
    "}\n\n", 
    moduleDescriptor->functionCount,
    moduleDescriptor->functionCount);
}

static void generateFunctionMap(FILE* file, NativeModuleDescriptor* moduleDescriptor) {
    fprintf(file, ""
        "static struct {char* name; int arity; NativeFn fn; } functionMap[] = {\n"
    );
    for(size_t i = 0; i < moduleDescriptor->functionCount; i++) {
        NativeFunctionDescriptor* fn = &moduleDescriptor->functions[i];
        if (fn->wrapped) {
            fprintf(file, ""
            "    {\"%s\", %zu, %sNativeWrapper},\n",  fn->name, fn->argTypesCount, fn->name
            );
        } else {
            fprintf(file, ""
            "    {\"%s\", -1, %s},\n", fn->name, fn->export
            );
        }
    }
    fprintf(file, ""
        "};\n\n"
    );
}

void generateModuleWrapperSource(FILE* file, const char* header, NativeModuleDescriptor* moduleDescriptor) {
    fprintf(file, "// Auto-generated source for native module: %s\n", moduleDescriptor->name);
    fprintf(file, "#include <stddef.h>\n\n");
    fprintf(file, "#include \"%s\"\n\n", header);

    fprintf(file, "const char CLOX_MODULE_NAME[] = \"%s\";\n\n", moduleDescriptor->name);

    for(size_t i = 0; i < moduleDescriptor->functionCount; i++) {
        generateFunctionWrapper(file, &moduleDescriptor->functions[i]);
    }

    fprintf(file, "CLOX_NO_EXPORT void %sDefaultModuleOnLoad(void) { }\n", moduleDescriptor->name);
    fprintf(file, "CLOX_EXPORT void onLoad(void) __attribute__((weak, alias(\"%sDefaultModuleOnLoad\")));\n", moduleDescriptor->name);

    fprintf(file, "CLOX_NO_EXPORT void %sDefaultModuleOnUnload(void) { }\n", moduleDescriptor->name);
    fprintf(file, "CLOX_EXPORT void onUnload(void) __attribute__((weak, alias(\"%sDefaultModuleOnUnload\")));\n", moduleDescriptor->name);
    fprintf(file, "\n");

    generateFunctionMap(file, moduleDescriptor);

    generateRegistrationFunctions(file, moduleDescriptor);
}
