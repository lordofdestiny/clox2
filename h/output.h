//
// Created by djumi on 11/19/2023.
//

#ifndef CLOX2_OUTPUT_H
#define CLOX2_OUTPUT_H

#include "object.h"

typedef enum {
    SEG_FUNCTION = 0xBEEF,
    SEG_FUNCTION_HEADER,
    SEG_FUNCTION_NAME,
    SEG_FUNCTION_CODE,
    SEG_FUNCTION_CONSTANTS,
    SEG_FUNCTION_SCRIPT,
    SEG_FUNCTION_END,
    SEG_FILE_END = 0x7CADBEEF
} SegmentSequence;

typedef enum {
    OUT_TAG_NUMBER,
    OUT_TAG_STRING,
    OUT_TAG_FUNCTION
} ValueTag;

void outputToBinary(ObjFunction *compiled, const char *path);

#endif //CLOX2_OUTPUT_H
