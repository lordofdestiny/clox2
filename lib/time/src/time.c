#include <time.h>

#include <unistd.h>

#include <clox/native/time/time.h>

double getTime(void) {
    return (double) clock() / CLOCKS_PER_SEC;
}

bool sleepFor(double seconds) {
    return sleep(seconds) == 0;
}
