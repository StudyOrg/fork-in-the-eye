#include "util.h"

#include "stdversion.h"

#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "router.h"

void receive_sleep() {
    struct timespec nanodelay = {0, 25000000};
    nanosleep(&nanodelay, NULL);
}

void set_nonlock(int fileno) {
    int mode = fcntl(fileno, F_GETFL);
    fcntl(fileno, F_SETFL, mode | O_NONBLOCK);
}

int close_unused_pipes(void * data) {
    Router *rt = (Router*)data;

    for(int i = 0; i < rt->procnum; i++) {
        for(int j = 0; j < rt->procnum; j++) {
            if(i == j) {
                continue;
            }

            if(i == rt->recent_pid) {
                close(rt->routes[rt->recent_pid][j][OUT]);
                continue;
            }

            if(i != rt->recent_pid && j != rt->recent_pid) {
                close(rt->routes[i][j][OUT]);
                close(rt->routes[i][j][IN]);
                continue;
            }

            close(rt->routes[i][j][IN]);
        }
    }

    return 0;
}

Message get_stop_message() {
    Message m;
    m.s_header.s_type = STOP;
    m.s_header.s_payload_len = 0;

    return m;
}

Message get_history_message(BalanceHistory h) {
    Message m;
    m.s_header.s_magic = MESSAGE_MAGIC;
    m.s_header.s_type = BALANCE_HISTORY;
    m.s_header.s_local_time = get_lamport_time();
    m.s_header.s_payload_len = sizeof(BalanceHistory);
    memcpy(m.s_payload, &h, sizeof(BalanceHistory));

    return m;
}

Message get_ack_message() {
    Message m;
    m.s_header.s_type = ACK;
    m.s_header.s_magic = MESSAGE_MAGIC;
    m.s_header.s_local_time = get_lamport_time();
    m.s_header.s_payload_len = 0;

    return m;
}
