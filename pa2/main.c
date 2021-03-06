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

    Router data;
    /*   0  1 2  3  4 */
    /* pa2 -p 2 30 40 */
    TEST(argc < 5, "Usage: pa2 -p N [S1...SN], N >= 2");

    data.procnum = atoi(argv[2]);
    TEST(data.procnum < 2, "Number if process must be >= 2");
    argc -= 3; // pa2 + -p + N
    argv += 3;
    TEST(argc != data.procnum, "Number of process not equal to number of initial values");

    ++data.procnum;

    data.recent_pid = 0;

    for(int i = 0; i < data.procnum; i++) {
        for(int j = 0; j < data.procnum; j++) {
            if(j != i) {
                int pipe_status = pipe(data.routes[i][j]);
                TEST(pipe_status < 0, "Can't create pipe");

                set_nonlock(data.routes[i][j][IN]);
                set_nonlock(data.routes[i][j][OUT]);

                pipes_info("The pipe %d ===> %d was created\n", j, i);
            } else {
                data.routes[i][j][IN] = -1;
                data.routes[i][j][OUT] = -1;
            }
        }
    }

    Message msg, resMsg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = 0;

    for(int i = 1; i < data.procnum; i++) {
        pid_t pid = fork();

        TEST(pid < 0, "Error creating the child\n");

        if (pid == 0) {
            service_account(&data, i, atoi(argv[i-1]));
            exit(0);
        }
    }

    close_unused_pipes(&data);

    timestamp_t pstamp = get_physical_time();

    int start_messages_counter = data.procnum - 1;
    int done_messages_counter = data.procnum - 1;

    while(start_messages_counter) {
        if(receive_any(&data, &resMsg) == 0) {
            if(resMsg.s_header.s_type == STARTED)
                start_messages_counter--;
            if(resMsg.s_header.s_type == DONE)
                done_messages_counter--;
        }
    }

    pstamp = get_physical_time();

    printf(log_received_all_started_fmt, pstamp, data.recent_pid);
    events_info(log_received_all_started_fmt, pstamp, data.recent_pid);

    bank_robbery(&data, data.procnum - 1);

    msg.s_header.s_type = STOP;
    msg.s_header.s_payload_len = 0;
    send_multicast(&data, &msg);

    while(done_messages_counter) {
        if(receive_any(&data, &resMsg) == 0) {
            if(resMsg.s_header.s_type == DONE)
                done_messages_counter--;
        }
    }

    pstamp = get_physical_time();

    printf(log_received_all_done_fmt, pstamp, data.recent_pid);
    events_info(log_received_all_done_fmt, pstamp, data.recent_pid);

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
