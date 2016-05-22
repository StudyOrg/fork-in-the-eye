#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int assertf( int condition, int critical, const char* head_msg, const char* str) {
    if (condition) {
        fprintf(stderr, "%s: ", head_msg);
        fprintf(stderr, "%s\n", str);

        if (critical) {
            exit(1);
        }
    }

    return condition;
}

void pipes_info(const char* format, ...) {
    va_list args;
    va_start(args, format);

    vfprintf(log.pipes_file, format, args);

    va_end(args);
    fflush(log.pipes_file);
}

void events_info(const char* format, ...) {
    va_list args;
    va_start(args, format);

    vfprintf(log.events_file, format, args);

    va_end(args);
    fflush(log.events_file);
}

void init_log() {
    log.pipes_file = fopen(pipes_log, "w");
    TEST(log.pipes_file == NULL, "Can't open pipes log file");

    log.events_file = fopen(events_log, "w");
    TEST(log.events_file == NULL, "Can't open events log file");
}

void release_log() {
    fclose(log.pipes_file);
    fclose(log.events_file);
}
