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

int main(int argc, char *argv[]) {
    init_log();

    TEST(argc < 5, "Usage: pa2 -p N [S1...SN], N >= 2");

    Router data;

    int option;
    while((option = getopt(argc, argv, "p:")) != -1) {
        switch (option) {
        case 'p':
            data.procnum = atoi(optarg) + 1;
            break;
        case '?':
        default:
            DIE("Bad parametes");
        }
    }

    argc -= optind;
    argv += optind;

    int pid;
    int start_msgs, done_msgs;

    data.recent_pid = 0;

    for(int i = 0; i < data.procnum; i++) {
        for(int j = 0; j < data.procnum; j++) {
            if(j==i) {
                data.routes[i][j][IN] = -1;
                data.routes[i][j][OUT] = -1;
            } else {
                int pipe_status = pipe(data.routes[i][j]);
                TEST(pipe_status < 0, "Can't create pipe");

                set_nonlock(data.routes[i][j][IN]);
                set_nonlock(data.routes[i][j][OUT]);

                pipes_info("The pipe %d ===> %d was created\n", j, i);
            }
        }
    }

    Message msg, resMsg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = 0;

    for(int i = 1; i < data.procnum; i++) {
        pid = fork();

        TEST(pid < 0, "Error creating the child\n");

        if (pid == 0) {
            service_account(&data, i, atoi(argv[i-1]));
            exit(0);
        }
    }

    close_unused_pipes(&data);

    timestamp_t tm = get_physical_time();
    start_msgs = data.procnum - 1;
    done_msgs = data.procnum - 1;

    while(start_msgs) {
        if(receive_any(&data, &resMsg) == 0) {
            if(resMsg.s_header.s_type == STARTED)
                start_msgs--;
            if(resMsg.s_header.s_type == DONE)
                done_msgs--;
        }
    }

    tm = get_physical_time();

    printf(log_received_all_started_fmt, tm, data.recent_pid);
    events_info(log_received_all_started_fmt, tm, data.recent_pid);

    bank_robbery(&data, data.procnum-1);

    msg.s_header.s_type = STOP;
    msg.s_header.s_payload_len = 0;
    send_multicast(&data, &msg);

    while(done_msgs) {
        if(receive_any(&data, &resMsg) == 0) {
            if(resMsg.s_header.s_type == DONE)
                done_msgs--;
        }
    }

    tm = get_physical_time();

    printf(log_received_all_done_fmt, tm, data.recent_pid);
    events_info(log_received_all_done_fmt, tm, data.recent_pid);

    int history_msgs = data.procnum - 1;
    AllHistory allHistory;
    allHistory.s_history_len = history_msgs;

    do {
        if(receive_any(&data, &resMsg) == 0) {
            BalanceHistory hist;
            memcpy(&hist, resMsg.s_payload, resMsg.s_header.s_payload_len);

            if(resMsg.s_header.s_type == BALANCE_HISTORY) {
                allHistory.s_history[hist.s_id-1] = hist;
                history_msgs--;
            }
        }
    } while(history_msgs);

    print_history(&allHistory);

    while (1) {
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
