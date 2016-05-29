#pragma once

static int _cs_use_lock = 0;

int request_cs(const void * self);
int release_cs(const void * self);
