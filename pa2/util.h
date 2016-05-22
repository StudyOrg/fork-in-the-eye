#pragma once

#include <time.h>

int receive_sleep() {
    struct timespec tmr;
    tmr.tv_sec = 0;
    tmr.tv_nsec = 50000000;

    if(nanosleep(&tmr, NULL) < 0 ) {
        return -1;
    }

    return 0;
}
