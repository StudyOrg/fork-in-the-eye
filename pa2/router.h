#pragma once

#include <sys/types.h>

#include "ipc.h"

static const int8_t IN = 0;
static const int8_t OUT = 1;

typedef int pipe_t[2];

typedef struct  {
    int procnum;
    pid_t recent_pid;
    pipe_t routes[MAX_PROCESS_ID][MAX_PROCESS_ID];
} Router;
