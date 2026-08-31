#include "scheduler.h"
#include "fb_console.h"

#define MAX_THREADS 64

static Thread *threads[MAX_THREADS];
static int thread_count = 0;
static int current_index = -1;
static bool scheduler_started = false;

void scheduler_init() {
    thread_count = 0;
    current_index = -1;
    scheduler_started = false;
}

void scheduler_add_thread(Thread *t) {
    if (thread_count >= MAX_THREADS) {
        kprintf("[megakernel] Max threads exceeded!\n");
        return;
    }
    threads[thread_count++] = t;
}

Thread *scheduler_current() {
    if (current_index < 0) return nullptr;
    return threads[current_index];
}

void schedule() {
    if (thread_count == 0) return;

    if (!scheduler_started) {
        scheduler_started = true;
        current_index = 0;
        threads[0]->state = RUNNING;

        uint64_t dummy_rsp;
        context_switch(&dummy_rsp, threads[0]->rsp);
        return;
    }

    Thread *old = threads[current_index];
    if (old->state == RUNNING) old->state = READY;

    int next_index = (current_index + 1) % thread_count;
    int checked = 0;
    while (threads[next_index]->state == BLOCKED && checked < thread_count) {
        next_index = (next_index + 1) % thread_count;
        checked++;
    }

    current_index = next_index;
    Thread *next = threads[current_index];
    next->state = RUNNING;

    context_switch(&old->rsp, next->rsp);
}