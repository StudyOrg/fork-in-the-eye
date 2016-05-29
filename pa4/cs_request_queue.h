#pragma once

#include "ipc.h"

typedef struct RequestInfoTag {
    local_id pid;
    timestamp_t lstamp;
    struct RequestInfoTag *n;
} CSRequestInfo;

typedef struct {
    int peek;
    CSRequestInfo *base;
} CSQueue;

void CSQueueAdd(local_id, timestamp_t);
int CSQueueDelete(local_id);
CSRequestInfo* CSQueuePeek();
void CSQueuePrint();
