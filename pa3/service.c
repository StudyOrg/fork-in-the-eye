#include "service.h"

#include "stdversion.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "log.h"
#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "banking.h"
#include "util.h"
#include "router.h"
#include "lamport_time.h"

#define HISTORY history.s_history[history.s_history_len]

void service_account(void *global_data, int recent_pid, int global_balance) {
    Router *data = (Router*)global_data;

    data->recent_pid = recent_pid;
    int done_msgs = data->procnum - 2;

    BalanceHistory history;
    history.s_id = recent_pid;
    history.s_history_len = 0;

    timestamp_t lstamp = lamport_global_stamp;
    timestamp_t lstamp_mem = lstamp;

    balance_t local_balance = global_balance;

    HISTORY.s_balance = local_balance;
    HISTORY.s_time = lstamp;
    HISTORY.s_balance_pending_in = 0;

    close_unused_pipes(data);

    events_info(log_started_fmt, lstamp, history.s_id, getpid(), getppid(), HISTORY.s_balance);
    printf(log_started_fmt, lstamp, history.s_id, getpid(), getppid(), HISTORY.s_balance);

    Message msg;

    lstamp = get_lamport_time();
    msg.s_header.s_type = STARTED;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = lstamp;
    sprintf(msg.s_payload, log_started_fmt, lstamp, history.s_id, getpid(), getppid(), HISTORY.s_balance);
    msg.s_header.s_payload_len = strlen(msg.s_payload);

    send(data, 0, &msg);

    ++history.s_history_len;

    HISTORY.s_balance = local_balance;
    HISTORY.s_time = lstamp;
    HISTORY.s_balance_pending_in = 0;

    ++history.s_history_len;

    Message answer;

    while(done_msgs) {
        if(receive_any(data, &answer) == 0) {
            if(answer.s_header.s_type == DONE) {
                lamport_update(answer.s_header.s_local_time);
                lstamp = get_lamport_time();

                --done_msgs;
            }

            else if(answer.s_header.s_type == TRANSFER) {
                if(answer.s_header.s_local_time > lstamp_mem) {
                    timestamp_t buf = lstamp_mem + 1;
                    while(buf != answer.s_header.s_local_time + 1) {
                        BalanceState balance;

                        balance.s_balance_pending_in = 0;
                        balance.s_balance = local_balance;
                        balance.s_time = buf;
                        HISTORY = balance;
                        ++history.s_history_len;

                        ++buf;
                    }
                }

                lamport_update(answer.s_header.s_local_time);
                lstamp = get_lamport_time();

                TransferOrder order;
                memcpy(&order, answer.s_payload, answer.s_header.s_payload_len);

                if(order.s_src == data->recent_pid) {
                    BalanceState balance;

                    balance.s_balance_pending_in = 0;
                    balance.s_balance = local_balance;
                    balance.s_time = lstamp;
                    HISTORY = balance;
                    ++history.s_history_len;

                    lstamp = get_lamport_time();

                    answer.s_header.s_local_time = lstamp;

                    events_info(log_transfer_out_fmt, lstamp, data->recent_pid, order.s_amount, order.s_dst);
                    printf(log_transfer_out_fmt, lstamp, data->recent_pid, order.s_amount, order.s_dst);

                    send(data, order.s_dst, &answer);

                    local_balance -= order.s_amount;

                    balance.s_balance_pending_in = order.s_amount;
                    balance.s_balance = local_balance;
                    balance.s_time = lstamp;

                    lstamp_mem = lstamp;

                    HISTORY = balance;
                    ++history.s_history_len;
                }

                if(order.s_dst == data->recent_pid) {
                    events_info(log_transfer_in_fmt, lstamp, data->recent_pid, order.s_amount, order.s_src);
                    printf(log_transfer_in_fmt, lstamp, data->recent_pid, order.s_amount, order.s_src);

                    BalanceState balance;
                    balance.s_balance_pending_in = 0;
                    balance.s_balance = local_balance;
                    balance.s_time = lstamp;

                    HISTORY = balance;
                    ++history.s_history_len;

                    local_balance += order.s_amount;

                    balance.s_balance_pending_in = 0;
                    balance.s_balance = local_balance;
                    balance.s_time = lstamp;

                    lstamp_mem = lstamp;

                    HISTORY = balance;
                    ++history.s_history_len;

                    lstamp = get_lamport_time();
                    msg.s_header.s_type = ACK;
                    msg.s_header.s_magic = MESSAGE_MAGIC;
                    msg.s_header.s_local_time = lstamp;
                    msg.s_header.s_payload_len = 0;

                    send(data, 0, &msg);
                }
            }

            else if(answer.s_header.s_type == STOP) {
                lamport_update(answer.s_header.s_local_time);

                lstamp = get_lamport_time();
                lstamp = get_lamport_time();

                msg.s_header.s_type = DONE;
                sprintf(msg.s_payload, log_done_fmt, lstamp, data->recent_pid, local_balance);
                msg.s_header.s_payload_len = strlen(msg.s_payload);
                msg.s_header.s_local_time = lstamp;
                send_multicast(data, &msg);

                events_info(log_done_fmt, lstamp, data->recent_pid, local_balance);
                printf(log_done_fmt, lstamp, data->recent_pid, local_balance);
            }
        }
    }

    if(answer.s_header.s_local_time >= lstamp_mem) {
        timestamp_t buf = lstamp_mem + 1;

        while(buf != answer.s_header.s_local_time) {
            BalanceState balance;
            balance.s_balance_pending_in = 0;
            balance.s_balance = local_balance;
            balance.s_time = buf;

            HISTORY = balance;
            ++history.s_history_len;

            ++buf;
        }
    }

    receive_sleep();

    lstamp = get_lamport_time();
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = BALANCE_HISTORY;
    msg.s_header.s_local_time = lstamp;
    msg.s_header.s_payload_len = sizeof(history);
    memcpy(msg.s_payload, &history, sizeof(history));

    send(data, 0, &msg);
}
