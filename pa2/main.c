#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <wait.h>

#include "log.h"
#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "banking.h"
#include "pipes_util.h"
#include "util.h"
#include "router.h"

void doChild(void *, int, int);

int main(int argc, char *argv[]) {
    init_log();

    TEST(argc < 4, "Usage: pa2 -p N [S1...SN]");

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

                log.pipes_info("The pipe %d ===> %d was created\n", j, i);
            }
        }
    }

    Message msg, resMsg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = 0;

    for(int i = 1; i < data.procnum; i++) {
        pid = fork();

        if(pid < 0) {
            fprintf(stderr, "Error creating the child\n");
            exit(1);
        } else if (pid == 0) {
            doChild(&data, i, atoi(argv[i-1]));
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
    log.events_info(log_received_all_started_fmt, tm, data.recent_pid);

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
    log.events_info(log_received_all_done_fmt, tm, data.recent_pid);

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

void doChild(void *parentData, int recent_pid, int initBalance) {
    Router *data = (Router*)parentData;

    data->recent_pid = recent_pid;
    int done_msgs = data->procnum - 2;
    Message msg, resMsg;

    BalanceHistory history;
    history.s_id = recent_pid;
    history.s_history_len = 0;

    timestamp_t tm = get_physical_time();

    balance_t childBalance = initBalance;

    history.s_history[history.s_history_len].s_balance = childBalance;
    history.s_history[history.s_history_len].s_time = tm;
    history.s_history[history.s_history_len].s_balance_pending_in = 0;

    close_unused_pipes(data);

    log.events_info(log_started_fmt, tm, history.s_id, getpid(), getppid(),
                    history.s_history[history.s_history_len].s_balance);
    printf(log_started_fmt, tm, history.s_id, getpid(), getppid(),
           history.s_history[history.s_history_len].s_balance);

    msg.s_header.s_type = STARTED;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    sprintf(msg.s_payload, log_started_fmt, tm, history.s_id, getpid(),
            getppid(), history.s_history[history.s_history_len].s_balance);
    msg.s_header.s_payload_len = strlen(msg.s_payload);
    send(data, 0, &msg);

    history.s_history_len ++;

    while(1 && done_msgs) {
        tm = get_physical_time();

        /*
        	Set balance for empty timestamps
        */
        BalanceState balance;
        balance.s_balance_pending_in = 0;
        balance.s_balance = childBalance;
        balance.s_time = get_physical_time();
        history.s_history[history.s_history_len] = balance;
        history.s_history_len ++;

        if(receive_any(data, &resMsg) == 0) {
            if(resMsg.s_header.s_type == DONE) {
                done_msgs--;
            }
            if(resMsg.s_header.s_type == STOP) {
                msg.s_header.s_type = DONE;
                sprintf(msg.s_payload, log_done_fmt, tm, data->recent_pid, childBalance);
                msg.s_header.s_payload_len = strlen(msg.s_payload);
                send_multicast(data, &msg);

                log.events_info(log_done_fmt, tm, data->recent_pid, childBalance);
                printf(log_done_fmt, tm, data->recent_pid, childBalance);
            }
            if(resMsg.s_header.s_type == TRANSFER) {
                TransferOrder order;
                memcpy(&order, resMsg.s_payload, resMsg.s_header.s_payload_len);

                if(order.s_src == data->recent_pid) {
                    log.events_info(log_transfer_out_fmt, tm, data->recent_pid, order.s_amount, order.s_dst);
                    printf(log_transfer_out_fmt, tm, data->recent_pid, order.s_amount,
                           order.s_dst);
                    send(data, order.s_dst, &resMsg);

                    childBalance -= order.s_amount;

                    BalanceState balance;
                    balance.s_balance_pending_in = 0;
                    balance.s_balance = childBalance;
                    balance.s_time = tm;

                    history.s_history[history.s_history_len] = balance;
                }
                if(order.s_dst == data->recent_pid) {
                    log.events_info(log_transfer_in_fmt, tm, data->recent_pid, order.s_amount, order.s_src);
                    printf(log_transfer_in_fmt, tm, data->recent_pid, order.s_amount,
                           order.s_src);

                    childBalance += order.s_amount;

                    BalanceState balance;
                    balance.s_balance_pending_in = 0;
                    balance.s_balance = childBalance;
                    balance.s_time = tm;

                    history.s_history[history.s_history_len] = balance;

                    msg.s_header.s_type = ACK;
                    msg.s_header.s_magic = MESSAGE_MAGIC;
                    msg.s_header.s_local_time = tm;
                    msg.s_header.s_payload_len = 0;
                    send(data, 0, &msg);
                }
            }
        }
    }

    struct timespec tmr;
    tmr.tv_sec = 0;
    tmr.tv_nsec = 50000000;
    nanosleep(&tmr, NULL);

    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = BALANCE_HISTORY;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_payload_len = sizeof(history);
    memcpy(msg.s_payload, &history, sizeof(history));

    send(data, 0, &msg);
}

void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount) {
    TransferOrder order;
    order.s_src = src;
    order.s_dst = dst;
    order.s_amount = amount;

    Message msg, resMsg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = TRANSFER;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_payload_len = sizeof(order);
    memcpy(msg.s_payload, &order, sizeof(order));

    send(parent_data, src, &msg);

    while(1) {
        if(receive(parent_data, dst, &resMsg) == 0) {
            if(resMsg.s_header.s_type == ACK)
                break;
        }
    }
}
