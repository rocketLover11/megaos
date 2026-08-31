#pragma once
#include <stdint.h>

struct Spinlock {
    volatile uint32_t locked;
};

static inline void spinlock_init(Spinlock *lock) {
    lock->locked = 0;
}

static inline void spinlock_acquire(Spinlock *lock) {
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        while (lock->locked) {
            asm volatile ("pause");
        }
    }
}

static inline void spinlock_release(Spinlock *lock) {
    __sync_lock_release(&lock->locked);
}