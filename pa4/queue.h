#pragma once

#include "ipc.h"

typedef struct item {
    /* ptr */
    struct item *next;
    /* data */
    local_id pid;
    timestamp_t lstamp;
} CSQueueNode;

typedef struct {
    int size;
    CSQueueNode *head;
} CSQueue;

/* Global queue object */
static CSQueue queue = {0, 0};

/* Operations */
void CSQueueAdd(local_id, timestamp_t);
int CSQueueDelete(local_id);
CSQueueNode* CSQueuePeek();
/* Utils */
void CSQueuePrint();
