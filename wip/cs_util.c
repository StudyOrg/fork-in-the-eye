#include "cs_util.h"

#include "queue.h"
#include "pa2345.h"
#include "router.h"
#include "lamport_time.h"
#include "banking.h"

static int requested = 0;

int request_cs(const void * self) {
    if(requested == 0) {
        Message msg;
        const RouterWrapper* csdata = self;

        timestamp_t tm = get_lamport_time();
        msg.s_header.s_type = CS_REQUEST;
        msg.s_header.s_payload_len = 0;
        msg.s_header.s_local_time = tm;
        send_multicast(csdata->data, &msg);

        CSQueueAdd(csdata->data->recent_pid, tm);

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

        CSQueueDelete(csdata->data->recent_pid);

        requested = 0;
        return 1;
    } else
        return 2;
}
