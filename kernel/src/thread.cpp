#include "thread.h"
#include "kheap.h"

#define KERNEL_STACK_SIZE (16 * 1024)

static uint64_t next_thread_id = 0;

Thread *thread_create(void (*entry)(), const char *name) {
    Thread *t = (Thread *)kmalloc(sizeof(Thread));
    void *stack = kmalloc(KERNEL_STACK_SIZE);
    uint64_t stack_top = (uint64_t)stack + KERNEL_STACK_SIZE;

    uint64_t *sp = (uint64_t *)(stack_top - 8 * 8);
    sp[0] = 0;
    sp[1] = 0;
    sp[2] = 0;
    sp[3] = 0;
    sp[4] = 0;
    sp[5] = 0;
    sp[6] = 0x202;
    sp[7] = (uint64_t)entry;

    t->rsp = (uint64_t)sp;
    t->kernel_stack = stack;
    t->state = READY;
    t->id = next_thread_id++;
    t->name = name;

    return t;
}