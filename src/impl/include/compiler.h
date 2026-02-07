#ifndef __CLOX2_COMPILER_H__
#define __CLOX2_COMPILER_H__

#include <stdbool.h>

#include <cloximpl_export.h>

#include "object.h"
#include "inputfile.h"

CLOXIMPL_EXPORT bool isRepl();
CLOXIMPL_EXPORT void setRepl(bool isRepl);

CLOXIMPL_EXPORT ObjFunction* compile(InputFile source);

#endif //__CLOX2_COMPILER_H__
