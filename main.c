#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>

#include "ipc.h"
#include "common.h"
#include "pa1.h"

#include "router.h"
#include "util.h"
#include "pipes_util.h"

int main(int argc, char * argv[]) {
	Router router;

	int opt = 0;
	while( (opt = getopt( argc, argv, "p:" )) != -1 ) {
		switch( opt ) {
			case 'p':
				router.procnum = atoi(optarg) + 1;
				break;
			case '?':
			default:
				fprintf(stderr, "Bad parameter\n");
				exit(1);
		}
	}

	router.recent_pid = 0;

	FILE *pipes_logf, *events_logf;

	pipes_logf = fopen(pipes_log, "w");
	TEST(pipes_logf == NULL, "Can't open pipes log file");

	events_logf = fopen(events_log, "a");
	TEST(events_logf == NULL, "Can't open events log file");

	for(int i = 0; i < router.procnum; i++) {
		for(int j = 0; j < router.procnum; j++) {
			if(j == i) {
				router.routes[i][j].filedes[IN] = -1;
				router.routes[i][j].filedes[OUT] = -1;
			} else {
				TEST(pipe(router.routes[i][j].filedes) < 0, "Can't create pipe");

				int mode = fcntl(router.routes[i][j].filedes[0], F_GETFL);
				fcntl(router.routes[i][j].filedes[0], F_SETFL, mode | O_NONBLOCK);

				fprintf(pipes_logf, "Created route %d <--> %d\n", j, i);
			}
		}
	}

	fclose(pipes_logf);

	Message send_msg, rec_msg;
	send_msg.s_header.s_magic = MESSAGE_MAGIC;
	send_msg.s_header.s_local_time = 0;

	pid_t pid;
	int start_msgs, done_msgs;

	for(int i = 1; i < router.procnum; i++) {
		pid = fork();

		if(pid < 0) {
			fprintf(stderr, "Error creating the child\n");
			exit(1);
		} else if (pid == 0) {

			start_msgs = router.procnum - 2;
			done_msgs = router.procnum - 2;

			router.recent_pid = i;

			release_free_pipes(&router);

			fprintf(events_logf, log_started_fmt, router.recent_pid, getpid(), getppid());
			fflush(events_logf);
			printf(log_started_fmt, router.recent_pid, getpid(), getppid());

			send_msg.s_header.s_type = STARTED;
			sprintf(send_msg.s_payload, log_started_fmt, router.recent_pid, getpid(), getppid());
			send_msg.s_header.s_payload_len = strlen(send_msg.s_payload);
			send_multicast(&router, &send_msg);

			while(start_msgs) {
				if(receive_any(&router, &rec_msg) == 0) {
					if(rec_msg.s_header.s_type == STARTED)
						start_msgs--;
					if(rec_msg.s_header.s_type == DONE)
						done_msgs--;
				}
			}

			fprintf(events_logf, log_received_all_started_fmt, router.recent_pid);
			fflush(events_logf);
			printf(log_received_all_started_fmt, router.recent_pid);

			fprintf(events_logf, log_done_fmt, router.recent_pid);
			fflush(events_logf);
			printf(log_done_fmt, router.recent_pid);

			send_msg.s_header.s_type = DONE;
			sprintf(send_msg.s_payload, log_done_fmt, router.recent_pid);
			send_msg.s_header.s_payload_len = strlen(send_msg.s_payload);
			send_multicast(&router, &send_msg);

			while(done_msgs) {
				if(receive_any(&router, &rec_msg) == 0) {
					if(rec_msg.s_header.s_type == DONE)
						done_msgs--;
				}
			}

			fprintf(events_logf, log_received_all_done_fmt, router.recent_pid);
			fflush(events_logf);
			printf(log_received_all_done_fmt, router.recent_pid);

			fclose(events_logf);
			exit(0);
		}
	}

	release_free_pipes(&router);

		start_msgs = router.procnum - 1;
		done_msgs = router.procnum - 1;

		while(start_msgs) {
			if(receive_any(&router, &rec_msg) == 0) {
				if(rec_msg.s_header.s_type == STARTED)
					start_msgs--;
				if(rec_msg.s_header.s_type == DONE)
					done_msgs--;
			}
		}

		fprintf(events_logf, log_received_all_started_fmt, router.recent_pid);
		fflush(events_logf);
		printf(log_received_all_started_fmt, router.recent_pid);

		while(done_msgs) {
			if(receive_any(&router, &rec_msg) == 0) {
				if(rec_msg.s_header.s_type == DONE)
					done_msgs--;
			}
		}

		fprintf(events_logf, log_received_all_done_fmt, router.recent_pid);
		fflush(events_logf);
		printf(log_received_all_done_fmt, router.recent_pid);

		fclose(events_logf);

		for(int i = 0; i < router.procnum; i++) {
			wait(&i);
		}

		return 0;

	return 0;
}
