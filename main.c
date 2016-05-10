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

int main(int argc, char * argv[]) {
	Router router;

	int opt = 0;
	while( (opt = getopt( argc, argv, "p:" )) != -1 ) {
		switch( opt ) {
			case 'p':
				router.procnum = atoi(optarg);
				break;
			case '?':
			default:
				fprintf(stderr, "Bad parameter\n");
				exit(1);
		}
	}

	router.recent_pid = 0;

	FILE *pipes_logf, *events_logf;

	pipes_logf = fopen(events_log, "w");
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

				fprintf(pipes_logf, "The pipe %d ===> %d was created\n", j, i);
			}
		}
	}

	return 0;
}
