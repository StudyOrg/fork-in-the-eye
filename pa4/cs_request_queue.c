#include "cs_request_queue.h"

#include <stdlib.h>
#include <stdio.h>

static CSQueue _cs_request_queue = {0, 0};

void CSQueueAdd(local_id pid, timestamp_t times) {
    CSRequestInfo* request_info;
    request_info = malloc(sizeof(CSRequestInfo));

    request_info->pid = pid;
    request_info->lstamp = times;
    request_info->n = NULL;

    if(!_cs_request_queue.peek) {
        // New queue
        _cs_request_queue.base = request_info;
    } else {
        // Add item
        CSRequestInfo *current = _cs_request_queue.base;
        CSRequestInfo *prevItem;

        if(_cs_request_queue.peek == 1) {
            if(current->lstamp > times) {
                request_info->n = current;
                _cs_request_queue.base = request_info;
            }

            if(current->lstamp < times) {
                current->n = request_info;
            }

            if(current->lstamp == times) {
                if(current->pid < pid) {
                    request_info->n = current->n;
                    current->n = request_info;
                } else {
                    request_info->n = current;
                    _cs_request_queue.base = request_info;
                }
            }

            ++_cs_request_queue.peek;
            return;
        }

        while (current->n != NULL && current->lstamp <= times) {
            prevItem = current;
            current = current->n;
        }

        if(current->n != NULL) {
            if(current->lstamp == times) {
                if(current->pid < pid) {
                    request_info->n = current->n;
                    current->n = request_info;
                } else {
                    request_info->n = current;
                    prevItem->n = request_info;
                }
            } else if(current->lstamp < times) {
                request_info->n = current->n;
                current->n = request_info;
            } else {
                request_info->n = current;
                prevItem->n = request_info;
            }
        } else {
            if(current->lstamp == times) {
                if(current->pid < pid) {
                    current->n = request_info;
                } else {
                    request_info->n = current;
                    prevItem->n = request_info;
                }
            } else if(current->lstamp < times)
                current->n = request_info;
            else {
                request_info->n = current;
                prevItem->n = request_info;
            }
        }
    }

    ++_cs_request_queue.peek;
}

int CSQueueDelete(local_id pid) {
    CSRequestInfo *prevItem, *current;
    current = _cs_request_queue.base;

    if(current->pid == pid) {
        _cs_request_queue.base = current->n;
        free(current);
        _cs_request_queue.peek --;
        return 0;
    }

    while (current->n != NULL && current->pid != pid) {
        prevItem = current;
        current = current->n;
    }

    if(current->n == NULL && current->pid != pid)
        return 1;

    if(_cs_request_queue.peek == 1)
        _cs_request_queue.base = NULL;
    else
        prevItem->n = current->n;

    free(current);
    _cs_request_queue.peek --;
    return 0;
}

CSRequestInfo* CSQueuePeek() {
    return _cs_request_queue.base;
}

void CSQueuePrint(int lid) {
    CSRequestInfo * current = _cs_request_queue.base;

    while (current != NULL) {
        printf("lid %d: (%d;%d)\n", lid, current->lstamp, current->pid);
        current = current->n;
    }
}
