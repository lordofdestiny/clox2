#ifndef __CLOX_LIB_NATIVE_H__
#define __CLOX_LIB_NATIVE_H__

#define NATIVE_ERROR(msg) (OBJ_VAL((Obj*) copyString(msg, strlen(msg))))

#endif //  __CLOX_LIB_NATIVE_H__
