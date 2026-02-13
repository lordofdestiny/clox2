#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>

#include <jansson.h>

#include "config.h"
#include "generate.h"

#define INVALID_ARG_ERROR 97
#define INVALID_FILE_PATH 66
#define FAILED_TO_READ 15

int main(int argc, char* argv[]) {
    if (argc == 7  && strcmp(argv[1], "-p")==0) {
        const char* filename = argv[2];
        const char* header = argv[3];
        const char* source = argv[4];
        const char* includeHeader = argv[5];
        const char* exportHeader = argv[6];
   
#if 0
printf("Command: cloxn -p %s %s %s\n", filename, header, source);
#endif


        NativeModuleDescriptor desc;
        int rc = loadNativeModuleDescriptor(filename, &desc);
#if 0
        if(rc != 0) {
            fprintf(stderr, "%s", getNativeModuleError());
            return FAILED_TO_READ;
        }
        
        printf("Module: %s\n", desc.name);
        printf("Function count: %zu\n", desc.functionCount);
#else
        (void)rc;
#endif

        void* res; 
        
        res =  freopen(header, "w", stdout);
        if (res == NULL) {
            fprintf(stderr,"Failed to open header output file: %s. %s\n", header, strerror(errno));
            exit(INVALID_FILE_PATH);
        }
        generateModuleWrapperHeader(stdout, &desc, exportHeader);

        res = freopen(source, "w", stdout);
        if (res == NULL) {
            fprintf(stderr,"Failed to open source output file: %s. %s\n", source, strerror(errno));
            exit(INVALID_FILE_PATH);
        }
        generateModuleWrapperSource(stdout, &desc,includeHeader);

        freeNativeModuleDescriptor(&desc);

        return EXIT_SUCCESS;
    }

    fprintf(stderr, "Invalid args: argc=%d\n", argc);
    for(int i = 0; i < argc; i++) {
        fprintf(stderr, "argv[%d] = \"%s\"\n", i, argv[i]);
    }

    return INVALID_ARG_ERROR;
}
