#ifndef __CLOX_NATIVE_LIBRARY_GENERATE_H__
#define __CLOX_NATIVE_LIBRARY_GENERATE_H__

#include <stdio.h>
#include "config.h"

void generateModuleWrapperHeader(
    FILE* file, 
    NativeModule* module,
    const char* exportHeader
);

void generateModuleWrapperSource(
    FILE* file,
    NativeModule* module,
    const char* includeHeader
);

#endif // __CLOX_NATIVE_LIBRARY_GENERATE_H__
