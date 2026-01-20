#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dlfcn.h"

typedef struct {
    void* lib;
    char*(*function)();
} LibHandle;

LibHandle handles[5];

void init() {
    char libPattern[] = "./liba.so";
    for (int i = 0; i < 5; i++) {
        libPattern[5] = 'a' + i;
        void* handle= dlopen(libPattern, RTLD_NOW);
        if (handle == NULL) {
            printf("Failed to open lib: %s. %s. Exiting ...\n", libPattern, dlerror());
            exit(0);
        }
        handles[i].lib = handle;
        handles[i].function = (char*(*)()) dlsym(handle, "message");
    }
}

void deinit() {
    for(int i= 0; i < 5; i++) {
        dlclose(handles[i].lib);
    }
    memset(handles, 0, sizeof(handles));
}

void exec() {
    for(int i = 0; i < 10 ; i++) {
       printf("Message: %s\n",  handles[i%5].function());
    }
}

int main() {
    init();
    exec();
    deinit();
}
