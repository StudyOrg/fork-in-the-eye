#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

#include "ipc.h"
#include "common.h"
#include "main.h"
#include "router.h"
#include "util.h"

int send(void * data, local_id dst, const Message * msg) {
    Router *rt = (Router*)data;
    uint16_t size = sizeof(msg->s_header) + msg->s_header.s_payload_len;

    int *fd = rt->routes[dst][rt->recent_pid].filedes;

    if(write(fd[OUT], msg, size) != size)
        return 1;

    return 0;
}

int send_multicast(void * data, const Message * msg) {
    Router *rt = (Router*)data;

    for(int i = 0; i < rt->procnum; i++) {
        if(i == rt->recent_pid)
            continue;
        if(send(data, i, msg) != 0)
            return 1;
    }
    return 0;
}

int receive(void * data, local_id from, Message * msg) {
    Router *rt = (Router*)data;
    uint16_t size = sizeof(msg->s_header);

    int *fd = rt->routes[rt->recent_pid][from].filedes;

    if(read(fd[IN], &msg->s_header, size) != size)
        return -1;

    size = msg->s_header.s_payload_len;

    if(read(fd[IN], &msg->s_payload, size) != size)
        return -1;

    return 0;
}

int receive_any(void * data, Message * msg) {
    Router *rt = (Router*)data;

    for(int i = 0; i < rt->procnum; i++) {
        if(i == rt->recent_pid)
            continue;
        if(receive(data, i, msg) == 0)
            return 0;
    }

    receive_sleep();

    return 1;
}
