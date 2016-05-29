#pragma once

extern int _cs_use_lock;

int request_cs(const void * self);
int release_cs(const void * self);
