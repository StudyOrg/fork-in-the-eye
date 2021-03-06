#include "util.h"

#include "stdversion.h"

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
