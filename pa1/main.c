#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ipc.h"
#include "common.h"
#include "pa1.h"

#include "router.h"
#include "log.h"
#include "pipes_util.h"

int main(int argc, char * argv[]) {
    init_log();

    Router router;

    int opt = 0;
    while ((opt = getopt( argc, argv, "p:" )) != -1) {
        switch (opt) {
        case 'p':
            router.procnum = atoi(optarg);
            break;
        default:
            DIE("Bad parameter\n");
        }
    }

    TEST(router.procnum <= 0, "Bad number of process");

    router.procnum += 1; // Включая родительский процесс

    router.recent_pid = 0;

    for (int i = 0; i < router.procnum; i++) {
        for (int j = 0; j < router.procnum; j++) {
            int* filedes = router.routes[i][j].filedes;

            if (j == i) {
                filedes[IN] = -1;
                filedes[OUT] = -1;
            } else {
                TEST(pipe(filedes) < 0, "Can't create pipe");

                int mode = fcntl(filedes[IN], F_GETFL);
                fcntl(filedes[IN], F_SETFL, mode | O_NONBLOCK);

                log.pipes_info("Created pipe %d <--> %d\n", j, i);
            }
        }
    }

    Message send_msg, rec_msg;
    send_msg.s_header.s_magic = MESSAGE_MAGIC;
    send_msg.s_header.s_local_time = 0;

    pid_t pid;
    int start_msgs, done_msgs;

    for (int i = 1; i < router.procnum; i++) {
        pid = fork();

        TEST(pid < 0, "Error creating the child\n");

        if (pid == 0) {
            // Child
            start_msgs = router.procnum - 2;
            done_msgs = router.procnum - 2;

            router.recent_pid = i;

            close_unused_pipes(&router);

            log.events_info(log_started_fmt, router.recent_pid, getpid(), getppid());
            log.info(log_started_fmt, router.recent_pid, getpid(), getppid());

            send_msg.s_header.s_type = STARTED;
            sprintf(send_msg.s_payload, log_started_fmt, router.recent_pid, getpid(), getppid());
            send_msg.s_header.s_payload_len = strlen(send_msg.s_payload);
            send_multicast(&router, &send_msg);

            while (start_msgs) {
                if (receive_any(&router, &rec_msg) == 0) {
                    if (rec_msg.s_header.s_type == STARTED)
                        start_msgs--;
                    if (rec_msg.s_header.s_type == DONE)
                        done_msgs--;
                }
            }

            log.events_info(log_received_all_started_fmt, router.recent_pid);
            log.info(log_received_all_started_fmt, router.recent_pid);

            log.events_info(log_done_fmt, router.recent_pid);
            log.info(log_done_fmt, router.recent_pid);

            send_msg.s_header.s_type = DONE;
            sprintf(send_msg.s_payload, log_done_fmt, router.recent_pid);
            send_msg.s_header.s_payload_len = strlen(send_msg.s_payload);
            send_multicast(&router, &send_msg);

            while (done_msgs) {
                if (receive_any(&router, &rec_msg) == 0) {
                    if (rec_msg.s_header.s_type == DONE)
                        done_msgs--;
                }
            }

            log.events_info(log_received_all_done_fmt, router.recent_pid);
            log.info(log_received_all_done_fmt, router.recent_pid);

            exit(0);
        }
    }

    close_unused_pipes(&router);

    start_msgs = router.procnum - 1;
    done_msgs = router.procnum - 1;

    while (start_msgs) {
        if (receive_any(&router, &rec_msg) == 0) {
            if (rec_msg.s_header.s_type == STARTED)
                start_msgs--;
            if (rec_msg.s_header.s_type == DONE)
                done_msgs--;
        }
    }

    log.events_info(log_received_all_started_fmt, router.recent_pid);
    log.info(log_received_all_started_fmt, router.recent_pid);

    while (done_msgs) {
        if (receive_any(&router, &rec_msg) == 0) {
            if (rec_msg.s_header.s_type == DONE)
                done_msgs--;
        }
    }

    log.events_info(log_received_all_done_fmt, router.recent_pid);
    log.info(log_received_all_done_fmt, router.recent_pid);
    // Дожидаемся завершения всех дочерних процессов
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

    void release_log();

    return 0;
}
