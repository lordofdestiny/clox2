#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#ifdef WITH_SAY_HELLO

#include "alpha.c"

#endif

int main() {
    void* lib = dlopen("./libtest.so", RTLD_NOW | RTLD_GLOBAL);
    if (lib == NULL) {
        fprintf(stderr, "Failed to load library: %s\n", dlerror());
        exit(1);
    }
    void (*call_method)(void) =  (void(*)(void)) dlsym(lib, "call_method");
    if(call_method == NULL) {
        fprintf(stderr, "Failed to load 'call_method': %s\n", dlerror());
        exit(1);
    }
    call_method();
    dlclose(lib);
    return 0;
}
