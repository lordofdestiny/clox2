#ifndef __CLOX_NATIVE_LIBRARY_GENERATE_H__
#define __CLOX_NATIVE_LIBRARY_GENERATE_H__

#include <stdio.h>
#include "config.h"

void generateModuleWrapperHeader(FILE* file, NativeModuleDescriptor* moduleDescriptor);
void generateModuleWrapperSource(FILE* file, const char* header, NativeModuleDescriptor* moduleDescriptor);

#endif // __CLOX_NATIVE_LIBRARY_GENERATE_H__
