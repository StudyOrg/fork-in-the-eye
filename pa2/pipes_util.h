#pragma once

#include "router.h"

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
