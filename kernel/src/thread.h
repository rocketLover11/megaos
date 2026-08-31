#pragma once
#include <stdint.h>

enum ThreadState { READY, RUNNING, BLOCKED };

struct Thread {
    uint64_t rsp;
    void *kernel_stack;
    ThreadState state;
    uint64_t id;
    const char *name;
};

Thread *thread_create(void (*entry)(), const char *name);

extern "C" void context_switch(uint64_t *old_rsp, uint64_t new_rsp);