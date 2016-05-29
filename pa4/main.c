#include "stdversion.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <errno.h>
#include <unistd.h>

#include "log.h"
#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "banking.h"
#include "util.h"
#include "router.h"
#include "service.h"
#include "lamport_time.h"
#include "cs_util.h"

int main(int argc, char *argv[]) {
    init_log();

    Router data;

    TEST(argc < 3, "Usage: pa4 -p N [--mutexl]");

    data.procnum = atoi(argv[2]);
    TEST(data.procnum < 2, "Number of process must be >= 2");

    if (argc == 4) {
        _cs_use_lock = 1;
    }

    ++data.procnum;

    data.recent_pid = 0;

    for(int i = 0; i < data.procnum; i++) {
        for(int j = 0; j < data.procnum; j++) {
            if(j != i) {
                int pipe_status = pipe(data.routes[i][j]);
                TEST(pipe_status < 0, "Can't create pipe");

                set_nonlock(data.routes[i][j][IN]);
                set_nonlock(data.routes[i][j][OUT]);

                pipes_info("pipe: opened for %d <-> %d\n", j, i);
            } else {
                data.routes[i][j][IN] = -1;
                data.routes[i][j][OUT] = -1;
            }
        }
    }

    Message msg, awaited_message;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = 0;

    for(int i = 1; i < data.procnum; i++) {
        pid_t pid = fork();

        TEST(pid < 0, "Error creating the child\n");

        if (pid == 0) {
            service_workers(&data, i);
            exit(0);
        }
    }

    close_unused_pipes(&data);

    timestamp_t lstamp = get_lamport_time();

    int start_messages_counter = data.procnum - 1;
    int done_messages_counter = data.procnum - 1;

    while(start_messages_counter) {
        if(receive_any(&data, &awaited_message) == 0) {
            if(awaited_message.s_header.s_type == STARTED) {
                start_messages_counter--;
                lamport_update(awaited_message.s_header.s_local_time);
                lstamp = get_lamport_time();
            }

            if(awaited_message.s_header.s_type == DONE) {
                done_messages_counter--;
            }
        }
    }

    printf(log_received_all_started_fmt, lstamp, data.recent_pid);
    events_info(log_received_all_started_fmt, lstamp, data.recent_pid);

    while(done_messages_counter) {
        if(receive_any(&data, &awaited_message) > -1) {
            if(awaited_message.s_header.s_type == DONE) {
                done_messages_counter--;
                lamport_update(awaited_message.s_header.s_local_time);
                lstamp = get_lamport_time();
            }
        }
    }

    printf(log_received_all_done_fmt, lstamp, data.recent_pid);
    events_info(log_received_all_done_fmt, lstamp, data.recent_pid);

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
