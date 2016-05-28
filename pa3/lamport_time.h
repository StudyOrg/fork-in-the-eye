#pragma once

static timestamp_t lamport_global_stamp = 0;

void lamport_update(timestamp_t);
