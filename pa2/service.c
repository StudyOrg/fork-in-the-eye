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

void service_account(void *parentData, int recent_pid, int initBalance) {
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

    events_info(log_started_fmt, tm, history.s_id, getpid(), getppid(),
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

                events_info(log_done_fmt, tm, data->recent_pid, childBalance);
                printf(log_done_fmt, tm, data->recent_pid, childBalance);
            }
            if(resMsg.s_header.s_type == TRANSFER) {
                TransferOrder order;
                memcpy(&order, resMsg.s_payload, resMsg.s_header.s_payload_len);

                if(order.s_src == data->recent_pid) {
                    events_info(log_transfer_out_fmt, tm, data->recent_pid, order.s_amount, order.s_dst);
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
                    events_info(log_transfer_in_fmt, tm, data->recent_pid, order.s_amount, order.s_src);
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

    receive_sleep();

    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = BALANCE_HISTORY;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_payload_len = sizeof(history);
    memcpy(msg.s_payload, &history, sizeof(history));

    send(data, 0, &msg);
}
