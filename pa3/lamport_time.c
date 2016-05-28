#include "banking.h"
#include "lamport_time.h"

volatile timestamp_t lamport_global_stamp = 0;

timestamp_t get_lamport_time() {
    ++lamport_global_stamp;
    return lamport_global_stamp;
}

timestamp_t max_timestamp(timestamp_t a, timestamp_t b) {
    return (a > b) ? a : b;
}

void lamport_update(timestamp_t t) {
    lamport_global_stamp = max_timestamp(lamport_global_stamp, t);
}
