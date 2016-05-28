#pragma once

extern volatile timestamp_t lamport_global_stamp;

void lamport_update(timestamp_t);
