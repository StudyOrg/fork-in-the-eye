#pragma once

#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define C_RED(str) ("\033[31;1m" str "\033[0m")

#define DIE(_message) (assertf( 1, 1, C_RED("Error"), (_message) ))
#define TEST(_condition, _message) (assertf( (_condition), 1, C_RED("Error"), (_message) ))
#define WARN(_condition, _message) (assertf( (_condition), 0, C_RED("Warning"), (_message) ))

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

typedef void (*logf)(const char* str, ...);

typedef struct {
    FILE* events_file;
    FILE* pipes_file;
    logf info;
    logf pipes_info;
    logf events_info;
} LogStruct;

static LogStruct log;

void info(const char* format, ...) {
    va_list args;
    va_start(args, format);

    vprintf(format, args);

    va_end(args);
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
    log.info = info;
    log.pipes_info = pipes_info;
    log.events_info = events_info;

    log.pipes_file = fopen(pipes_log, "w");
    TEST(log.pipes_file == NULL, "Can't open pipes log file");

    log.events_file = fopen(events_log, "w");
    TEST(log.events_file == NULL, "Can't open events log file");
}

void release_log() {
    fclose(log.pipes_file);
    fclose(log.events_file);
}
