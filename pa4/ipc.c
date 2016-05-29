#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "ipc.h"
#include "common.h"
#include "pa1.h"
#include "router.h"

int send(void * self, local_id dst, const Message * msg) {
    Router* data = self;
    uint16_t size = sizeof(msg->s_header) + msg->s_header.s_payload_len;

    if(write(data->routes[dst][data->recent_pid][OUT], msg, size) != size)
        return 1;
    return 0;
}

int send_multicast(void * self, const Message * msg) {
    Router* data = self;

    for(int i = 0; i < data->procnum; i++) {
        if(i == data->recent_pid)
            continue;
        if(send(self, i, msg) != 0)
            return 1;
    }
    return 0;
}

int receive(void * self, local_id from, Message * msg) {
    Router* data = self;
    uint16_t size = sizeof(msg->s_header);

    if(read(data->routes[data->recent_pid][from][IN], &msg->s_header, size) != size)
        return -1;

    size = msg->s_header.s_payload_len;

    if(read(data->routes[data->recent_pid][from][IN], &msg->s_payload, size) != size)
        return -1;

    return 0;
}

int receive_any(void * self, Message * msg) {
    Router* data = self;

    for(int i = 0; i < data->procnum; i++) {
        if(i == data->recent_pid)
            continue;
        if(receive(self, i, msg) == 0)
            return i;
    }

    receive_sleep();

    return -2;
}
