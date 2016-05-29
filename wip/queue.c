#include <stdio.h>
#include "queue.h"
#include "stdlib.h"

void CSQueueAdd(local_id pid, timestamp_t times) {
    CSQueueNode *newItem;
    newItem = malloc(sizeof(CSQueueNode));
    newItem->pid = pid;
    newItem->lstamp = times;
    newItem->next = NULL;

    if(queue.size == 0)
        queue.head = newItem;
    else {
        CSQueueNode *current = queue.head;
        CSQueueNode *prevItem;

        if(queue.size == 1) {
            if(current->lstamp > times) {
                newItem->next = current;
                queue.head = newItem;
            }

            if(current->lstamp < times) {
                current->next = newItem;
            }

            if(current->lstamp == times) {
                if(current->pid < pid) {
                    newItem->next = current->next;
                    current->next = newItem;
                } else {
                    newItem->next = current;
                    queue.head = newItem;
                }
            }
            ++queue.size;
            return;
        }

        while (current->next != NULL && current->lstamp <= times) {
            prevItem = current;
            current = current->next;
        }

        if(current->next != NULL) {
            if(current->lstamp == times) {
                if(current->pid < pid) {
                    newItem->next = current->next;
                    current->next = newItem;
                } else {
                    newItem->next = current;
                    prevItem->next = newItem;
                }
            } else if(current->lstamp < times) {
                newItem->next = current->next;
                current->next = newItem;
            } else {
                newItem->next = current;
                prevItem->next = newItem;
            }
        } else {
            if(current->lstamp == times) {
                if(current->pid < pid) {
                    current->next = newItem;
                } else {
                    newItem->next = current;
                    prevItem->next = newItem;
                }
            } else if(current->lstamp < times)
                current->next = newItem;
            else {
                newItem->next = current;
                prevItem->next = newItem;
            }
        }
    }

    ++queue.size;
}

int CSQueueDelete(local_id pid) {
    CSQueueNode *prevItem, *current;
    current = queue.head;

    if(current->pid == pid) {
        queue.head = current->next;
        free(current);
        --queue.size;
        return 0;
    }

    while (current->next != NULL && current->pid != pid) {
        prevItem = current;
        current = current->next;
    }

    if(current->next == NULL && current->pid != pid)
        return 1;

    if(queue.size == 1)
        queue.head = NULL;
    else
        prevItem->next = current->next;

    free(current);
    --queue.size;
    return 0;
}

CSQueueNode *CSQueuePeek() {
    return queue.head;
}

void CSQueuePrint(int lid) {
    CSQueueNode * current = queue.head;

    while (current != NULL) {
        printf("lid %d: (%d;%d)\n", lid, current->lstamp, current->pid);
        current = current->next;
    }
}
