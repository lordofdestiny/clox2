#include <math.h>

#include <clox/native/math/math.h>

double ludolphine_number(void) {
    return M_PI;
}

double eulers_number(void) {
    return M_E;
}

#include <stdio.h>

CLOXMATH_EXPORT void onLoad(void){
    printf("MATH MODULE LOADED\n");
}
