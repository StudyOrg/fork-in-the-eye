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
#include "cs_request_queue.h"
#include "cs_util.h"

void service_workers(void *global_data, int recent_pid) {
    Router *data = (Router*)global_data;

    data->recent_pid = recent_pid;

    int start_messages_counter = data->procnum - 2;
    int done_messages_counter = data->procnum - 2;

    timestamp_t lstamp = lamport_global_stamp;

    close_unused_pipes(data);

    events_info(log_started_fmt, lstamp, data->recent_pid, getpid(), getppid(), 0);
    printf(log_started_fmt, lstamp, data->recent_pid, getpid(), getppid(), 0);

    Message msg;

    lstamp = get_lamport_time();
    msg.s_header.s_type = STARTED;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_local_time = lstamp;
    sprintf(msg.s_payload, log_started_fmt, lstamp, data->recent_pid, getpid(), getppid(), 0);
    msg.s_header.s_payload_len = strlen(msg.s_payload);

    send_multicast(data, &msg);

    Message answer;

    while(start_messages_counter) {
        if(receive_any(data, &answer) > -1) {
            if(answer.s_header.s_type == STARTED) {
                start_messages_counter--;
            }
            if(answer.s_header.s_type == DONE) {
                done_messages_counter--;
            }
        }
    }

    events_info(log_received_all_started_fmt, lstamp, data->recent_pid);
    printf(log_received_all_started_fmt, lstamp, data->recent_pid);

    int replyCounter = 0;
    int printIterator = 0;
    int printMax = data->recent_pid * 5;

    while(done_messages_counter || printIterator < printMax) {
        int rPid = receive_any(data, &answer);
        if(rPid > -1) {
            if(answer.s_header.s_type == DONE) {
                lamport_update(answer.s_header.s_local_time);
                lstamp = lamport_global_stamp;

                done_messages_counter--;
            }

            if(answer.s_header.s_type == CS_REQUEST) {
                lamport_update(answer.s_header.s_local_time);
                lstamp = get_lamport_time();

                CSQueueAdd(rPid, answer.s_header.s_local_time);

                lstamp = get_lamport_time();
                msg.s_header.s_type = CS_REPLY;
                msg.s_header.s_payload_len = 0;
                msg.s_header.s_local_time = lstamp;
                send(data, rPid, &msg);
            }

            if(answer.s_header.s_type == CS_RELEASE) {
                lamport_update(answer.s_header.s_local_time);
                lstamp = get_lamport_time();

                CSQueueDelete(rPid);
            }

            if(answer.s_header.s_type == CS_REPLY) {
                lamport_update(answer.s_header.s_local_time);
                lstamp = get_lamport_time();

                replyCounter ++;
            }
        }

        if(_cs_use_lock == 1) {
            RouterWrapper csdata;
            csdata.rt = data;

            if(printIterator < printMax)
                request_cs(&csdata);

            CSRequestInfo *head = CSQueuePeek();
            if(head != NULL) {
                if(replyCounter == data->procnum - 2 && head->pid == data->recent_pid) {
                    char str[MAX_PAYLOAD_LEN];
                    sprintf(str, log_loop_operation_fmt, data->recent_pid, printIterator + 1, printMax);
                    print(str);

                    printIterator++;

                    release_cs(&csdata);
                    replyCounter = 0;
                }
            }
        } else {
            if(printIterator < printMax) {
                char str[MAX_PAYLOAD_LEN];
                sprintf(str, log_loop_operation_fmt, data->recent_pid, printIterator+1, printMax);
                print(str);
                printIterator++;
            }
        }

        if(printIterator == printMax) {
            events_info(log_done_fmt, lstamp, data->recent_pid, 0);
            printf(log_done_fmt, lstamp, data->recent_pid, 0);

            msg.s_header.s_type = DONE;
            sprintf(msg.s_payload, log_done_fmt, lstamp, data->recent_pid, 0);
            msg.s_header.s_payload_len = strlen(msg.s_payload);
            send_multicast(data, &msg);

            printIterator ++;
        }
    }

    events_info(log_received_all_started_fmt, lstamp, data->recent_pid);
    printf(log_received_all_started_fmt, lstamp, data->recent_pid);

    receive_sleep();
}
