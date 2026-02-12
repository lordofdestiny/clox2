#include <time.h>

#include <unistd.h>

#include <clox/native/time/time.h>

NumberResult getTime(void) {
    return (NumberResult) {
        .success = true,
        .value = (double) clock() / CLOCKS_PER_SEC
    };
}

BoolResult sleepFor(double seconds) {
    int ret = sleep(seconds);
    return (BoolResult) {
        .success = true,
        .value = ret == 0
    };
}
