#include "stdversion.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <wait.h>
#include <errno.h>


#include "ipc.h"
#include "common.h"
#include "banking.h"
#include "pa2345.h"
#include "router.h"
#include "queue.h"
#include "util.h"
#include "log.h"
#include "lamport_time.h"
#include "nice_service.h"
#include "cs_util.h"

int main(int argc, char *argv[]) {
    init_log();

    Router data;

    TEST(argc < 3, "Usage: pa4 -p N [--mutexl]");

    if (argc == 4) {
        _cs_use_lock = 1;
        ++argv;
    }

    data.procnum = atoi(argv[2]);
    TEST(data.procnum < 2, "Number of process must be >= 2");

    INC(data.procnum);

    data.recent_pid = ZERRO;

    for(int i = 0; i < data.procnum; i++) {
        for(int j = 0; j < data.procnum; j++) {
            if(j==i) {
                data.routes[i][j][IN] = -1;
                data.routes[i][j][OUT] = -1;
            } else {
                TEST(pipe(data.routes[i][j]) < 0, "pipe: error while creating\n");

                set_nonlock(data.routes[i][j][IN]);
                set_nonlock(data.routes[i][j][OUT]);

                pipes_info("pipe: created for %d <-> %d\n", j, i);
            }
        }
    }

    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = ZERRO;

    for(int i = 1; i < data.procnum; i++) {
        pid_t pid = fork();

        TEST(pid < 0, "Error creating the child\n");

        if (pid == 0) {
            nice_service(&data, i);
            exit(0);
        }
    }

    close_unused_pipes(&data);

    timestamp_t tm = get_lamport_time();

    int start_messages_counter = data.procnum - 1;
    int done_messages_counter = data.procnum - 1;

    Message answer;

    while(start_messages_counter) {
        if(receive_any(&data, &answer) > -1) {
            if(answer.s_header.s_type == STARTED) {
                DEC(start_messages_counter);

                lamport_update(answer.s_header.s_local_time);
                tm = get_lamport_time();
            }
            if(answer.s_header.s_type == DONE) {
                DEC(done_messages_counter);

                lamport_update(answer.s_header.s_local_time);
                tm = get_lamport_time();
            }
        }
    }

    events_info(log_received_all_started_fmt, tm, data.recent_pid);

    while(done_messages_counter) {
        if(receive_any(&data, &answer) > -1) {
            if(answer.s_header.s_type == DONE) {
                DEC(done_messages_counter);

                lamport_update(answer.s_header.s_local_time);
                tm = get_lamport_time();
            }
        }
    }

    events_info(log_received_all_done_fmt, tm, data.recent_pid);

    forever {
        int status;
        pid_t done = wait(&status);
        if (done == -1) {
            if (errno == ECHILD) {
                break;
            }
        } else {
            TEST(!WIFEXITED(status) || WEXITSTATUS(status) != 0, "Can't wait child");
        }
    }

    release_log();

    return 0;
}
