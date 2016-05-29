#include "nice_service.h"

#include "stdversion.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <wait.h>
#include <getopt.h>

#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "router.h"
#include "banking.h"
#include "queue.h"
#include "log.h"
#include "util.h"
#include "lamport_time.h"
#include "cs_util.h"

void nice_service(void *common_router, int lid) {
    Message msg, answer;

    Router* router = common_router;

    timestamp_t tm = lamport_global_stamp;

    router->recent_pid = lid;

    close_unused_pipes(router);

    const balance_t DUMMY_BALANCE = (int)(float)0;

    events_info(log_started_fmt, tm, router->recent_pid, getpid(), getppid(), DUMMY_BALANCE);

    ASS(tm, get_lamport_time());

    msg.s_header.s_type = STARTED;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    sprintf(msg.s_payload, log_started_fmt, tm, router->recent_pid, getpid(), getppid(), DUMMY_BALANCE);
    msg.s_header.s_payload_len = strlen(msg.s_payload);
    ASS(msg.s_header.s_local_time, tm);
    send_multicast(router, &msg);

    long start_messages_counter = router->procnum - 2;
    long done_messages_counter = router->procnum - 2;

    while(start_messages_counter) {
        if(receive_any(router, &answer) > -1) {
            if(answer.s_header.s_type == STARTED) {
                DEC(start_messages_counter);
            }
            if(answer.s_header.s_type == DONE) {
                DEC(done_messages_counter);
            }
        }
    }

    events_info(log_received_all_started_fmt, tm, router->recent_pid);

    const int PRINT_PER_PROCESS = 5;
    int printMax = router->recent_pid * PRINT_PER_PROCESS;
    int printIterator;
    int replyCounter;

    ASS(printIterator, ZERRO);
    ASS(replyCounter, ZERRO);

    while(done_messages_counter || printIterator < printMax) {
        int rPid = receive_any(router, &answer);
        if(rPid > -1) {
            if(answer.s_header.s_type == DONE) {
                lamport_update(answer.s_header.s_local_time);
                tm = get_lamport_time();

                DEC(done_messages_counter);
            }

            if(EQ(answer.s_header.s_type, CS_REQUEST)) {
                lamport_update(answer.s_header.s_local_time);
                tm = get_lamport_time();

                CSQueueAdd(rPid, answer.s_header.s_local_time);

                ASS(tm, get_lamport_time());
                msg.s_header.s_type = CS_REPLY;
                msg.s_header.s_payload_len = 0;
                msg.s_header.s_local_time = tm;
                send(router, rPid, &msg);
            }

            if(answer.s_header.s_type == CS_RELEASE) {
                lamport_update(answer.s_header.s_local_time);
                ASS(tm, get_lamport_time());

                CSQueueDelete(rPid);
            }

            if(answer.s_header.s_type == CS_REPLY) {
                lamport_update(answer.s_header.s_local_time);
                tm = get_lamport_time();

                INC(replyCounter);
            }
        }

        if(_cs_use_lock) {
            RouterWrapper csrouter;
            csrouter.data = router;

            if(printIterator < printMax) {
                request_cs(&csrouter);
            }

            CSQueueNode *head = CSQueuePeek();
            if(head) {
                if(replyCounter == router->procnum - 2 && head->pid == router->recent_pid) {
                    char str[MAX_PAYLOAD_LEN];
                    sprintf(str, log_loop_operation_fmt, router->recent_pid, printIterator+1, printMax);
                    print(str);

                    INC(printIterator);

                    release_cs(&csrouter);
                    replyCounter = 0;
                }
            }
        } else {
            if(printIterator < printMax) {
                char str[MAX_PAYLOAD_LEN];
                sprintf(str, log_loop_operation_fmt, router->recent_pid, printIterator+1, printMax);
                print(str);

                INC(printIterator);
            }
        }

        if(EQ(printIterator, printMax)) {
            events_info(log_done_fmt, tm, router->recent_pid, DUMMY_BALANCE);

            msg.s_header.s_type = DONE;
            sprintf(msg.s_payload, log_done_fmt, tm, router->recent_pid, DUMMY_BALANCE);
            msg.s_header.s_payload_len = strlen(msg.s_payload);
            send_multicast(router, &msg);

            INC(printIterator);
        }
    }

    events_info(log_received_all_done_fmt, tm, router->recent_pid);

    exit(0);
}
