#include "cs_util.h"
#include "pa2345.h"
#include "router.h"
#include "banking.h"
#include "lamport_time.h"
#include "cs_request_queue.h"

int request_cs(const void * self) {
    if(_cs_locked == 0) {
        Message msg;
        RouterWrapper const * const rt = (RouterWrapper*)self;

        timestamp_t tm = get_lamport_time();
        msg.s_header.s_type = CS_REQUEST;
        msg.s_header.s_payload_len = 0;
        msg.s_header.s_local_time = tm;
        send_multicast(rt->rt, &msg);

        CSQueueAdd(rt->rt->recent_pid, tm);

        _cs_locked = 1;
        return 1;
    } else {
        return 2;
    }
}

int release_cs(const void * self) {
    if(_cs_locked == 1) {
        Message msg;
        RouterWrapper const * const rt = self;

        timestamp_t tm = get_lamport_time();
        msg.s_header.s_type = CS_RELEASE;
        msg.s_header.s_payload_len = 0;
        msg.s_header.s_local_time = tm;
        send_multicast(rt->rt, &msg);

        CSQueueDelete(rt->rt->recent_pid);

        _cs_locked = 0;
        return 1;
    } else {
        return 2;
    }
}
