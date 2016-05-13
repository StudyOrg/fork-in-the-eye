#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "ipc.h"
#include "common.h"
#include "pa1.h"
#include "router.h"

int send(void * data, local_id dst, const Message * msg) {
  Router *rt = (Router*)data;

	uint16_t size = sizeof(msg->s_header) + msg->s_header.s_payload_len;

	if(write(rt->routes[dst][rt->recent_pid].filedes[1], msg, size) != size)
		return 1;

	return 0;
}

int send_multicast(void * data, const Message * msg) {
  Router *rt = (Router*)data;

	for(int i = 0; i < rt->procnum; i++) {
		if(i == rt->recent_pid)
			continue;
		if(send(data, i, msg) != 0)
			return 1;
	}
	return 0;
}

int receive(void * data, local_id from, Message * msg) {
  Router *rt = (Router*)data;

	uint16_t size = sizeof(msg->s_header);

	if(read(rt->routes[rt->recent_pid][from].filedes[0], &msg->s_header, size) != size)
		return -1;

	size = msg->s_header.s_payload_len;

	if(read(rt->routes[rt->recent_pid][from].filedes[0], &msg->s_payload, size) != size)
		return -1;

	return 0;
}

int receive_any(void * data, Message * msg) {
  Router *rt = (Router*)data;

	for(int i = 0; i < rt->procnum; i++) {
		if(i == rt->recent_pid)
			continue;
		if(receive(data, i, msg) == 0)
			return 0;
	}

	struct timespec tmr;
	tmr.tv_sec = 0;
	tmr.tv_nsec = 50000000;

	if(nanosleep(&tmr, NULL) < 0 ) {
		return -1;
	}

	return 1;
}
