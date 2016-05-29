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

void doChild(void *, int, int, int);

static timestamp_t lamportStamp = 0;

timestamp_t get_lamport_time() {
    return ++lamportStamp;
}

timestamp_t max(timestamp_t a, timestamp_t b) {
    if (a>b)
        return a;
    else
        return b;
}

int main(int argc, char *argv[]) {
		init_log();

    int pid;
    Router data;
    int start_msgs, done_msgs;

    int mutexfl = 0;

    TEST(argc < 3, "Usage: pa4 -p N [--mutexl]");

    data.procnum = atoi(argv[2]);
    TEST(data.procnum < 2, "Number of process must be >= 2");

    if (argc == 4) {
        mutexfl = 1;
    }

    ++data.procnum;

    data.recent_pid = 0;

    for(int i = 0; i < data.procnum; i++) {
        for(int j = 0; j < data.procnum; j++) {
            if(j==i) {
                data.routes[i][j][IN] = -1;
                data.routes[i][j][OUT] = -1;
            } else {
                TEST(pipe(data.routes[i][j]) < 0, "pipe: error while creating\n");

                set_nonlock(data.routes[i][j][IN]);
                set_nonlock(data.routes[i][j][IN]);

								pipes_info("pipe: created for %d <-> %d\n", j, i);
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
            doChild(&data, i, 0, mutexfl);
            exit(0);
        }
    }

    close_unused_pipes(&data);

    timestamp_t tm = get_lamport_time();
    start_msgs = data.procnum - 1;
    done_msgs = data.procnum - 1;

    while(start_msgs) {
        if(receive_any(&data, &resMsg) > -1) {
            if(resMsg.s_header.s_type == STARTED) {
                start_msgs--;
                lamportStamp = max(lamportStamp, resMsg.s_header.s_local_time);
                tm = get_lamport_time();
            }
            if(resMsg.s_header.s_type == DONE) {
                done_msgs--;
                lamportStamp = max(lamportStamp, resMsg.s_header.s_local_time);
                tm = get_lamport_time();
            }
        }
    }

		events_info(log_received_all_started_fmt, tm, data.recent_pid);

    while(done_msgs) {
        if(receive_any(&data, &resMsg) > -1) {
            if(resMsg.s_header.s_type == DONE) {
                done_msgs--;
                lamportStamp = max(lamportStamp, resMsg.s_header.s_local_time);
                tm = get_lamport_time();
            }
        }
    }

		events_info(log_received_all_done_fmt, tm, data.recent_pid);

    for(int i = 0; i < data.procnum; i++) {
        wait(&i);
    }

release_log();

    return 0;
}

void doChild(void *parentData, int lid, int initBalance, int mutexfl) {
    Message msg, resMsg;

    Router* data = parentData;

    int start_msgs = data->procnum - 2;
    int done_msgs = data->procnum - 2;

    timestamp_t tm = lamportStamp;

    data->recent_pid = lid;

    close_unused_pipes(data);

    balance_t childBalance = initBalance;

		events_info(log_started_fmt, tm, data->recent_pid, getpid(), getppid(), childBalance);

    tm = get_lamport_time();
    msg.s_header.s_type = STARTED;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    sprintf(msg.s_payload, log_started_fmt, tm, data->recent_pid, getpid(), getppid(), childBalance);
    msg.s_header.s_payload_len = strlen(msg.s_payload);
    msg.s_header.s_local_time = tm;
    send_multicast(data, &msg);

    while(start_msgs) {
        if(receive_any(data, &resMsg) > -1) {
            if(resMsg.s_header.s_type == STARTED)
                start_msgs--;
            if(resMsg.s_header.s_type == DONE)
                done_msgs--;
        }
    }

		events_info(log_received_all_started_fmt, tm, data->recent_pid);

    int replyCounter = 0;
    int printIterator = 0;
    int printMax = data->recent_pid * 5;
    while(done_msgs || printIterator < printMax) {
        int rPid = receive_any(data, &resMsg);
        if(rPid > -1) {
            if(resMsg.s_header.s_type == DONE) {
                lamportStamp = max(lamportStamp, resMsg.s_header.s_local_time);
                tm = get_lamport_time();

                done_msgs--;
            }

            if(resMsg.s_header.s_type == CS_REQUEST) {
                lamportStamp = max(lamportStamp, resMsg.s_header.s_local_time);
                tm = get_lamport_time();

                add_item(rPid, resMsg.s_header.s_local_time);

                tm = get_lamport_time();
                msg.s_header.s_type = CS_REPLY;
                msg.s_header.s_payload_len = 0;
                msg.s_header.s_local_time = tm;
                send(data, rPid, &msg);
            }

            if(resMsg.s_header.s_type == CS_RELEASE) {
                lamportStamp = max(lamportStamp, resMsg.s_header.s_local_time);
                tm = get_lamport_time();

                delete_item(rPid);
            }

            if(resMsg.s_header.s_type == CS_REPLY) {
                lamportStamp = max(lamportStamp, resMsg.s_header.s_local_time);
                tm = get_lamport_time();

                replyCounter ++;
            }
        }

        if(mutexfl == 1) {
            RouterWrapper csdata;
            csdata.data = data;

            if(printIterator < printMax)
                request_cs(&csdata);

            item_t *head = get_head();
            if(head != NULL) {
                if(replyCounter == data->procnum - 2 && head->pid == data->recent_pid) {
                    char str[MAX_PAYLOAD_LEN];
                    sprintf(str, log_loop_operation_fmt, data->recent_pid, printIterator+1, printMax);
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
						events_info(log_done_fmt, tm, data->recent_pid, childBalance);

            msg.s_header.s_type = DONE;
            sprintf(msg.s_payload, log_done_fmt, tm, data->recent_pid, childBalance);
            msg.s_header.s_payload_len = strlen(msg.s_payload);
            send_multicast(data, &msg);

            printIterator ++;
        }
    }

		events_info(log_received_all_done_fmt, tm, data->recent_pid);

    exit(0);
}

int requested = 0;

int request_cs(const void * self) {
    if(requested == 0) {
        Message msg;
        const RouterWrapper* csdata = self;

        timestamp_t tm = get_lamport_time();
        msg.s_header.s_type = CS_REQUEST;
        msg.s_header.s_payload_len = 0;
        msg.s_header.s_local_time = tm;
        send_multicast(csdata->data, &msg);

        add_item(csdata->data->recent_pid, tm);

        requested = 1;
        return 1;
    } else
        return 2;
}

int release_cs(const void * self) {
    if(requested == 1) {
        Message msg;
        const RouterWrapper* csdata = self;

        timestamp_t tm = get_lamport_time();
        msg.s_header.s_type = CS_RELEASE;
        msg.s_header.s_payload_len = 0;
        msg.s_header.s_local_time = tm;
        send_multicast(csdata->data, &msg);

        delete_item(csdata->data->recent_pid);

        requested = 0;
        return 1;
    } else
        return 2;
}
