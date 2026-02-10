#include <clox/time/timelib.h>

#include <time.h>

#include <unistd.h>

NumberResult getTime() {
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
