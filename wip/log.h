#pragma once

#include <stdio.h>

#define C_RED(str) ("\033[31;1m" str "\033[0m")

#define DIE(_message) (assertf( 1, 1, C_RED("Error"), (_message) ))
#define TEST(_condition, _message) (assertf( (_condition), 1, C_RED("Error"), (_message) ))
#define WARN(_condition, _message) (assertf( (_condition), 0, C_RED("Warning"), (_message) ))

int assertf( int condition, int critical, const char* head_msg, const char* str);

typedef void (*logf)(const char* str, ...);

typedef struct {
    FILE* events_file;
    FILE* pipes_file;
} LogStruct;

static LogStruct log;

void pipes_info(const char* format, ...);

void events_info(const char* format, ...);

void init_log();

void release_log();
