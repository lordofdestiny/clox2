#include <stdio.h>
#include <stdio.h>
#include <string.h>

#include <jansson.h>

#include "config.h"
#include "generate.h"

#define INVALID_ARG_ERROR 97
#define FAILED_TO_READ 15;

int main(int argc, char* argv[]) {
    if (argc == 3  && strcmp(argv[1], "-p")==0) {
        const char* filename = argv[2];
        
        NativeModuleDescriptor desc;
        int rc = loadNativeModuleDescriptor(filename, &desc);
        if(rc != 0) {
            fprintf(stderr, "%s", getNativeModuleError());
            return FAILED_TO_READ;
        }
        
        printf("Module: %s\n", desc.name);
        printf("Function count: %zu\n", desc.functionCount);
        
        // for(size_t i = 0; i < desc.functionCount; i++) {
        //     printFunctionSignature(stdout, &desc.functions[i]);
        //     printf(";\n");
        // }

        freopen("header.h", "w", stdout);
        generateModuleWrapperHeader(stdout, &desc);
        freopen("source.c", "w", stdout);
        generateModuleWrapperSource(stdout,"header.h", &desc);

        freeNativeModuleDescriptor(&desc);

        return EXIT_SUCCESS;
    }

    fprintf(stderr, "Invalid args\n");

    return INVALID_ARG_ERROR;
}
